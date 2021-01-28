#ifndef PCCOMMS_H
#define PCCOMMS_H

#include <Arduino.h>
#include "memcard.h"
#include "optiboot.h"

//Memcarduinoplus
//Device Firmware identifier
#define IDENTIFIER "VIRMCD" //MemCARDuinoPlus
#define VERSION 0x05        //Firmware version byte (Major.Minor)

//Commands
#define GETID 0xA0   //Get identifier
#define GETVER 0xA1  //Get firmware version
#define MCREAD 0xA2  //Memory Card Read (frame)
#define MCWRITE 0xA3 //Memory Card Write (frame)

//Responses
#define ERROR 0xE0 //Invalid command received (error)

#define SERIALTIMEOUTTICKS  0xFFFF

extern uint16_t Serial_IdleTicks;

void ProcessSerialEvents();
void ReadFrame(unsigned int);  //Serial
void WriteFrame(unsigned int); //Serial



#endif