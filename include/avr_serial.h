#ifndef AVR_SERIAL_H
#define AVR_SERIAL_H

#include <stdint.h>

//Memcarduinoplus
//Device Firmware identifier
#define IDENTIFIER "MCDINO" //MemCARDuinoPlus
#define VERSION 0x05        //Firmware version byte (Major.Minor)

//Responses
#define ERROR 0xE0 //Invalid command received (error)
#define OKAY 0x0E  //General command acknowledge

#define SERIALTIMEOUTTICKS 0x25C6   //Aiming for about 120ms delay
#define SERIALMAXACTIVETICKS 0x0218 //Aiming for about 6440us active

namespace VirtualMC
{
    namespace avr
    {
        enum Serial_Commands : uint8_t
        {
            SER_None = 0x00,
            SER_Get_ID_ = 0xA0,
            SER_Get_Version = 0xA1,
            SER_MC_Read = 0xA2,
            SER_MC_Write = 0xA3,
            SER_PAD_On = 0xB0,
            SER_PAD_Off = 0xB1,
            SER_MC_On = 0xB2,
            SER_MC_Off = 0xB3,    //(>>OKAY)
            SER_PAD_SetAll = 0xC0 //( <<6 bytes )
        };

        extern uint16_t Serial_ActiveTicks;
        extern uint16_t Serial_IdleTicks;
        extern bool RTS_Status;

        static const uint8_t RTS_Pin = 2;
        static const uint8_t CTS_Pin = 3;

        bool Serial_Busy();
        void Serial_GoIdle();
        void Serial_Init();
        void Serial_ProcessEvents();
        void Serial_ReadFrame(unsigned int); //Serial
        void Serial_Reset();
        void Serial_WriteFrame(unsigned int); //Serial
    }                                         // namespace avr
} // namespace VirtualMC

#endif //AVR_SERIAL_H