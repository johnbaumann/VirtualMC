#include "avr_serial.h"

#include <Arduino.h>

#include "avr_digitalWriteFast.h"
//#include "avr_flashdata.h"
#include "avr_optiboot.h"
#include "sio.h"
#include "sio_memory_card.h"
#include "sio_controller.h"

namespace VirtualMC
{
    namespace avr
    {

        uint8_t Serial_In = 0x00;
        uint8_t Serial_Out = 0xFF;
        uint8_t Serial_Checksum = 0x00;
        uint16_t Serial_ActiveTicks = 0;
        uint16_t Serial_IdleTicks = SERIALTIMEOUTTICKS;
        bool bSerialBusy = false;

        uint16_t Sector = 0x0000;
        uint8_t Serial_Cur_Cmnd = Serial_Commands::SER_None;
        uint8_t Serial_Last_Cmnd = Serial_Commands::SER_None;
        uint8_t Serial_Cmnd_Ticks = 0;

        uint16_t Temp_PAD_DigitalSwitches = 0xFFFF;
        uint16_t Temp_PAD_Analog1 = 0xFFFF;
        uint16_t Temp_PAD_Analog2 = 0xFFFF;

        unsigned long write_start_time;
        unsigned long write_end_time;

        void Serial_ReceivePAD()
        {
            switch (Serial_Cmnd_Ticks)
            {
            case 1:
                Temp_PAD_DigitalSwitches = (Serial.read() << 8);
                break;

            case 2:
                Temp_PAD_DigitalSwitches |= Serial.read();
                break;

            case 3:
                Temp_PAD_Analog1 = Serial.read() << 8 | 0xFF;
                break;

            case 4:
                Temp_PAD_Analog1 |= Serial.read();
                break;

            case 5:
                Temp_PAD_Analog2 = Serial.read() << 8 | 0xFF;
                break;

            case 6:
                Temp_PAD_Analog2 |= Serial.read();
                sio::controller::Analog1 = Temp_PAD_Analog1;
                sio::controller::Analog2 = Temp_PAD_Analog2;
                sio::controller::DigitalSwitches = Temp_PAD_DigitalSwitches;
                Serial_GoIdle();
                break;
            }
        }

        bool Serial_Busy()
        {
            if (Serial_IdleTicks < SERIALTIMEOUTTICKS && (Serial_Last_Cmnd == Serial_Commands::SER_MC_Read || Serial_Last_Cmnd == Serial_Commands::SER_MC_Write))
            {
                bSerialBusy = true;
            }
            else
            {
                bSerialBusy = false;
            }

            return bSerialBusy;
        }

        void Serial_GoIdle()
        {
            Serial_Cur_Cmnd = Serial_Commands::SER_None;
            Serial_Cmnd_Ticks = 0;
        }

        void Serial_Init()
        {
            Serial.begin(38400);
            Serial_GoIdle();
        }

        void Serial_ReadFrame(unsigned int Address)
        {
            Sector = (Address >> 8) | (Address << 8);
            byte Serial_Checksum = (Address >> 8) ^ (Address & 0xFF);
            byte Serial_Out = 0x00;

            //optiboot_readPage(FlashData, ramBuffer, Address+1);

            if (sio::memory_card::myFile.position() != ((uint32_t)Sector * 128u))
                sio::memory_card::myFile.seek((uint32_t)Sector * 128u);

            for (int Sector_Offset = 0; Sector_Offset < 128; Sector_Offset++)
            {
                /*if (Sector + 1 > NUMBER_OF_PAGES)
                {
                    Serial_Out = 0xFF;
                }
                else
                {
                    Serial_Out = pgm_read_byte_near((FlashData + (Sector * (uint16_t)128) + Sector_Offset));
                }*/
                Serial_Out = sio::memory_card::myFile.read();

                //SerialOut = ramBuffer[Sector_Offset];
                Serial_Checksum ^= Serial_Out;
                Serial.write(Serial_Out);
            }

            Serial.write(Serial_Checksum);                             //Checksum (MSB xor LSB xor Data)
            Serial.write(sio::memory_card::Responses::kGoodReadWrite); //Memory Card status byte*/
            sio::memory_card::FLAG = sio::memory_card::Flags::kDirectoryUnread;
            Serial_GoIdle();
        }

        //Write a frame from the serial port to the Memory Card
        void Serial_WriteFrame(unsigned int Address)
        {
            Sector = (Address >> 8) | (Address << 8);
            byte Serial_Checksum = (Address >> 8) ^ (Address & 0xFF);
            byte MC_Checksum_In = 0xFF;

            uint8_t DataBuffer[SPM_PAGESIZE];

            //Copy 128 bytes from the serial input
            for (int i = 0; i < 128; i++)
            {
                while (!Serial.available())
                    ;

                DataBuffer[i] = Serial.read();
                Serial_Checksum ^= DataBuffer[i];
            }

            while (!Serial.available())
                ;

            MC_Checksum_In = Serial.read(); //Checksum (MSB xor LSB xor Data)

            if (Sector < 0 || Sector > 1024)
            {
                Serial.write(sio::memory_card::Responses::kBadSector);
            }
            else if (Serial_Checksum == MC_Checksum_In)
            {
                /*if (Sector + 1 <= NUMBER_OF_PAGES)
                {
                    write_start_time = micros();
                    if (SPM_PAGESIZE > 128)
                    {
                        for (uint16_t i = 0; i+128 < SPM_PAGESIZE; i++)
                        {
                            DataBuffer[i] = pgm_read_byte_near((FlashData + ((Sector) * 128) + i + 128));
                        }
                    }
                    //optiboot_writePage(FlashData, DataBuffer, Sector + 1);
                    write_end_time = micros();
                    sio::memory_card::FLAG = sio::memory_card::Flags::kDirectoryUnread;
                }*/
                Serial.write(sio::memory_card::Responses::kGoodReadWrite); //Memory Card status byte
                                                                           //Write 128 byte data to the frame
            }
            else
            {
                Serial.write(sio::memory_card::Responses::kBadChecksum);
            }

            Serial_GoIdle();
        }

        // Directly adapted from Memcarduino by ShendoXT
        // https://github.com/ShendoXT/memcarduino
        void Serial_ProcessEvents()
        {
            bool cmdRouted = false;
            int bytes_available = Serial.available();

            if (bytes_available > 0)
            {
                Serial_IdleTicks = 0;

                for (int i = 0; i < bytes_available; i++)
                {
                    cmdRouted = false;
                    while (!cmdRouted)
                    {
                        Serial_Last_Cmnd = Serial_Cur_Cmnd;
                        switch (Serial_Cur_Cmnd)
                        {

                        case Serial_Commands::SER_None:
                            Serial_Cur_Cmnd = Serial.read();
                            break;

                        case Serial_Commands::SER_PAD_SetAll:
                            if (Serial_Cmnd_Ticks > 0)
                            {
                                Serial_ReceivePAD();
                            }
                            cmdRouted = true;
                            break;

                        case Serial_Commands::SER_Get_ID_:
                            Serial.write(IDENTIFIER);
                            cmdRouted = true;
                            Serial_GoIdle();
                            break;

                        case Serial_Commands::SER_Get_Version:
                            Serial.write(VERSION);
                            cmdRouted = true;
                            Serial_GoIdle();
                            break;

                        case Serial_Commands::SER_MC_Read:
                            while (Serial.available() < 2)
                                ;
                            Serial_ReadFrame(Serial.read() | Serial.read() << 8);
                            cmdRouted = true;
                            break;

                        case Serial_Commands::SER_MC_Write:
                            while (Serial.available() < 2)
                                ;
                            Serial_WriteFrame(Serial.read() | Serial.read() << 8);
                            cmdRouted = true;
                            break;

                        case Serial_Commands::SER_PAD_On:
                            if (!sio::bPadEnabled)
                            {
                                sio::bPadEnabled = true;
                            }
                            Serial.write(OKAY);
                            cmdRouted = true;
                            Serial_GoIdle();
                            break;

                        case Serial_Commands::SER_PAD_Off:
                            if (sio::bPadEnabled)
                            {
                                sio::bPadEnabled = false;
                            }
                            Serial.write(OKAY);
                            cmdRouted = true;
                            Serial_GoIdle();
                            break;

                        case Serial_Commands::SER_MC_On:
                            sio::memory_card::Enable();
                            Serial.write(OKAY);
                            cmdRouted = true;
                            Serial_GoIdle();
                            break;

                        case Serial_Commands::SER_MC_Off:
                            sio::memory_card::Disable();
                            Serial.write(OKAY);
                            cmdRouted = true;
                            Serial_GoIdle();
                            break;

                        default:
                            //Serial.write(ERROR);
                            Serial.print("Last write: ");
                            Serial.print(write_end_time - write_start_time);
                            Serial.println(" micros");
                            cmdRouted = true;
                            Serial_GoIdle();
                            break;
                        }
                    }
                    if (Serial_Cur_Cmnd != Serial_Commands::SER_None)
                        Serial_Cmnd_Ticks++;
                }
            }
            else
            {
                if (Serial_IdleTicks < SERIALTIMEOUTTICKS && Serial_Cur_Cmnd == Serial_Commands::SER_None)
                    Serial_IdleTicks++;
            }
        }
    } // namespace avr
} // namespace VirtualMC