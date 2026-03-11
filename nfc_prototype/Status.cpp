#include "Status.h"
#include <Arduino.h>

#define PIN_R 4
#define PIN_G 2
#define PIN_B 15

// state
static StatusMode currentMode   = StatusMode::Connecting;
static StatusMode previousMode  = StatusMode::Connecting; // restore after one-shot animations
static bool       ledState      = false;
static int        flashCount    = 0;
static unsigned long lastToggle = 0;
static unsigned long modeEnteredAt = 0;

static void setRGB(bool r, bool g, bool b) {
    digitalWrite(PIN_R, r ? HIGH : LOW);
    digitalWrite(PIN_G, g ? HIGH : LOW);
    digitalWrite(PIN_B, b ? HIGH : LOW);
}

static void ledOff() {
    setRGB(false, false, false);
}

void setStatus(StatusMode mode) {
    // Save previous mode so one-shot animations can restore it
    if (mode != StatusMode::UnlockSuccess && mode != StatusMode::UnlockFail) {
        previousMode = mode;
    }
    currentMode = mode;
    flashCount  = 0;
    ledState    = false;
    lastToggle  = 0; // force immediate update on next updateStatus() call
    modeEnteredAt = millis();
}

void initStatus() {
    pinMode(PIN_R, OUTPUT);
    pinMode(PIN_G, OUTPUT);
    pinMode(PIN_B, OUTPUT);
    ledOff();
}

void updateStatus() {
    unsigned long now = millis();

    switch (currentMode) {

        case StatusMode::Connecting: {
            // Flash red every 300ms
            if (now - lastToggle >= 300) {
                ledState = !ledState;
                setRGB(ledState, false, false);
                lastToggle = now;
            }
            break;
        }

        case StatusMode::Offline: { // solid yellow
            setRGB(true, true, false);
            break;
        }

        case StatusMode::Connected: {
            // Solid blue
            setRGB(false, false, true);
            break;
        }

        case StatusMode::Lock: {
            // Solid red for 1 second then restore
            setRGB(true, false, false);
            if (now - modeEnteredAt >= 1000) {
            currentMode = StatusMode::Connected;
            }
            break;
        }

        case StatusMode::UnlockSuccess: { // flash green 3 times
            if (flashCount >= 6) {
                ledOff();
                currentMode = StatusMode::Connected;
                flashCount  = 0;
                lastToggle  = 0;
                break;
            }
            if (now - lastToggle >= 200) {
                ledState = !ledState;
                setRGB(false, ledState, false);
                lastToggle = now;
                flashCount++;
            }
            break;
        }

        case StatusMode::UnlockFail: { // flash red 3 times 
            if (flashCount >= 6) {
                ledOff();
                currentMode = StatusMode::Connected;
                flashCount  = 0;
                lastToggle  = 0;
                break;
            }
            if (now - lastToggle >= 200) {
                ledState = !ledState;
                setRGB(ledState, false, false);
                lastToggle = now;
                flashCount++;
            }
            break;
        }
    }
}