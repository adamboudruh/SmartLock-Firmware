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

using namespace websockets;

// Wi-Fi Settings
const char* ssid = "SULTANS_PALACE"; // Replace with your Wi-Fi name
const char* password       = "";
const char* backendApiURL = "http://10.0.0.49:3000";
// const char* backendApiURL = "https://smartlock-backend-ezhcfahsavavfehu.canadacentral-01.azurewebsites.net";
const char* websocketURL = "ws://10.0.0.49:3000/device"; // WebSocket endpoint

String deviceId;
String deviceSecret;
WebsocketsClient client;

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
    // Use real time if NTP is synced, otherwise millis() as fallback
    String timestamp = String((unsigned long)time(nullptr));
    if (timestamp == "0") timestamp = String(millis()); // NTP not synced yet

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
        sendStateEvent("INIT");
    } else if (strcmp(action, "LOCK") == 0) {
        Serial.println("[Command] LOCK received. Locking the relay.");
        pendingLock = true; // Activate the relay (lock)
        isLocked = true;
    } else if (strcmp(action, "UNLOCK") == 0) {
        Serial.println("[Command] UNLOCK received. Unlocking the relay.");
        pendingUnlock = true; // Deactivate the relay (unlock)
        isLocked = false;
    } else if (strcmp(action, "SYNC") == 0) {
        Serial.println("[Command] SYNC received. Saving new whitelist in memory.");
        saveWhitelist(doc["whitelist"]);
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
    if (!client.available()) return;

    StaticJsonDocument<192> doc;
    doc["event"]    = eventType;
    doc["isLocked"] = isLocked;
    doc["isAjar"]   = isAjar;
    doc["ts"]       = millis();
    if (uid != nullptr) {
        doc["uid"] = uid; // ← only included if provided
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

    while (WiFi.status() != WL_CONNECTED) {
        delay(10);
        updateStatus();
    }
    Serial.println("\n[WiFi] Connected to Wi-Fi!");
    Serial.print("Device IP Address: ");
    Serial.println(WiFi.localIP());

    configTime(0, 0, "pool.ntp.org");
    Serial.print("[NTP] Syncing time...");
    while (time(nullptr) < 100000) { // time(nullptr) returns 1 until synced
        delay(100);
        Serial.print(".");
    }
    Serial.printf("\n[NTP] Time synced: %lu\n", (unsigned long)time(nullptr));
}

void initWebSocket() {
  Serial.println("[WebSocket] Connecting to server...");
  setStatus(StatusMode::Connecting);
  // Set WebSocket event and message handlers
  client.onMessage(onMessageCallback); // Message handler
  client.onEvent(onEventsCallback);    // WebSocket events handler

  if (client.connect(websocketURL)) {
    Serial.println("[WebSocket] Successfully connected to the server.");
    setStatus(StatusMode::Connected);
  } else {
    Serial.println("[WebSocket] Connection failed.");
  }
}

void handleWebSocket() {
  if (!client.available()) {
    Serial.println("[WebSocket] Disconnected. Attempting to reconnect...");
    delay(2000);
    setStatus(StatusMode::Connecting);
    initWebSocket(); // Reconnect if disconnected
    return;
  }

  // Poll the WebSocket for incoming messages
  client.poll();
}