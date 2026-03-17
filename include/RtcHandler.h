#pragma once
#include <Arduino.h>

// call once in setup()
void initRTC();

// returns current Unix timestamp from the DS3231
unsigned long getRtcTimestamp();

// returns true if battery died or time was never set
bool rtcLostPower();

// returns ISO 8601 string for event payloads
String getRtcISOString();

// syncs NTP time to the DS3231 — call when WiFi is connected
bool syncRtcFromNTP();