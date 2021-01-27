#ifndef SIO_H
#define SIO_H

#include <Arduino.h>


enum PS1_SIOCommands : byte
{
    Idle = 0x00,
    PAD_Access = 0x01,
    MC_Access = 0x81,
    Ignore = 0xFF
};

extern byte CurrentSIOCommand;

#endif