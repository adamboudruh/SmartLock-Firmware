#pragma once

// ─── RFID (MFRC522)
#define SS_PIN     5
#define RST_PIN    16

// ─── RELAY (Solenoid Lock)
#define RELAY_PIN       26

// ─── REED SWITCH (Door Ajar Detection)
#define REED_PIN        14

// ─── BUTTONS
#define LOCK_BUTTON_PIN  13
#define STATE_BUTTON_PIN 27 // not used anymore
#define UNLOCK_BUTTON_PIN 33

// ─── RGB STATUS LED
#define PIN_R       4
#define PIN_G       2
#define PIN_B       15

// ─── BUZZER
#define PASSIVE_BUZZER_PIN   27

// ─── RTC (DS3231)
#define I2C_SDA_PIN    21
#define I2C_SCL_PIN    22