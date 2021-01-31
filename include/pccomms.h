#ifndef PCCOMMS_H
#define PCCOMMS_H

#include <Arduino.h>
#include "memcard.h"
#include "optiboot.h"
#include "sio.h"

//Memcarduinoplus
//Device Firmware identifier
#define IDENTIFIER "MCDINO" //MemCARDuinoPlus
#define VERSION 0x05        //Firmware version byte (Major.Minor)

//Commands
#define GETID 0xA0   //Get identifier
#define GETVER 0xA1  //Get firmware version
#define MCREAD 0xA2  //Memory Card Read (frame)
#define MCWRITE 0xA3 //Memory Card Write (frame)

//New commands
#define VMCPADON 0xB0   //Turn Pad On
#define VMCPADOFF 0xB1  //Turn Pad Off
#define VMCCARDON 0xB2  //Turn Card On
#define VMCCARDOFF 0xB3 //Turn Card Off

//Responses
#define ERROR 0xE0 //Invalid command received (error)
#define OKAY 0x0E  //General command acknowledge

#define SERIALTIMEOUTTICKS 0xFFFF

extern uint16_t Serial_IdleTicks;

void Serial_ProcessEvents();
void ReadFrame(unsigned int);  //Serial
void WriteFrame(unsigned int); //Serial

#endif