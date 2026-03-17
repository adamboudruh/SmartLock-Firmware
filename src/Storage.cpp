#include "Storage.h"
#include <LittleFS.h>

static bool mounted = false;

// This file provides a simple wrapper for littleFS operations to make the rest of the code
// a little more readable, also avoids having to import LittleFS in a abunch of files

bool initStorage() { // mount LittleFS, call once in setup()
    if (mounted) return true;
    if (!LittleFS.begin(true)) {
        Serial.println("[Storage] LittleFS mount failed even after format");
        return false;
    }
    mounted = true;
    Serial.println("[Storage] LittleFS mounted");
    return true;
}

String readFile(const char* path) { // read file of path, returns "" if missing
    if (!mounted) return "";
    if (!LittleFS.exists(path)) return "";
    File f = LittleFS.open(path, "r");
    if (!f) return "";
    String content = f.readString();
    f.close();
    return content;
}

bool writeFile(const char* path, const String& content) {
    if (!mounted) return false;
    File f = LittleFS.open(path, "w");
    if (!f) return false;
    f.print(content);
    f.close();
    return true;
}

bool fileExists(const char* path) {
    if (!mounted) return false;
    return LittleFS.exists(path);
}

bool deleteFile(const char* path) {
    if (!mounted) return false;
    if (!LittleFS.exists(path)) return true; // already gone
    return LittleFS.remove(path);
}