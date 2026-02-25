#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <Arduino.h>
#include <MFRC522.h>

// Function declarations
void initialReedSetup();
bool handleRfidTag(const String& uid);
void handleReedSwitch();
void handleLockButton();
void handleStateButton();

#endif