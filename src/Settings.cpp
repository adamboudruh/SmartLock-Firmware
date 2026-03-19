#include "Settings.h"
#include "Storage.h"
#include "Buzzer.h"
#include "PendingCommands.h"
#include "OutputHandler.h"
#include "WifiHandler.h"
#include <ArduinoJson.h>

#define SETTINGS_PATH "/settings.json"
#define MAX_SETTINGS 10

struct Setting {
    int id;
    String name;
    String value;
};

static Setting settings[MAX_SETTINGS];
static int settingsCount = 0;

// defaults used if no settings in flash or if parsing fails
static void loadDefaults() {
    settingsCount = 5;
    settings[0] = { SETTING_AUTO_LOCK_ENABLED,   "AutoLockEnabled",     "1"  }; // enum, string, default value
    settings[1] = { SETTING_AUTO_LOCK_DELAY_SEC,  "AutoLockDelaySec",   "30" };
    settings[2] = { SETTING_DOOR_BUZZ_ENABLED,    "DoorOpenBuzzEnabled", "1"  };
    settings[3] = { SETTING_DOOR_BUZZ_DELAY_SEC,  "DoorOpenBuzzDelaySec","60" };
    settings[4] = { SETTING_OPEN_CLOSE_TONE_ID,   "OpenCloseToneId",    "1"  };
    Serial.println("[Settings] Loaded defaults");
}

static void parseIntoMemory(const String& json) {
    DynamicJsonDocument doc(2048);
    if (deserializeJson(doc, json) != DeserializationError::Ok) {
        Serial.println("[Settings] Failed to parse JSON, using defaults");
        loadDefaults();
        return;
    }

    JsonArray array = doc.as<JsonArray>();
    settingsCount = 0;
    for (JsonObject s : array) { // read from flash into memory
        if (settingsCount >= MAX_SETTINGS) break;
        settings[settingsCount].id    = s["id"].as<int>();
        settings[settingsCount].name  = s["name"].as<String>();
        settings[settingsCount].value = s["value"].as<String>();
        settingsCount++;
    }
    Serial.printf("[Settings] Loaded %d settings into memory\n", settingsCount);

    // print them out for debugging
    for (int i = 0; i < settingsCount; i++) {
        Serial.printf("  [%d] %s = %s\n", settings[i].id, settings[i].name.c_str(), settings[i].value.c_str());
    }
}

// pull settings from flash storage on boot, load defaults if nothing
void loadSettings() {
    String json = readFile(SETTINGS_PATH);
    if (json.isEmpty()) {
        Serial.println("[Settings] No cached settings, using defaults");
        loadDefaults();
        return;
    }
    Serial.println("[Settings] Loaded from flash, parsing...");
    parseIntoMemory(json);
}

bool saveSettings(const String& json) {
    if (!writeFile(SETTINGS_PATH, json)) return false;
    Serial.println("[Settings] Saved to flash");
    parseIntoMemory(json);
    return true;
}

static String getSettingValue(int settingId) {
    for (int i = 0; i < settingsCount; i++) {
        if (settings[i].id == settingId) return settings[i].value;
    }
    return "";
}

int getSettingInt(int settingId) {
    String val = getSettingValue(settingId);
    return val.isEmpty() ? 0 : val.toInt();
}

bool getSettingBool(int settingId) {
    return getSettingInt(settingId) != 0;
}

// timer state vars
static bool autoLockPending = false;  // whether auto-lock is currently counting down
static unsigned long unlockTimestamp = 0;  // time when the door was unlocked, start counting from here

static bool doorOpenPending = false;  // whether door open warning is counting down
static unsigned long doorOpenTimestamp = 0;  // time when door was opened, start counting from here
static bool doorWarningActive = false;  // whether buzzer is currently going off

// notifications from elsewhere in code to manage timers
void notifyUnlocked() {
    // play unlock tone
    int toneId = getSettingInt(SETTING_OPEN_CLOSE_TONE_ID);
    Serial.printf("[Settings] Playing unlock tone with ID %d\n", toneId);
    if (toneId > 0) {
        playToneById(toneId);
    }

    Serial.println("[Settings] Notified of unlock event");
    if (getSettingBool(SETTING_AUTO_LOCK_ENABLED)) { // only start counting down if setting is enabled
        Serial.println("[Settings] Auto lock enabled, arming auto lock timer");
        autoLockPending = true;
        unlockTimestamp = millis();
        Serial.printf("[Settings] Auto lock armed: %d seconds\n",
            getSettingInt(SETTING_AUTO_LOCK_DELAY_SEC));
    }
}

void notifyLocked() {
    // cancel auto-lock if manually locked
    autoLockPending = false;
    Serial.println("[Settings] Auto lock cancelled");
}

void notifyDoorOpened() {
    if (getSettingBool(SETTING_DOOR_BUZZ_ENABLED)) {
        doorOpenPending = true;
        doorOpenTimestamp = millis();
        Serial.printf("[Settings] Door open timer started: %d seconds\n",
            getSettingInt(SETTING_DOOR_BUZZ_DELAY_SEC));
    }
}

void notifyDoorClosed() {
    doorOpenPending = false;
    if (doorWarningActive) {
        stopWarning();
        doorWarningActive = false;
        Serial.println("[Settings] Door closed — warning stopped");
    }
}

// enforce settings every iteration
void checkSettings() {
    unsigned long now = millis();

    // auto lock check
    if (autoLockPending) {
        unsigned long delaySec = getSettingInt(SETTING_AUTO_LOCK_DELAY_SEC);
        if (now - unlockTimestamp >= delaySec * 1000) { // if exceeded delay, trigger auto-lock
            Serial.println("[Settings] Auto lock triggered");
            autoLockPending = false;
            isLocked = true;
            sendStateEvent("AUTO_LOCK");
            pendingLock = true;
        }
    }

    // door open too long check
    if (doorOpenPending && !doorWarningActive) {
        unsigned long delaySec = getSettingInt(SETTING_DOOR_BUZZ_DELAY_SEC);
        if (now - doorOpenTimestamp >= delaySec * 1000) {
            Serial.println("[Settings] Door open too long, buzzing!");
            doorWarningActive = true;
            playDoorWarning();
        }
    }
}