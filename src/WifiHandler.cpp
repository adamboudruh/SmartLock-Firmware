#include <ArduinoWebsockets.h>
#include <ArduinoJson.h>
#include <WiFi.h> // Use ESP8266WiFi.h if using ESP8266
#include <Preferences.h> // for provisioning
#include "mbedtls/md.h" // for hashing
#include "WiFiHandler.h"
#include "Whitelist.h"
#include "PendingCommands.h" // for queueing locks and unlocks
#include "OutputHandler.h"
#include "Status.h"
#include "Encryption.h"
#include "EventCache.h"
#include "RtcHandler.h"

using namespace websockets;


// enums not used but defined here for clarity:

// Supported WebSocket actions (incoming from backend)
enum class WsAction {
    AUTH_OK,      // authentication confirmed, send INIT
    SYNC,         // whitelist and settings payload
    SYNC_OK,      // backend confirmed offline event upload, clear cache
    LOCK,         // remote lock command
    UNLOCK        // remote unlock command
};

// supported state events (outgoing to backend)
enum class StateEvent {
    INIT,            // device just connected, request SYNC
    LOCK,            // physical button lock
    BUTTON_UNLOCK,   // physical button unlock
    UNLOCK_SUCCESS,  // RFID key matched, unlocked
    FAIL_UNLOCK,     // RFID key not recognized
    DOOR_OPEN,       // reed switch: door opened
    DOOR_CLOSED,     // reed switch: door closed
    OFFLINE_SYNC     // batch upload of cached offline events
};

// Wi-Fi Settings
const char* ssid = "SULTANS_PALACE"; // Replace with your Wi-Fi name
const char* password       = "maryam0299";
const char* backendApiURL = "http://10.0.0.49:3000";
const char* websocketURL = "ws://10.0.0.49:3000/device"; // WebSocket endpoint

String deviceId;
String deviceSecret;
WebsocketsClient client;

bool isOnline = false;
static bool wsConnecting = false;

static unsigned long lastWifiRetry = 0;
static unsigned long lastWsRetry   = 0;
static const unsigned long WIFI_RETRY_INTERVAL = 30000; // 30s between WiFi retries
static const unsigned long WS_RETRY_INTERVAL   = 5000;  // 5s between WS retries

void loadCredentials() { // retrieve deviceId and device secret from memory
    Preferences prefs;
    prefs.begin("smartlock", true); // true = read only
    deviceId = prefs.getString("device_id", "");
    deviceSecret = prefs.getString("device_secret", "");
    prefs.end();

    if (deviceId.isEmpty() || deviceSecret.isEmpty()) {
        Serial.println("[Auth] ERROR: device_id or device_secret not provisioned in NVS!");
    } else {
        Serial.printf("[Auth] Loaded credentials for device: %s\n", deviceId.c_str());
    }
}

static void sendAuthMessage() { // initial authentication message containing credentials
    String timestamp = String(getRtcTimestamp());

    String payload   = deviceId + ":" + timestamp;
    String hmac      = computeHMAC(payload, deviceSecret);

    StaticJsonDocument<256> doc;
    doc["deviceId"]  = deviceId;
    doc["timestamp"] = timestamp;
    doc["hmac"]      = hmac;

    String output;
    serializeJson(doc, output);
    client.send(output);
    Serial.printf("[Auth] Auth message sent for device: %s\n", deviceId.c_str());
}

// WebSocket Message Callback
void onMessageCallback(WebsocketsMessage message) {
    String payload = message.data(); // Get the WebSocket message as a string
    Serial.printf("[WebSocket] Message Received: %s\n", payload.c_str());

    // Parse the JSON message
    StaticJsonDocument<256> doc; // Allocate a JSON document (adjust size as necessary)
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        Serial.print("[WebSocket] JSON Parsing Error: ");
        Serial.println(error.c_str());
        return; // Return if there’s a parsing issue
    }

    // Extract the "action" field
    const char* action = doc["action"];
    
    if (strcmp(action, "AUTH_OK") == 0) {
        Serial.println("[Auth] Authenticated! Sending INIT...");
        isOnline = true;
        setStatus(StatusMode::Connected);
        sendStateEvent("INIT");
        if (hasCachedEvents()) {
            Serial.printf("[OfflineSync] Found %d cached events, uploading...\n", getCachedEventCount());
            sendCachedEvents();
        }
    } else if (strcmp(action, "SYNC_OK") == 0) {
        Serial.println("[WebSocket] Backend confirmed, clearing cache");
        clearEventCache();
    } else if (strcmp(action, "LOCK") == 0) {
        Serial.println("[Command] LOCK received. Locking the relay.");
        pendingLock = true; // activate the relay (lock)
        isLocked = true;
    } else if (strcmp(action, "UNLOCK") == 0) {
        Serial.println("[Command] UNLOCK received. Unlocking the relay.");
        pendingUnlock = true; // deactivate the relay (unlock)
        isLocked = false;
    } else if (strcmp(action, "SYNC") == 0) {
        Serial.println("[Command] SYNC received. Saving new whitelist in memory.");
        String whitelistJson;
        serializeJson(doc["whitelist"], whitelistJson);
        saveWhitelist(whitelistJson);
    } else {
        Serial.println("[WebSocket] Unknown action received.");
    }
}

// WebSocket Event Callback
void onEventsCallback(WebsocketsEvent event, String data) {
    switch (event) {
        case WebsocketsEvent::ConnectionOpened:
            Serial.println("[WebSocket] Connection Opened!");
            sendAuthMessage(); // first thing is send credentials to get verified
            break;
        case WebsocketsEvent::ConnectionClosed:
            Serial.println("[WebSocket] Connection Closed.");
            isOnline = false;
            setStatus(StatusMode::Offline);
            break;
        case WebsocketsEvent::GotPing:
            Serial.println("[WebSocket] Received Ping.");
            break;
        case WebsocketsEvent::GotPong:
            Serial.println("[WebSocket] Received Pong.");
            break;
        default:
            Serial.println("[WebSocket] Unknown Event.");
            break;
    }
}

// Sends message of local state change to server
void sendStateEvent(const char* eventType, const char* uid) {
    String timestamp = getRtcISOString();

    if (!isOnline) { // if offline, don't bother with WS stuff, just cache the event
        Serial.printf("[Offline] Caching event: %s\n", eventType);
        cacheEvent(eventType, isLocked, isAjar, uid, timestamp);
        return;
    }

    StaticJsonDocument<256> doc;
    doc["event"]    = eventType;
    doc["isLocked"] = isLocked;
    doc["isAjar"]   = isAjar;
    doc["ts"]       = timestamp;
    if (uid != nullptr) {
        doc["uid"] = uid;
    }

    String payload;
    serializeJson(doc, payload);
    client.send(payload);
    Serial.printf("[WebSocket] State sent: %s\n", payload.c_str());
}

// function to connect to the network
void initWifi() {
    loadCredentials();

    Serial.println("[WiFi] Connecting to Wi-Fi...");
    WiFi.begin(ssid, password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 50) {
        delay(100);
        updateStatus();
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("[WiFi] Connected!");
        Serial.printf("[WiFi] IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("[WiFi] Connection failed — continuing in offline mode");
        setStatus(StatusMode::Offline);
    }
}

void initWebSocket() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WebSocket] No WiFi — skipping WS connection");
        return;
    }

    Serial.println("[WebSocket] Connecting to server...");
    setStatus(StatusMode::Connecting);
    // Set WebSocket event and message handlers
    client.onMessage(onMessageCallback); // Message handler
    client.onEvent(onEventsCallback);    // WebSocket events handler

    if (client.connect(websocketURL)) {
        Serial.println("[WebSocket] Successfully connected to the server.");
    } else {
        Serial.println("[WebSocket] Connection failed.");
        setStatus(StatusMode::Offline);
    }
}

void handleWebSocket() {
    if (client.available()) {
        client.poll();
    }
}

void reconnectTask(void* param) {
    const unsigned long WIFI_RETRY_MS = 30000;
    const unsigned long WS_RETRY_MS   = 10000;

    for (;;) { // infinite loop — task runs forever
        // Step 1: WiFi reconnect
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[Reconnect] WiFi down — attempting reconnect...");
            WiFi.begin(ssid, password);

            // Wait up to 5s for connection (blocks this task only, not loop())
            int attempts = 0;
            while (WiFi.status() != WL_CONNECTED && attempts < 50) {
                vTaskDelay(pdMS_TO_TICKS(100));
                attempts++;
            }

            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("[Reconnect] WiFi reconnected!");
                Serial.printf("[Reconnect] IP: %s\n", WiFi.localIP().toString().c_str());
            } else {
                Serial.println("[Reconnect] WiFi still down, retrying later...");
                vTaskDelay(pdMS_TO_TICKS(WIFI_RETRY_MS));
                continue; // skip WS attempt
            }
        }

        // step 2: WebSocket reconnect (only if WiFi is up and WS is down)
        if (!client.available() && !wsConnecting) {
            wsConnecting = true;
            Serial.println("[Reconnect] WS down — attempting reconnect...");
            initWebSocket();
            wsConnecting = false;
        }

        vTaskDelay(pdMS_TO_TICKS(WS_RETRY_MS));
    }
}

void startReconnectTask() {
    xTaskCreatePinnedToCore(
        reconnectTask,    // task function
        "reconnectTask",  // name
        8192,             // stack size (bytes)
        NULL,             // params
        1,                // priority (low — don't starve loop())
        NULL,             // task handle (not needed)
        0                 // core 0 (loop() runs on core 1)
    );
    Serial.println("[Reconnect] Background reconnect task started on core 0");
}

void sendCachedEvents() {
    if (!client.available()) return;

    Serial.println("[EventCache] Sending cached events...");
    String cached = getCachedEvents();
    if (cached.isEmpty()) {
        Serial.println("[EventCache] No cached events to send, syncing normally.");
        return;
    }

    DynamicJsonDocument doc(8192);
    doc["event"] = "OFFLINE_SYNC";

    DynamicJsonDocument eventsDoc(8192);
    deserializeJson(eventsDoc, cached);
    doc["events"] = eventsDoc.as<JsonArray>();

    String payload;
    serializeJson(doc, payload);
    client.send(payload);
    Serial.printf("[OfflineSync] Sent %d cached events to backend\n", getCachedEventCount());
}