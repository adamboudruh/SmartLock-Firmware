#include "Whitelist.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

#define WHITELIST_PATH "/whitelist.json"
#define MAX_TAGS 32

struct Tag {
    String uid;
    String name;
};

static Tag whitelist[MAX_TAGS];
static int whitelistSize = 0;

static void parseIntoMemory(const String& json) {
    StaticJsonDocument<2048> doc;
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
    if (!LittleFS.begin(true)) {  // pass true to format on mount failure
        Serial.println("[Whitelist] LittleFS mount failed even after format");
        return;
    }
    if (!LittleFS.begin()) {
        Serial.println("[Whitelist] LittleFS mount failed");
        return;
    }
    if (!LittleFS.exists(WHITELIST_PATH)) {
        Serial.println("[Whitelist] No cached whitelist found, using empty list");
        return;
    }
    File f = LittleFS.open(WHITELIST_PATH, "r");
    String json = f.readString();
    f.close();
    Serial.println("[Whitelist] Loaded from flash, parsing...");
    parseIntoMemory(json);
}

bool saveWhitelist(const String& json) { // receives the json array containing the new keys from the server
    if (!LittleFS.begin()) return false;
    File f = LittleFS.open(WHITELIST_PATH, "w");
    if (!f) return false;
    f.print(json);
    f.close();
    Serial.println("[Whitelist] Saved to flash");
    parseIntoMemory(json); // rebuild the in memory array right away
    return true;
}

bool checkUID(const String& uid, String& outName) {
    for (int i = 0; i < whitelistSize; i++) {
        if (uid.equalsIgnoreCase(whitelist[i].uid)) {
            outName = whitelist[i].name;
            return true;
        }
    }
    return false;
}

void printWhitelist() {
    // --- Flash contents ---
    Serial.println("[Whitelist] === FLASH ===");
    if (!LittleFS.begin(true)) {
      Serial.println("[Whitelist] LittleFS mount failed");
    }
    if (!LittleFS.begin()) {
        Serial.println("[Whitelist] LittleFS mount failed");
    } else if (!LittleFS.exists(WHITELIST_PATH)) {
        Serial.println("[Whitelist] No file found on flash");
    } else {
        File f = LittleFS.open(WHITELIST_PATH, "r");
        Serial.println(f.readString());
        f.close();
    }

    // --- In-memory contents ---
    Serial.println("[Whitelist] === MEMORY ===");
    if (whitelistSize == 0) {
        Serial.println("[Whitelist] Empty — no tags loaded in memory");
        return;
    }
    Serial.printf("[Whitelist] %d tag(s):\n", whitelistSize);
    for (int i = 0; i < whitelistSize; i++) {
        Serial.printf("  [%d] name=%-20s uid=%s\n", i, whitelist[i].name.c_str(), whitelist[i].uid.c_str());
    }
}