 #include "sio.h"

byte CurrentSIOCommand = PS1_SIOCommands::Idle;

uint16_t SIO_TimeoutTicks = 0;