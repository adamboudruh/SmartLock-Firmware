#include <SPI.h>
#include <MFRC522.h>
#include "InputHandler.h"
#include "OutputHandler.h"
#include "WiFiHandler.h"
#include "Helpers.h"
#include "PendingCommands.h"
#include "Whitelist.h"
#include "Storage.h"
#include "Status.h"
#include "Pins.h"
#include "Buzzer.h"
#include "RtcHandler.h"
#include "Settings.h"
#include <Arduino.h>


const bool REINIT_AFTER_RELAY = true; // set true to re-init RC522 after relay action if noise causes hangs

volatile bool pendingLock=false;
volatile bool pendingUnlock = false;

MFRC522 mfrc522(SS_PIN, RST_PIN);


void setup() {
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();

  // initialize the status LED
  initStatus();
  
  // initialize the buzzer
  initBuzzer();

	// going out to relay module to control solenoid
  pinMode(RELAY_PIN, OUTPUT);

  // Force relay OFF immediately after setting as OUTPUT
  // unlockDoor();
  isLocked = false;

	// going out to reed switch for logging open/close events
	pinMode(REED_PIN, INPUT_PULLUP);
	initialReedSetup();

  pinMode(UNLOCK_BUTTON_PIN, INPUT_PULLUP);
  pinMode(LOCK_BUTTON_PIN, INPUT_PULLUP);

  Serial.println("RC522 ready. Tap a card to match against green/red tags...");

  initStorage();    // mount LittleFS before any file operations
  loadWhitelist();  // load whitelist from flash to memory
  loadSettings();   // load settings from flash to memory

  setStatus(StatusMode::Connecting);
  initWifi();

  initRTC(); // initialize real time clock
  if (rtcLostPower()) {
    Serial.println("[RTC] Time is stale — will sync from NTP");
    syncRtcFromNTP();
  } else {
    Serial.println("[RTC] RTC in good shape, no need to sync");
  }
  initWebSocket();
  startReconnectTask();
}

void loop() {
  // Update the LED status
  updateStatus();

  // Reed switch debounce and logging
  handleReedSwitch();

  // handle button input
  handleLockButton();
  // handleStateButton();
  handleUnlockButton();

  // handle any incoming WS commands
  handleWebSocket();

  checkSettings(); // check for updated settings on each loop
  updateBuzzer();    // check if we need to play a tone based on recent events

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
  handleRfidTag(uid);

  // Clean up reader session
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}