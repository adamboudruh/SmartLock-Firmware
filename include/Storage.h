#pragma once
#include <Arduino.h>

bool initStorage();                              // mount LittleFS, call once in setup()
String readFile(const char* path);               // returns "" if missing, otherwise returns file contents as String
bool writeFile(const char* path, const String& content);
bool fileExists(const char* path);
bool deleteFile(const char* path);