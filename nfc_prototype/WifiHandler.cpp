#include <ArduinoWebsockets.h>
#include <ArduinoJson.h>
#include <WiFi.h> // Use ESP8266WiFi.h if using ESP8266
#include "WiFiHandler.h"
#include "PendingCommands.h" // for queueing locks and unlocks
#include "OutputHandler.h"

using namespace websockets;

// Wi-Fi Settings
const char* ssid = "SULTANS_PALACE"; // Replace with your Wi-Fi name
const char* password = "maryam0299"; // Replace with your Wi-Fi password

// const char* dbApiURL = "https://localhost:7110";
// const char* dbApiURL = "https://smartlock-db-api-cffjc8fthjfwb9hu.canadacentral-01.azurewebsites.net";
const char* backendApiURL = "http://10.0.0.49:3000";
// const char* backendApiURL = "https://smartlock-backend-ezhcfahsavavfehu.canadacentral-01.azurewebsites.net";

const char* websocketURL = "ws://10.0.0.49:3000"; // WebSocket endpoint

WebsocketsClient client;

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
    
    if (strcmp(action, "LOCK") == 0) {
        Serial.println("[Command] LOCK received. Locking the relay.");
        pendingLock = true; // Activate the relay (lock)
        isLocked = true;
    } else if (strcmp(action, "UNLOCK") == 0) {
        Serial.println("[Command] UNLOCK received. Unlocking the relay.");
        pendingUnlock = true; // Deactivate the relay (unlock)
        isLocked = false;
    } else {
        Serial.println("[WebSocket] Unknown action received.");
    }
}

// WebSocket Event Callback
void onEventsCallback(WebsocketsEvent event, String data) {
    switch (event) {
        case WebsocketsEvent::ConnectionOpened:
            Serial.println("[WebSocket] Connection Opened!");
            client.send("ESP32 Connected!"); // Notify the server
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
  Serial.println("[WiFi] Connecting to Wi-Fi...");
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\n[WiFi] Connected to Wi-Fi!");
    Serial.print("Device IP Address: ");
    Serial.println(WiFi.localIP());

}

void initWebSocket() {
  Serial.println("[WebSocket] Connecting to WebSocket server...");

  // Set WebSocket event and message handlers
  client.onMessage(onMessageCallback); // Message handler
  client.onEvent(onEventsCallback);    // WebSocket events handler

  if (client.connect(websocketURL)) {
    Serial.println("[WebSocket] Successfully connected to the server.");
    client.send("ESP32 Connected!"); // Optional greeting
    sendStateEvent("INIT");
  } else {
    Serial.println("[WebSocket] Connection failed.");
  }
}

void handleWebSocket() {
  if (!client.available()) {
    Serial.println("[WebSocket] Disconnected. Attempting to reconnect...");
    delay(2000);
    initWebSocket(); // Reconnect if disconnected
  }

  // Poll the WebSocket for incoming messages
  client.poll();
}