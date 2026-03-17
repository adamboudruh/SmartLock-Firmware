#include "EventCache.h"
#include "Storage.h"
#include <ArduinoJson.h>

#define CACHE_PATH "/event_cache.json"
#define MAX_CACHED_EVENTS 200 // TODO: consider if this is a good number, want at least a day's worth of events

void cacheEvent(const char* event, bool isLocked, bool isAjar, const char* uid, const String& timestamp) {
    // read existing cache
    DynamicJsonDocument doc(8192);
    String existing = readFile(CACHE_PATH);
    if (!existing.isEmpty()) {
        deserializeJson(doc, existing);
    }

    JsonArray arr = doc.is<JsonArray>() ? doc.as<JsonArray>() : doc.to<JsonArray>(); // either read it as JsonArray or convert it to JsonArray

    if (arr.size() >= MAX_CACHED_EVENTS) {
        Serial.println("[EventCache] Cache full, dropping oldest event");
        // remove first element by rebuilding as ArduinoJson doesn't support shift
        DynamicJsonDocument newDoc(8192);
        JsonArray newArr = newDoc.to<JsonArray>();
        bool skippedFirst = false;
        for (JsonObject obj : arr) {
            if (!skippedFirst) { skippedFirst = true; continue; }
            newArr.add(obj);
        }
        doc = newDoc;
        arr = doc.as<JsonArray>();
    }

    JsonObject entry = arr.createNestedObject();
    entry["event"]    = event;
    entry["isLocked"] = isLocked;
    entry["isAjar"]   = isAjar;
    entry["ts"]       = timestamp;
    if (uid != nullptr) {
        entry["uid"] = uid;
    }

    String output;
    serializeJson(doc, output);
    writeFile(CACHE_PATH, output);

    Serial.printf("[EventCache] Cached event: %s (total: %d)\n", event, arr.size());
}

bool hasCachedEvents() {
    return fileExists(CACHE_PATH) && !readFile(CACHE_PATH).isEmpty();
}

String getCachedEvents() {
    return readFile(CACHE_PATH);
}

void clearEventCache() {
    deleteFile(CACHE_PATH);
    Serial.println("[EventCache] Cache cleared");
}

int getCachedEventCount() {
    String json = readFile(CACHE_PATH);
    if (json.isEmpty()) return 0;
    DynamicJsonDocument doc(8192);
    deserializeJson(doc, json);
    return doc.as<JsonArray>().size();
}