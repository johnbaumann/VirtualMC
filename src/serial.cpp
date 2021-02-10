#include "serial.h"

byte Serial_In = 0x00;
byte Serial_Out = 0xFF;
byte Serial_Checksum = 0x00;
uint16_t Serial_ActiveTicks = 0;
uint16_t Serial_IdleTicks = 0;
bool bSerialBusy = false;
bool RTS_Status = false;

byte Serial_Cur_Cmnd = Serial_Commands::SER_None;
byte Serial_Last_Cmnd = Serial_Commands::SER_None;
byte Serial_Cmnd_Ticks = 0;

uint16_t Temp_PAD_DigitalSwitches = 0xFFFF;
uint16_t Temp_PAD_Analog1 = 0xFFFF;
uint16_t Temp_PAD_Analog2 = 0xFFFF;

void Serial_ReceivePAD()
{
    switch (Serial_Cmnd_Ticks)
    {
    case 1:
        Temp_PAD_DigitalSwitches = (Serial.read() << 8);
        break;

    case 2:
        Temp_PAD_DigitalSwitches |= Serial.read();
        PAD_DigitalSwitches = Temp_PAD_DigitalSwitches;
        break;

    case 3:
        Temp_PAD_Analog1 = Serial.read() << 8 | 0xFF;
        break;

    case 4:
        Temp_PAD_Analog1 |= Serial.read();
        PAD_Analog1 = Temp_PAD_Analog1;
        break;

    case 5:
        Temp_PAD_Analog2 = Serial.read() << 8 | 0xFF;
        break;

    case 6:
        Temp_PAD_Analog2 |= Serial.read();
        PAD_Analog2 = Temp_PAD_Analog2;
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
    pinModeFast(CTS_Pin, INPUT);
    pinModeFast(RTS_Pin, OUTPUT);
}

void Serial_ReadFrame(unsigned int Address)
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
    Serial_GoIdle();
}

//Write a frame from the serial port to the Memory Card
void Serial_WriteFrame(unsigned int Address)
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
                    if (!bPadEnabled)
                    {
                        bPadEnabled = true;
                    }
                    Serial.write(OKAY);
                    cmdRouted = true;
                    Serial_GoIdle();
                    break;

                case Serial_Commands::SER_PAD_Off:
                    if (bPadEnabled)
                    {
                        bPadEnabled = false;
                    }
                    Serial.write(OKAY);
                    cmdRouted = true;
                    Serial_GoIdle();
                    break;

                case Serial_Commands::SER_MC_On:
                    if (!bMemCardEnabled)
                    {
                        MC_FLAG = MC_Flags::Directory_Unread;
                        bMemCardEnabled = true;
                    }
                    Serial.write(OKAY);
                    cmdRouted = true;
                    Serial_GoIdle();
                    break;

                case Serial_Commands::SER_MC_Off:
                    if (bMemCardEnabled)
                    {
                        bMemCardEnabled = false;
                        MC_GoIdle();
                    }
                    Serial.write(OKAY);
                    cmdRouted = true;
                    Serial_GoIdle();
                    break;

                default:
                    Serial.write(ERROR);
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

    if(Serial_ActiveTicks < SERIALMAXACTIVETICKS)
    {
        Serial_ActiveTicks++;
    }
    else if(!Serial_Busy() && RTS_Status == true)
    {
        RTS_Status = false;
        digitalWriteFast(RTS_Pin, HIGH);
    }
}