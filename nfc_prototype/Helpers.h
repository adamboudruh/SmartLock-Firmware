#ifndef HELPERS_H
#define HELPERS_H
#pragma once
#include <Arduino.h>

// Converts UID buffer to a readable hex string
String uidToString(const byte* buffer, byte bufferSize);

#endif