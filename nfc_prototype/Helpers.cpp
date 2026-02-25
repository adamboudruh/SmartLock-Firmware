#include "Helpers.h"
#include <Arduino.h>

// builds a two digit hex representation of each UID byte
String uidToString(const byte* buffer, byte bufferSize) {
  String s;
  s.reserve(bufferSize * 2); // 2 chars per byte, no spaces
  for (byte i = 0; i < bufferSize; i++) {
    char part[3];
    sprintf(part, "%02X", buffer[i]); // ← removed leading space
    s += part;
  }
  return s; // ← removed s.trim()
}