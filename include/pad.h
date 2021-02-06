#ifndef PAD_H
#define PAD_H

#include <Arduino.h>

enum PAD_Commands : byte
{
    _Access = 0x01, // Pad Select
    _Read = 0x42,   // Read Command
    _None = 0x00    // No command, idle state
};

// https://problemkaputt.de/psx-spx.htm#controllersandmemorycards
enum PAD_Types : uint16_t
{
    Mouse = 0x5A12,           // (two button mouse)
    NegCon = 0x5A23,          // (steering twist/wheel/paddle)
    Konami_Lightgun = 0x5A31, // (IRQ10-type)
    Digital_Pad = 0x5A41,     // (or analog pad/stick in digital mode; LED=Off)
    Analog_Stick = 0x5A53,    // (or analog pad in "flight mode"; LED=Green)
    Namco_Lightgun = 0x5A63,  // (Cinch-type)
    Analog_Pad = 0x5A73,      // (in normal analog mode; LED=Red)
    Multitap = 0x5A80,        // (multiplayer adaptor) (when activated)
    Jogcon = 0x5AE3,          // (steering dial)
    Config_Mode = 0x5AF3,     // (when in config mode; see rumble command 43h)
    High_Z = 0xFFFF           // (no controller connected, pins floating High-Z)
};

extern bool PAD_SendAck;
extern uint16_t PAD_DigitalSwitches;
extern uint16_t PAD_Analog1;
extern uint16_t PAD_Analog2;

void PAD_GoIdle();
byte PAD_ProcessEvents(byte);
byte PAD_ReadPadTicks(byte);

#endif