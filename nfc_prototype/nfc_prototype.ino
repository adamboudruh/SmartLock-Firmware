#include <SPI.h>
#include <MFRC522.h>
#include "Tag.h"
#include "InputHandler.h"
#include "OutputHandler.h"
#include "WiFiHandler.h"
#include "Helpers.h"
#include "PendingCommands.h"
#include "Whitelist.h"
#include <Arduino.h>

#define RST_PIN 22
#define SS_PIN  5
#define REED_PIN 13
#define BUTTON_PIN 12
const int  RELAY_PIN  = 15;     // INx to your relay channel


const bool REINIT_AFTER_RELAY = true; // set true to re-init RC522 after relay action if noise causes hangs

volatile bool pendingLock=false;
volatile bool pendingUnlock = false;

MFRC522 mfrc522(SS_PIN, RST_PIN);


void setup() {
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();
  
	// going out to relay module to control solenoid
  pinMode(RELAY_PIN, OUTPUT);

  // Force relay OFF immediately after setting as OUTPUT
  // unlockDoor();
  isLocked = false;

	// going out to reed switch for logging open/close events
	pinMode(REED_PIN, INPUT_PULLUP);
	initialReedSetup();

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.println("RC522 ready. Tap a card to match against green/red tags...");

  loadWhitelist();  // load whitelist from flash to memory

  initWifi();
  initWebSocket();
}

void loop() {

  // Reed switch debounce and logging
  handleReedSwitch();

  // handle button input
  handleLockButton();
  handleStateButton();

  // handle any incoming WS commands
  handleWebSocket();

  if (pendingUnlock) {
    pendingUnlock = false;
    unlockDoor();
    if (REINIT_AFTER_RELAY) { delay(50); mfrc522.PCD_Init(); }
  }
  if (pendingLock) {
    pendingLock = false;
    lockDoor(); // true = lock
    if (REINIT_AFTER_RELAY) { delay(50); mfrc522.PCD_Init(); }
  }

  // RFID: look for a new card
  if (!mfrc522.PICC_IsNewCardPresent()) { delay(10); return; } // if a new card isn't present just return
  if (!mfrc522.PICC_ReadCardSerial())   { delay(10); return; } // if it's not reading a card, just return

  String uid = uidToString(mfrc522.uid.uidByte, mfrc522.uid.size); // convert read UID to a hex string
  Serial.print("UID: ");
  Serial.println(uid);

  // check RFID reading against whitelist
  if (handleRfidTag(uid)) {
    // If a known tag was detected, optionally wait or do any additional actions here
  }

  // Clean up reader session
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}