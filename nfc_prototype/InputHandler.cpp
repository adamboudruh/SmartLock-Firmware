#include "InputHandler.h"
#include "Tag.h"
#include "PendingCommands.h"
#include "Helpers.h"
#include "OutputHandler.h"
#include "WiFiHandler.h"
#include <Arduino.h>

#define REED_PIN 13
#define LOCK_BUTTON_PIN 12
#define STATE_BUTTON 14

// state for reed switch
const unsigned long DEBOUNCE_MS = 50;
int lastSwitchReading = HIGH;
int stableState = HIGH;
unsigned long lastDebounceTs = 0;
// state for button
unsigned long lastButtonPress = 0;
// state for rfid reader
String lastScannedUid = "";
unsigned long lastScanTs = 0;
const unsigned long RFID_DEBOUNCE_MS = 2000;

// Define the whitelist array and its size
const Tag whitelist[] = {
  {"Green", "046AF0603E6180"},
  {"Red",   "04C9B7603E6180"}
};

const int whitelistSize = sizeof(whitelist) / sizeof(whitelist[0]);

void initialReedSetup() {
	lastSwitchReading = digitalRead(REED_PIN);
	stableState = lastSwitchReading;
  isAjar = (stableState == LOW);
}

// check reed switch state changes and handle them
void handleReedSwitch() {
  int reading = digitalRead(REED_PIN);
  if (reading != lastSwitchReading) { // if reading has changed, set the debounce
    lastDebounceTs = millis();
  }
  if ((millis() - lastDebounceTs) > DEBOUNCE_MS) { // if debounce time has not passed, don't bother reading as it's too soon
    if (reading != stableState) { // if the state has changed
      stableState = reading; //set stableState to new state (reading)
      if (stableState == LOW) { // LOW means that the circuit is not running, aka the door is open, switches are separated
        isAjar = true;
        Serial.printf("[%lu] Door: OPEN\n", millis());
        sendStateEvent("DOOR_OPEN");
      } else {
        isAjar = false;
        Serial.printf("[%lu] Door: CLOSED\n", millis());
        sendStateEvent("DOOR_CLOSED");
      }
    }
  }
  lastSwitchReading = reading; // set lastSwitchReading for future reference
}

// Act accordingly based on RFID reading
bool handleRfidTag(const String& uid) {
  unsigned long now = millis();

  if (!isLocked) {
    Serial.println("Already unlocked, skipping.");
    return false;
  }

  // Gate: ignore if same card is still present within debounce window
  if (uid.equalsIgnoreCase(lastScannedUid) && (now - lastScanTs) < RFID_DEBOUNCE_MS) {
    Serial.println("[RFID] Same card still present, ignoring.");
    return false;
  }

  // Update tracking
  lastScannedUid = uid;
  lastScanTs = now;

  // Rest of your existing logic below, unchanged
  for (int i = 0; i < whitelistSize; i++) {
    if (uid.equalsIgnoreCase(whitelist[i].uid)) {
      Serial.printf("Match: %s tag -> UNLOCK\n", whitelist[i].name);
      pendingUnlock = true;
      isLocked = false;
      sendStateEvent("UNLOCK_SUCCESS", uid.c_str());
      return true;
    }
  }
  Serial.println("No match: unknown tag");
  sendStateEvent("FAIL_UNLOCK", uid.c_str());
  return false;
}

// handle lock button input
void handleLockButton() {
  static int lastButtonState = HIGH;
  int buttonState = digitalRead(LOCK_BUTTON_PIN); // ← was BUTTON_PIN, now fixed

  if (buttonState == LOW && lastButtonState == HIGH && !isLocked) {
    if (millis() - lastButtonPress > 200) {
      Serial.println("Button pressed -> LOCK");
      pendingLock = true;
      isLocked = true;
      sendStateEvent("LOCK");
      lastButtonPress = millis();
    }
  }
  lastButtonState = buttonState;
}

// handle state button input - prints current lock state to serial
void handleStateButton() {
  static int lastButtonState = HIGH;
  static unsigned long lastButtonPress = 0;
  int buttonState = digitalRead(STATE_BUTTON);

  if (buttonState == LOW && lastButtonState == HIGH) {
    if (millis() - lastButtonPress > 200) {
      Serial.printf("[%lu] Lock: %s | Door: %s\n",
          millis(),
          isLocked ? "LOCKED" : "UNLOCKED",
          isAjar   ? "AJAR"   : "CLOSED"
      );
      lastButtonPress = millis();
    }
  }
  lastButtonState = buttonState;
}