#ifndef TESTDATA_H
#define TESTDATA_H

#include <Arduino.h>
#include <stdint.h>

// On the 328p, pages are 128 bytes, which matches the frame size of a memory card.
// Mega 2560 pages are 256 bytes, needs changes
#define NUMBER_OF_BLOCKS 3
// Define the number of pages you want to write to here (limited by flash size)
#define NUMBER_OF_PAGES NUMBER_OF_BLOCKS * 64

extern const uint8_t FlashData[];

#endif