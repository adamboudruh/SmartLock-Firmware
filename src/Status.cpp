#include "Status.h"
#include "Pins.h"
#include <Arduino.h>
#include <math.h>

#define CH_R 5
#define CH_G 6
#define CH_B 7

// state
static StatusMode currentMode   = StatusMode::Connecting;
static StatusMode previousMode  = StatusMode::Connecting; // restore after one-shot animations
static bool       ledState      = false;
static int        flashCount    = 0;
static unsigned long lastToggle = 0;
static unsigned long modeEnteredAt = 0;

static void setRGB(bool r, bool g, bool b) {
    ledcWrite(CH_R, r ? 255 : 0);
    ledcWrite(CH_G, g ? 255 : 0);
    ledcWrite(CH_B, b ? 255 : 0);
}

static void ledOff() {
    setRGB(false, false, false);
}

void setStatus(StatusMode mode) {
    // Don't flash "connecting" if we're already in offline mode — stay yellow
    if (mode == StatusMode::Connecting && currentMode == StatusMode::Offline) return;

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
    ledcSetup(CH_R, 5000, 8); ledcAttachPin(PIN_R, CH_R);
    ledcSetup(CH_G, 5000, 8); ledcAttachPin(PIN_G, CH_G);
    ledcSetup(CH_B, 5000, 8); ledcAttachPin(PIN_B, CH_B);
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
            // Smooth RGB sine-wave rainbow — channels 120° apart
            float t = (float)(now % 4000) / 4000.0f * 2.0f * (float)M_PI;
            ledcWrite(CH_R, (uint8_t)((sinf(t)              + 1.0f) * 127.5f));
            ledcWrite(CH_G, (uint8_t)((sinf(t + 2.0944f)    + 1.0f) * 127.5f));
            ledcWrite(CH_B, (uint8_t)((sinf(t + 4.1888f)    + 1.0f) * 127.5f));
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

        case StatusMode::Unlock: {
            // Solid green for 1 second then restore
            setRGB(false, true, false);
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