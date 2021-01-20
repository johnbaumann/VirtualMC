#ifndef PS1MEMORYCARD_H
#define PS1MEMORYCARD_H

#include <Arduino.h>
#include <avr/pgmspace.h> // Allows reading memory from program storage space.

#include "MC1Data.h"

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

class PS1MemoryCard
{
private:
    byte Cur_Cmnd;
    uint8_t Cmnd_Ticks;

    byte FLAG;
    uint16_t MC_Sector;
    byte Sector_Offset;

    byte Checksum_Out;
    byte Checksum_In;

    bool bSendAck;

    byte ReadCmnd_Tick(byte &);  //52h
    byte WriteCmnd_Tick(byte &); //57h
    byte GetIDCmnd_Tick(byte &); //53h

public:
    PS1MemoryCard();
    void GoIdle();
    byte Process(byte);
    bool SendAck();
};

#endif