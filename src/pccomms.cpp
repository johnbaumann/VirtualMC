#include "pccomms.h"

byte Serial_In = 0x00;
byte Serial_Out = 0xFF;
byte Serial_Checksum = 0x00;
uint16_t Serial_IdleTicks = 0;

void ReadFrame(unsigned int Address)
{
    uint16_t MC_Sector = (Address >> 8) | (Address << 8);
    byte Serial_Checksum = (Address >> 8) ^ (Address & 0xFF);
    byte Serial_Out = 0x00;

    //optiboot_readPage(FlashData, ramBuffer, Address+1);

    for (int Sector_Offset = 0; Sector_Offset < SPM_PAGESIZE; Sector_Offset++)
    {
        if (MC_Sector + 1 > NUMBER_OF_PAGES)
        {
            Serial_Out = 0xFF;
        }
        else
        {
            Serial_Out = pgm_read_byte_near((FlashData + (MC_Sector * (uint16_t)128) + Sector_Offset));
        }

        //SerialOut = ramBuffer[Sector_Offset];
        Serial_Checksum ^= Serial_Out;
        Serial.write(Serial_Out);
    }

    Serial.write(Serial_Checksum);      //Checksum (MSB xor LSB xor Data)
    Serial.write(MC_Responses::GoodRW); //Memory Card status byte*/
    MC_FLAG = MC_Flags::Directory_Unread;
}

//Write a frame from the serial port to the Memory Card
void WriteFrame(unsigned int Address)
{
    uint16_t MC_Sector = (Address >> 8) | (Address << 8);
    byte Serial_Checksum = (Address >> 8) ^ (Address & 0xFF);
    byte MC_Checksum_In = 0xFF;

    uint8_t MC_DataBuffer[SPM_PAGESIZE];

    //Copy 128 bytes from the serial input
    for (int i = 0; i < 128; i++)
    {
        while (!Serial.available())
            ;

        MC_DataBuffer[i] = Serial.read();
        Serial_Checksum ^= MC_DataBuffer[i];
    }

    while (!Serial.available())
        ;

    MC_Checksum_In = Serial.read(); //Checksum (MSB xor LSB xor Data)

    if (MC_Sector < 0 || MC_Sector > 1024)
    {
        Serial.write(MC_Responses::BadSector);
    }
    else if (Serial_Checksum == MC_Checksum_In)
    {
        if (MC_Sector + 1 <= NUMBER_OF_PAGES)
        {
            optiboot_writePage(FlashData, MC_DataBuffer, MC_Sector + 1);
            MC_FLAG = MC_Flags::Directory_Unread;
        }
        Serial.write(MC_Responses::GoodRW); //Memory Card status byte
                                            //Write 128 byte data to the frame
    }
    else
    {
        Serial.write(MC_Responses::BadChecksum);
    }
}

// Directly adapted from Memcarduino by ShendoXT
// https://github.com/ShendoXT/memcarduino
void ProcessSerialEvents()
{
    while (Serial.available() > 0)
    {
        Serial_IdleTicks = 0;

        //bPCLinkActive = true;
        Serial_In = Serial.read();

        switch (Serial_In)
        {
        default:
            Serial.write(ERROR);
            break;

        case GETID:
            Serial.write(IDENTIFIER);
            break;

        case GETVER:
            Serial.write(VERSION);
            break;

        case MCREAD:
            while (Serial.available() < 2)
                ;
            delay(5);
            ReadFrame(Serial.read() | Serial.read() << 8);
            break;

        case MCWRITE:
            while (Serial.available() < 2)
                ;
            delay(5);
            WriteFrame(Serial.read() | Serial.read() << 8);
            break;
        }
    }

    if(Serial_IdleTicks < 0xFFFF)
        Serial_IdleTicks++;
}