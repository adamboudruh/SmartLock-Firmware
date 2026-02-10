#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN 22
#define SS_PIN  5
#define REED_PIN 4

const int  RELAY_PIN  = 15;     // INx to your relay channel
const bool ACTIVE_LOW = true;   // MOST 2-ch relay boards are low-trigger: LOW = ON
const bool REINIT_AFTER_RELAY = true; // set true to re-init RC522 after relay action if noise causes hangs
const unsigned long DEBOUNCE_MS = 50;

int lastSwitchReading = HIGH;
int stableState = HIGH;
unsigned long lastDebounceTs = 0;

MFRC522 mfrc522(SS_PIN, RST_PIN);

// Expected tag UIDs in "XX XX XX ..." uppercase hex with spaces
const char* greenTag = "04 6A F0 60 3E 61 80";
const char* redTag   = "04 C9 B7 60 3E 61 80";

bool relayOn = false; // mutable state
bool blockUntilRemoval = false;

static String uidToString(const byte* buffer, byte bufferSize) {
  String s;
  s.reserve(bufferSize * 3); // " XX" per byte
  for (byte i = 0; i < bufferSize; i++) {
    char part[4];
    sprintf(part, " %02X", buffer[i]);  // uppercase hex
    s += part;
  }
  s.trim();
  return s;
}

static void setRelay(bool on) {
  relayOn = on;
  // OFF level depends on ACTIVE_LOW
  digitalWrite(RELAY_PIN, (ACTIVE_LOW ? (on ? LOW : HIGH) : (on ? HIGH : LOW)));
  Serial.printf("[%lu] Relay: %s\n", millis(), on ? "ON (solenoid energized)" : "OFF (released)");
}

void setup() {
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();
	// going out to relay module to control solenoid
  pinMode(RELAY_PIN, OUTPUT);
  // Force relay OFF immediately after setting as OUTPUT
  setRelay(false);

	// going out to reed switch for logging open/close events
	pinMode(REED_PIN, INPUT_PULLUP);
	lastSwitchReading = digitalRead(REED_PIN);
	stableState = lastSwitchReading;

  Serial.println("RC522 ready. Tap a card to match against green/red tags...");
}

void loop() {
  // Gate reads until the card is removed
  if (blockUntilRemoval) {
    if (!mfrc522.PICC_IsNewCardPresent()) {
      blockUntilRemoval = false; // card removed; allow next read
    }
    // Still block other work only minimally; continue reed handling below
  }

  // Reed switch debounce and logging
  int reading = digitalRead(REED_PIN);
  if (reading != lastSwitchReading) {
    lastDebounceTs = millis();
  }
  if ((millis() - lastDebounceTs) > DEBOUNCE_MS) {
    if (reading != stableState) {
      stableState = reading;
      if (stableState == LOW) {
        Serial.printf("[%lu] Door: OPEN\n", millis());
      } else {
        Serial.printf("[%lu] Door: CLOSED\n", millis());
      }
    }
  }
  lastSwitchReading = reading;

  // If still blocking due to held card, skip RFID processing
  if (blockUntilRemoval) {
    delay(20);
    return;
  }

  // RFID: look for a new card
  if (!mfrc522.PICC_IsNewCardPresent()) { delay(10); return; }
  if (!mfrc522.PICC_ReadCardSerial())   { delay(10); return; }

  String uid = uidToString(mfrc522.uid.uidByte, mfrc522.uid.size);
  Serial.print("UID: ");
  Serial.println(uid);

  // Decide action
  bool shouldUnlock = false, shouldLock = false;
  if (uid.equalsIgnoreCase(greenTag)) {
    Serial.println("Match: GREEN tag -> UNLOCK");
    shouldUnlock = true;
  } else if (uid.equalsIgnoreCase(redTag)) {
    Serial.println("Match: RED tag -> LOCK");
    shouldLock = true;
  } else {
    Serial.println("No match: unknown tag");
  }

  // Actuate relay
  if (shouldUnlock) setRelay(true);
  if (shouldLock)   setRelay(false);

  // Clean up reader session
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  // Optional reinit to recover from EMI
  if (REINIT_AFTER_RELAY && (shouldUnlock || shouldLock)) {
    delay(50);
    mfrc522.PCD_Init();
  }

  // Enable gating: do not process more cards until the current one is removed
  blockUntilRemoval = true;
  delay(50);
}