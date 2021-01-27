#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <SPI.h>
#include <avr/pgmspace.h> // Allows reading memory from program storage space.

#include "digitalWriteFast.h" //https://code.google.com/archive/p/digitalwritefast/
#include "memcard.h"
#include "optiboot.h"
#include "pccomms.h"
#include "sio.h"


static const uint8_t ACK = 2;

#endif