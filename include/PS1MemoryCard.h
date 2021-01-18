#ifndef PS1MEMORYCARD_H
#define PS1MEMORYCARD_H

#include <Arduino.h>

// Implement actual bit manipulation later
// Need more info on FLAG bits.
enum MC_Flags : byte
{
    Directory_Unread = 0x08, // Initial power on value
    Directory_Read = 0x00    // Apparently cleared after good MC Write
                             // (test write sector 3F = 0x1F80) offset
};

enum MC_Commands : byte
{
    Access = 0x81,
    Read = 0x52,
    Get_ID = 0x53,
    Write = 0x57,
    None = 0x00
};

enum MC_Responses : byte
{
    Idle_High = 0xFF,
    Dummy = 0x00,
    ID1 = 0x5A,
    ID2 = 0x5D,
    CmdAck1 = 0x5C,
    CmdAck2 = 0x5D,
    GoodRW = 0x47,
    BadChecksum = 0x4E,
    BadSector = 0xFF
};

class PS1MemoryCard
{
private:
    byte Cur_Cmnd;
    uint8_t Cmnd_Ticks;

    byte FLAG;
    uint16_t MC_Sector;
    byte Sector_Offset;

    bool bSendAck;
    bool bDeviceActive;

    byte ReadCmnd_Tick(byte&);

public:
    PS1MemoryCard();
    void SlaveSelectLow();
    byte Process(byte);
    bool SendAck();
    bool IsActiveDevice();
};

#endif