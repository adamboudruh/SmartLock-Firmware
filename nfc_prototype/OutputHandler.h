#ifndef OUTPUT_HANDLER_H
#define OUTPUT_HANDLER_H

#pragma once

extern bool isLocked; // true = locked, false = unlocked

extern bool isAjar; // true = door ajar, false = door closed

void lockDoor();
void unlockDoor();
#endif