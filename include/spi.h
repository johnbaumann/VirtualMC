#ifndef SPI_H
#define SPI_H

#include <Arduino.h>
#include "digitalWriteFast.h"

void SPI_Active();
void SPI_Passive();

bool SPI_Data_Ready();

void SPI_Disable();
void SPI_Enable();
void SPI_Init();

void SEND_ACK();

static const uint8_t ACK = 2;

#endif // SPI_H