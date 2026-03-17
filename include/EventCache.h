#pragma once
#include <Arduino.h>

void cacheEvent(const char* event, bool isLocked, bool isAjar, const char* uid, const String& timestamp); // saves an event to the cache
bool hasCachedEvents();  // checks if any events are cached, used to see if we need to resync
String getCachedEvents();  // returns JSON array string
void clearEventCache();  // clears event cache
int getCachedEventCount();  // returns number of cached events, mainly for debugging/logging