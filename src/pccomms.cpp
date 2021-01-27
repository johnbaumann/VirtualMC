#include "pccomms.h"

byte SerialIn = 0x00;
byte SerialOut = 0xFF;
byte SerialChecksum = 0x00;

void ReadFrame(unsigned int Address)
{
    uint16_t MC_Sector = (Address >> 8) | (Address << 8);
    byte SerialChecksum = (Address >> 8) ^ (Address & 0xFF);
    byte SerialOut = 0x00;

    //optiboot_readPage(FlashData, ramBuffer, Address+1);

    for (int Sector_Offset = 0; Sector_Offset < SPM_PAGESIZE; Sector_Offset++)
    {
        if (MC_Sector + 1 > NUMBER_OF_PAGES)
        {
            SerialOut = 0xFF;
        }
        else
        {
            SerialOut = pgm_read_byte_near((FlashData + (MC_Sector * (uint16_t)128) + Sector_Offset));
        }

        //SerialOut = ramBuffer[Sector_Offset];
        SerialChecksum ^= SerialOut;
        Serial.write(SerialOut);
    }

    Serial.write(SerialChecksum);       //Checksum (MSB xor LSB xor Data)
    Serial.write(MC_Responses::GoodRW); //Memory Card status byte*/
    MC_FLAG = MC_Flags::Directory_Unread;
}

//Write a frame from the serial port to the Memory Card
void WriteFrame(unsigned int Address)
{
    uint16_t MC_Sector = (Address >> 8) | (Address << 8);
    byte SerialChecksum = (Address >> 8) ^ (Address & 0xFF);
    byte MC_Checksum_In = 0xFF;

    uint8_t ReadData[SPM_PAGESIZE];

    //Copy 128 bytes from the serial input
    for (int i = 0; i < 128; i++)
    {
        while (!Serial.available())
            ;

        ReadData[i] = Serial.read();
        SerialChecksum ^= ReadData[i];
    }

    while (!Serial.available())
        ;

    MC_Checksum_In = Serial.read(); //Checksum (MSB xor LSB xor Data)

    if (MC_Sector < 0 || MC_Sector > 1024)
    {
        Serial.write(MC_Responses::BadSector);
    }
    else if (SerialChecksum == MC_Checksum_In)
    {
        if (MC_Sector + 1 <= NUMBER_OF_PAGES)
        {
            optiboot_writePage(FlashData, ReadData, MC_Sector + 1);
        }
        Serial.write(MC_Responses::GoodRW); //Memory Card status byte
                                            //Write 128 byte data to the frame
    }
    else
    {
        Serial.write(MC_Responses::BadChecksum);
    }
}

void ProcessSerialEvents()
{
    while (Serial.available() > 0)
    {
        //bPCLinkActive = true;
        SerialIn = Serial.read();

        switch (SerialIn)
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
}