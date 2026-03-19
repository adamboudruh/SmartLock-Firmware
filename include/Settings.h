#pragma once
#include <Arduino.h>

// must match DB enum
#define SETTING_AUTO_LOCK_ENABLED       1
#define SETTING_AUTO_LOCK_DELAY_SEC     2
#define SETTING_DOOR_BUZZ_ENABLED       3
#define SETTING_DOOR_BUZZ_DELAY_SEC     4
#define SETTING_OPEN_CLOSE_TONE_ID      5

void loadSettings();  // read from flash on boot
bool saveSettings(const String& json);  // save from SYNC to flash + memory
void checkSettings();  // call in loop(), enforces timers
void notifyUnlocked();
void notifyLocked();
void notifyDoorOpened();
void notifyDoorClosed();                      // notifications for timers
int getSettingInt(int settingId);  // get a setting value as int
bool getSettingBool(int settingId);  // get a setting value as bool