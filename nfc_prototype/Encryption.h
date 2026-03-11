#pragma once
#include <Arduino.h>

String computeHMAC(const String& message, const String& secret);
String hashUID(const String& uid);