#ifndef MEMCARD_H
#define MEMCARD_H

#include <Arduino.h>
#include "sio.h"
#include "optiboot.h"

// On the 328p, pages are 128 bytes, which matches the frame size of a memory card.
// Mega 2560 pages are 256 bytes, needs changes
#define NUMBER_OF_BLOCKS 3
// Define the number of pages you want to write to here (limited by flash size)
#define NUMBER_OF_PAGES NUMBER_OF_BLOCKS * 64

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
    None = 0x00,   // No command, idle state
    Error = 0xFF   // Bad command
};

enum MC_Responses : byte
{
    Idle_High = 0xFF,   // High default state
    Dummy = 0x00,       // Filler Data
    ID1 = 0x5A,         // Memory Card ID1
    ID2 = 0x5D,         // Memory Card ID2
    CmdAck1 = 0x5C,     // Command Acknowledge 1
    CmdAck2 = 0x5D,     // Command Acknowledge 2
    GoodRW = 0x47,      // Good Read/Write
    BadChecksum = 0x4E, // Bad Checksum during Write
    BadSector = 0xFF    // Bad Memory Card Sector
};

extern const uint8_t FlashData[SPM_PAGESIZE * NUMBER_OF_PAGES] __attribute__((aligned(SPM_PAGESIZE))) PROGMEM;
extern byte MC_FLAG;

extern uint16_t MC_Sector;
extern bool MC_SendAck;

void MC_CommitUncommited();
void MC_GoIdle();
byte MC_ProcessMemCardEvents(byte);
byte MC_ReadCmnd_Tick(byte &);  //PSX 52h
byte MC_WriteCmnd_Tick(byte &); //PSX 57h
void MC_CommitWrite();

#endif //MC1DATA_H