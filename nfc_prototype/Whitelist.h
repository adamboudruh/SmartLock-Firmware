#pragma once
#include <Arduino.h>

void loadWhitelist();  // call when booting up, loads the whitelist from flash memory
bool saveWhitelist(const String& json);   // saves new whitelist to memory
bool checkUID(const String& uid, String& outName); // returns true along with the name of the key if found
void printWhitelist();  // for debugging and stuff