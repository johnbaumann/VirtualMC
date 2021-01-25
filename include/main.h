#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>

// Implement actual bit manipulation later
// Need more info on FLAG bits.
enum MC_Flags : byte
{
    Directory_Unread = 0x08, // Initial power on value
    Directory_Read = 0x00    // Cleared after good MC Write
                             // (test write sector 3F = 0x1F80) offset
};

enum MC_Commands : byte
{
    Access = 0x81, // Memory Card Select
    Read = 0x52,   // Read Command
    Get_ID = 0x53, // Get ID Command
    Write = 0x57,  // Write Command
    None = 0x00    // No command, idle state
};

enum MC_Responses : byte
{
    Idle_High = 0xFF,   // Not fully tested, but Slave Out seems to idle high
    Dummy = 0x00,       // Filler Data
    ID1 = 0x5A,         // Memory Card ID1
    ID2 = 0x5D,         // Memory Card ID2
    CmdAck1 = 0x5C,     // Command Acknowledge 1
    CmdAck2 = 0x5D,     // Command Acknowledge 2
    GoodRW = 0x47,      // Good Read/Write
    BadChecksum = 0x4E, // Bad Checksum during Write
    BadSector = 0xFF    // Bad Memory Card Sector
};

enum PS1_SPICommands : byte
{
    Idle = 0x00,
    PAD_Access = 0x01,
    MC_Access = 0x81,
    Ignore = 0xFF
};

static const uint8_t ACK = 2;

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

#endif