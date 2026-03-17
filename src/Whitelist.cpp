#include "Whitelist.h"
#include "Encryption.h"
#include "Storage.h"
#include <ArduinoJson.h>

#define WHITELIST_PATH "/whitelist.json"
#define MAX_TAGS 32

struct Tag {
    String uid;
    String name;
};

static Tag whitelist[MAX_TAGS];
static int whitelistSize = 0;

// parses given JSON string into in memory whitelist
static void parseIntoMemory(const String& json) {
    DynamicJsonDocument doc(2048);
    if (deserializeJson(doc, json) != DeserializationError::Ok) {
        Serial.println("[Whitelist] Failed to parse JSON");
        return;
    }
    JsonArray array = doc.as<JsonArray>(); // read from whitelist file into JSON array
    whitelistSize = 0;
    for (JsonObject key : array) {
        if (whitelistSize >= MAX_TAGS) break;
        whitelist[whitelistSize].uid  = key["uid"].as<String>();
        whitelist[whitelistSize].name = key["name"].as<String>();
        whitelistSize++;
    }
    Serial.printf("[Whitelist] Loaded %d tags into memory\n", whitelistSize);
}

void loadWhitelist() {
    String json = readFile(WHITELIST_PATH);
    if (json.isEmpty()) {
        Serial.println("[Whitelist] No cached whitelist found, using empty list");
        return;
    }
    Serial.println("[Whitelist] Loaded from flash, parsing...");
    parseIntoMemory(json);
}

bool saveWhitelist(const String& json) { // receives the json array containing the new keys from the server
    if (!writeFile(WHITELIST_PATH, json)) return false; // attempt to write to flash, if it fails return false
    Serial.println("[Whitelist] Saved to flash");
    parseIntoMemory(json);
    return true;
}

bool checkUID(const String& uid, String& outName) {
    String hashedUid = hashUID(uid); // ← hash before comparing
    for (int i = 0; i < whitelistSize; i++) {
        if (hashedUid.equalsIgnoreCase(whitelist[i].uid)) {
            outName = whitelist[i].name;
            return true;
        }
    }
    return false;
}

void printWhitelist() { // for debuggiong, sometimes I wanna see what's under the hood
    Serial.println("[Whitelist] === FLASH MEMORY ===");
    String json = readFile(WHITELIST_PATH);
    if (json.isEmpty()) {
        Serial.println("[Whitelist] No file found on flash");
    } else {
        Serial.println(json);
    }
}