#include "pad.h"

byte PAD_Cur_Cmnd;
uint8_t PAD_Cmnd_Ticks;

uint16_t PAD_DigitalSwitches = 0xFFFF;
uint16_t PAD_Analog1 = 0xFFFF;
uint16_t PAD_Analog2 = 0xFFFF;
uint16_t PAD_MOT = 0x0000;
uint8_t PAD_TAP = 0x00;

bool PAD_SendAck = true;

void PAD_GoIdle()
{
    PAD_Cur_Cmnd = PAD_Commands::_None;
    PAD_Cmnd_Ticks = 0;
    PAD_SendAck = true;
    uint8_t PAD_TAP = 0x00;
}

byte PAD_ProcessEvents(byte DataIn)
{
    byte DataOut = 0xFF;
    bool cmdRouted = false;

    while (!cmdRouted)
    {
        switch (PAD_Cur_Cmnd)
        {
        // No command yet
        case PAD_Commands::_None:
            // Store incoming byte as command
            PAD_Cur_Cmnd = DataIn;
            // Store low byte of Pad type
            DataOut = lowByte(PAD_Types::Digital_Pad);
            // Safe to exit interpret loop
            cmdRouted = true;
            break;

        case PAD_Commands::_Access:
            if (DataIn == PAD_Commands::_Read)
            {
                PAD_Cur_Cmnd = DataIn;
                // Re-evaluate command
                // cmdRouted = false;
            }
            else
            {
                PAD_Cur_Cmnd = PAD_Commands::_Error;
                // Re-evaluate command
                // cmdRouted = false;
            }
            break;

        case PAD_Commands::_Read:
            DataOut = PAD_ReadCmnd_Tick(DataIn);
            cmdRouted = true;
            break;

        case PAD_Commands::_Error:
            // Need data on PAD responses to invalid commands
            DataOut = 0xFF;
            PAD_GoIdle();
            cmdRouted = true;
            break;
        }
    }

    return DataOut;
}

byte PAD_ReadCmnd_Tick(byte DataIn)
{
    byte DataOut;

    PAD_SendAck = true; // Default true;

    switch (PAD_Cmnd_Ticks)
    {
        //Data is sent and received simultaneously,
        //so the data we send isn't received by the system
        //until the next bytes are exchanged. In this way,
        //you have to basically respond one byte earlier than
        //actually occurs between Slave and Master.
        //Offset "Send" bytes noted from nocash's psx specs.

    case 0: //42h
        //idhi
        DataOut = highByte(PAD_Types::Digital_Pad);
        //DataOut = 0x18;
        break;

    case 1: //TAP
        PAD_TAP = DataIn;
        DataOut = lowByte(PAD_DigitalSwitches);
        break;

    case 2: //MOT
        PAD_MOT = (DataIn << 8);
        DataOut = highByte(PAD_DigitalSwitches);
        break;

    case 3: //MOT
        PAD_MOT |= DataIn;
        DataOut = 0xFC;
        //PAD_GoIdle();
        PAD_SendAck = false;
        break;

    default:
        DataOut = 0xFD;
        PAD_GoIdle();
        break;
    }

    PAD_Cmnd_Ticks++;

    return DataOut;
}