#include "Buzzer.h"
#include "Pins.h"
#include <Arduino.h>

void initBuzzer() {
    pinMode(PASSIVE_BUZZER_PIN, OUTPUT);
    digitalWrite(PASSIVE_BUZZER_PIN, LOW);
}

void playUnlockTone() {
    // Two quick rising notes — short and satisfying
    tone(PASSIVE_BUZZER_PIN, 1047, 80);  // C6
    delay(100);
    tone(PASSIVE_BUZZER_PIN, 1319, 80);  // E6
    delay(100);
    noTone(PASSIVE_BUZZER_PIN);
}