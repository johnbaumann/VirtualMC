#include "pad.h"

uint8_t PAD_Cmnd_Ticks;
byte PAD_Cur_Cmnd = PAD_Commands::_None;

uint16_t PAD_DigitalSwitches = 0xFFFF;
uint16_t PAD_Analog1 = 0xFFFF;
uint16_t PAD_Analog2 = 0xFFFF;

bool PAD_SendAck = true;

void PAD_GoIdle()
{
    PAD_Cmnd_Ticks = 0;
    PAD_SendAck = true;
}

byte PAD_ProcessEvents(byte DataIn)
{
    byte DataOut = 0x69;

    switch (PAD_Cmnd_Ticks)
    {
    case 0:
        DataOut = lowByte(PAD_Types::Digital_Pad);
        break;

    case 1:
        DataOut = highByte(PAD_Types::Digital_Pad);
        break;

    case 2:
        DataOut = lowByte(PAD_DigitalSwitches);
        break;

    case 3:
        DataOut = highByte(PAD_DigitalSwitches);
        PAD_GoIdle();
        break;

    default:
        DataOut = 0xFF;
        PAD_GoIdle();
        break;
    }

    PAD_Cmnd_Ticks++;

    return DataOut;
}