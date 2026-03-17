#pragma once
#include <Arduino.h>

// Shared pending command flags
extern volatile bool pendingLock;
extern volatile bool pendingUnlock;