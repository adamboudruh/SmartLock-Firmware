#include "OutputHandler.h"
#include <Arduino.h>
#include "WifiHandler.h"

const int  RELAY_PIN  = 15;     // INx to your relay channel
const bool ACTIVE_LOW = true;   // MOST 2-ch relay boards are low-trigger: LOW = ON
bool isLocked = true;          // assume locked on boot
bool isAjar = false;          // assume closed on boot


static void applyRelay(bool lock) {
    digitalWrite(RELAY_PIN, (ACTIVE_LOW ? (lock ? HIGH : LOW) : (lock ? LOW : HIGH)));
    Serial.printf("[%lu] Relay: %s\n", millis(), lock ? "ON (solenoid energized)" : "OFF (released)");
}

void lockDoor() {
    applyRelay(true);
}

void unlockDoor() {
    applyRelay(false); //sendStateEvent() already called in RfidHandler
}