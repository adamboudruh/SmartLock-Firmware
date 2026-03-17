#include "RtcHandler.h"
#include <Wire.h>
#include <RTClib.h>

static RTC_DS3231 rtc;
static bool rtcAvailable = false;

void initRTC() {
    Wire.begin(); // SDA=21, SCL=22 (ESP32 defaults)

    if (!rtc.begin()) {
        Serial.println("[RTC] DS3231 not found!");
        rtcAvailable = false;
        return;
    }

    rtcAvailable = true;

    if (rtc.lostPower()) {
        Serial.println("[RTC] Lost power — time is stale, will need NTP sync");
    }

    DateTime now = rtc.now();
    Serial.printf("[RTC] Current time: %04d-%02d-%02dT%02d:%02d:%02dZ\n",
        now.year(), now.month(), now.day(),
        now.hour(), now.minute(), now.second());
}

bool syncRtcFromNTP() {
    if (!rtcAvailable) {
        Serial.println("[RTC] Cannot sync — DS3231 not available");
        return false;
    }

    time_t ntpTime = time(nullptr);
    if (ntpTime < 100000) {
        Serial.println("[RTC] Cannot sync — NTP not ready yet");
        return false;
    }

    struct tm* timeInfo = gmtime(&ntpTime);
    rtc.adjust(DateTime(
        timeInfo->tm_year + 1900,
        timeInfo->tm_mon + 1,
        timeInfo->tm_mday,
        timeInfo->tm_hour,
        timeInfo->tm_min,
        timeInfo->tm_sec
    ));

    Serial.printf("[RTC] Synced from NTP: %lu\n", ntpTime);
    return true;
}

unsigned long getRtcTimestamp() {
    if (!rtcAvailable) return 0;
    return rtc.now().unixtime();
}

String getRtcISOString() {
    if (!rtcAvailable) return "1970-01-01T00:00:00Z";

    DateTime now = rtc.now();
    char buf[25];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ",
        now.year(), now.month(), now.day(),
        now.hour(), now.minute(), now.second());
    return String(buf);
}

bool rtcLostPower() {
    if (!rtcAvailable) return true;
    return rtc.lostPower();
}