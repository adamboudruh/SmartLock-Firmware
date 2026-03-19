#include "Buzzer.h"
#include "Pins.h"
#include "Pitches.h"
#include <Arduino.h>

// ── Warning buzz state (non-blocking) ──
static bool warningActive = false;
static unsigned long lastWarningToggle = 0;
static bool warningOn = false;
static const unsigned long WARNING_ON_MS  = 200;
static const unsigned long WARNING_OFF_MS = 300;

void initBuzzer() {
    pinMode(PASSIVE_BUZZER_PIN, OUTPUT);
    digitalWrite(PASSIVE_BUZZER_PIN, LOW);
}

// play a melody array (blocking, but short)
static void playMelody(const int* notes, const int* durations, int length) {
    for (int i = 0; i < length; i++) {
        int noteDuration = 1000 / durations[i];
        if (notes[i] == NOTE_REST) {
            delay(noteDuration);
        } else {
            tone(PASSIVE_BUZZER_PIN, notes[i], noteDuration);
            delay(noteDuration * 1.3); // slight gap between notes
        }
        noTone(PASSIVE_BUZZER_PIN);
    }
}

// 1: Happy (two quick rising notes)
static void playHappyTone() {
    int notes[]     = { NOTE_C5, NOTE_E5, NOTE_G5, NOTE_C6 };
    int durations[] = { 8,       8,       8,       4       };
    playMelody(notes, durations, 4);
}

// 2: Sad (descending minor)
static void playSadTone() {
    int notes[]     = { NOTE_E5, NOTE_DS5, NOTE_D5, NOTE_CS5, NOTE_C5 };
    int durations[] = { 4,       4,        4,       4,        2       };
    playMelody(notes, durations, 5);
}

// 3: Mario coin sound
static void playMarioCoin() {
    int notes[]     = { NOTE_B5, NOTE_E6 };
    int durations[] = { 10,      4       };
    playMelody(notes, durations, 2);
}

// 4: Dumpster Baby
static void playDumpsterBaby() {
    int notes[]     = { NOTE_GS4, NOTE_AS4, NOTE_F4, NOTE_FS4, NOTE_CS5, NOTE_FS5,
                        NOTE_CS6, NOTE_FS4, NOTE_F4, NOTE_FS4, NOTE_CS5, NOTE_FS5, NOTE_CS6 };
    int durations[] = { 12,       12,       12,      12,       12,       12,
                        12,       12,       12,      12,       12,       12,       12      };
    playMelody(notes, durations, 13);
}

// 5: Lucy in the Sky
static void playLucy() {
    int notes[]     = { NOTE_E3, NOTE_A3, NOTE_E4, NOTE_G3, NOTE_E4, NOTE_A3,
                        NOTE_FS3, NOTE_A3, NOTE_E4, NOTE_F3, NOTE_D4, NOTE_CS4, NOTE_A3 };
    int durations[] = { 4,       4,       4,       4,       4,       4,
                        4,       4,       4,       4,       8,       8,        4       };
    playMelody(notes, durations, 13);
}

// 6: Be Nice 2 Me
static void playBeNice2Me() {
    int notes[]     = { NOTE_G3,  NOTE_FS4, NOTE_A4,  NOTE_A6,  NOTE_A6,
                        NOTE_REST, NOTE_G6, NOTE_REST, NOTE_FS6, NOTE_REST,
                        NOTE_D6,  NOTE_REST, NOTE_A6,  NOTE_CS6, NOTE_D6 };
    int durations[] = { 8,        8,        8,        8,        8,
                        8,        8,        8,        8,        8,
                        4,        8,        8,        8,        8       };
    playMelody(notes, durations, 15);
}

// 7: Here Comes The Sun
static void playHereComesTheSun() {
    int notes[] = {
        NOTE_CS5, NOTE_A4, NOTE_B4, NOTE_CS5, NOTE_A4, NOTE_CS5, NOTE_B4, NOTE_A4,
        NOTE_FS4, NOTE_A4, NOTE_B4, NOTE_A4, NOTE_FS4, NOTE_GS4, NOTE_FS4, NOTE_GS4,
        NOTE_A4,  NOTE_B4
    };
    int durations[] = {
        8, 8, 8, 4, 3, 8, 4, 4,
        4, 4, 4, 4, 8, 8, 8, 8,
        4, 4
    };
    playMelody(notes, durations, 18);
}


void playToneById(int toneId) {
    switch (toneId) { // ensure toneId is between 1 and 6
        case 0:  break;               // silent
        case 1:  playHappyTone();       break;
        case 2:  playSadTone();         break;
        case 3:  playMarioCoin();       break;
        case 4:  playDumpsterBaby();    break;
        case 5:  playLucy();            break;
        case 6:  playBeNice2Me();       break;
        case 7:  playHereComesTheSun(); break;
        default: playHappyTone();       break;
    }
}

int getToneCount() {
    return 7; // update when adding new tones
}

// ── Door open warning (non-blocking, called from loop via updateBuzzer) ─���
void playDoorWarning() {
    warningActive = true;
    lastWarningToggle = millis();
    warningOn = true;
    tone(PASSIVE_BUZZER_PIN, NOTE_A5);
}

void stopWarning() {
    warningActive = false;
    warningOn = false;
    noTone(PASSIVE_BUZZER_PIN);
}

void updateBuzzer() {
    if (!warningActive) return;

    unsigned long now = millis();
    unsigned long interval = warningOn ? WARNING_ON_MS : WARNING_OFF_MS;

    if (now - lastWarningToggle >= interval) {
        lastWarningToggle = now;
        warningOn = !warningOn;
        if (warningOn) {
            tone(PASSIVE_BUZZER_PIN, NOTE_A5);
        } else {
            noTone(PASSIVE_BUZZER_PIN);
        }
    }
}