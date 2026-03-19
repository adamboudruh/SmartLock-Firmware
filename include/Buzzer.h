#pragma once

void initBuzzer();
void playToneById(int toneId);   // dispatcher: 0=silent, 1-6=melodies
void playDoorWarning();  // annoying buzz
void stopWarning();  // stop the annoying buzz
void updateBuzzer();  // call in loop() for non-blocking warning
int getToneCount();  // number of available tones (for mobile UI)