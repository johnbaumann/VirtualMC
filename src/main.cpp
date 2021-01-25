#include <Arduino.h>
#include <SPI.h>
#include <avr/pgmspace.h> // Allows reading memory from program storage space.

#include "digitalWriteFast.h" //https://code.google.com/archive/p/digitalwritefast/
#include "main.h"
#include "MC1Data.h"

//Written for the 328p running 3.3V @ 8MHz
//Should work on other chips with native SPI so long as 3.3V logic level
//ACK     2   (PS1: ACK)
//SS      10  (PS1: ATT)
//MOSI    11  (PS1: CMND)
//MISO    12  (PS1: Data)

//byte SerialOut[16];

byte CurrentSPICommand = PS1_SPICommands::Idle;
byte Cur_Cmnd;
uint8_t Cmnd_Ticks;

byte FLAG;
uint16_t MC_Sector;
byte Sector_Offset;
byte Checksum_Out;
byte Checksum_In;

bool bSendAck;
bool bPCLinkActive = false;

byte SerialIn = 0x00;
byte SerialOut = 0xFF;
byte SerialChecksum = 0x00;

byte inline ReadCmnd_Tick(byte &);  //PSX 52h
byte inline WriteCmnd_Tick(byte &); //PSX 57h

void GoIdle();

void ReadFrame(uint16_t);  //Serial
void WriteFrame(uint16_t); //Serial

void setup(void)
{
  byte clr = 0;

  // Set MISO and ACK as output, the rest as input.
  pinModeFast(SCK, INPUT);
  pinModeFast(SS, INPUT);
  pinModeFast(MOSI, INPUT);
  pinModeFast(MISO, OUTPUT);
  pinModeFast(ACK, OUTPUT);

  // ACK high on idle
  digitalWriteFast(ACK, HIGH);

  Serial.begin(38400);

  // Set bits in the SPCR register
  SPCR = 0x6F;

  //Clear SPSR and SPDR by reading their values
  clr = SPDR;
  clr = SPSR;

  clr = clr; // Supress clr not used warning. We're using it to clear flag registers above.

  SPDR = 0xFF; // Keep Slave out high
  FLAG = MC_Flags::Directory_Unread;
  GoIdle();
}

void inline SEND_ACK()
{
  //delayMicroseconds(7);
  digitalWriteFast(ACK, LOW);
  delayMicroseconds(4);
  digitalWriteFast(ACK, HIGH);
}

void inline DISABLE_SPI()
{
  SPCR &= ~_BV(SPE); //Disable SPI
  pinModeFast(MISO, INPUT);
  pinModeFast(ACK, INPUT);
  return;
}

void inline ENABLE_SPI()
{
  SPCR |= _BV(SPE); //Enable SPI
  pinModeFast(MISO, OUTPUT);
  pinModeFast(ACK, OUTPUT);
}

void GoIdle()
{
  Cur_Cmnd = MC_Commands::None;
  MC_Sector = 0x0000;
  Cmnd_Ticks = 0;
  bSendAck = true;
  Sector_Offset = 0;
}

void ReadFrame(uint16_t Address)
{
  SerialChecksum = (Address >> 8) ^ (Address & 0xFF);

  for (byte Sector_Offset = 0; Sector_Offset < 128; Sector_Offset++)
  {
    SerialOut = pgm_read_byte_near((FakeData + (Address * (uint16_t)128) + Sector_Offset));
    SerialChecksum ^= SerialOut;
    Serial.write(SerialOut);
  }

  Serial.write(SerialChecksum); //Checksum (MSB xor LSB xor Data)
  Serial.write(MC_Responses::GoodRW); //Memory Card status byte*/
  bPCLinkActive = false;
}

byte inline ReadCmnd_Tick(byte &DataIn)
{
  byte DataOut;

  bSendAck = true; // Default true;

  switch (Cmnd_Ticks)
  {
    //Data is sent and received simultaneously,
    //so the data we send isn't received by the system
    //until the next bytes are exchanged. In this way,
    //you have to basically respond one byte earlier than
    //actually occurs between Slave and Master.
    //Offset "Send" bytes noted from nocash's psx specs.

  case 0: //52h
    DataOut = MC_Responses::ID1;
    break;

  case 1: //00h
    DataOut = MC_Responses::ID2;
    break;

  case 2: //00h
    DataOut = MC_Responses::Dummy;
    break;

  case 3: //MSB
    //Store upper 8 bits of sector
    MC_Sector = (DataIn << 8);
    //Reply with (pre)
    DataOut = DataIn;
    break;

  case 4: //LSB
    //Store lower 8 bits of sector
    MC_Sector |= DataIn;
    DataOut = MC_Responses::CmdAck1;
    break;

  case 5: //00h
    DataOut = MC_Responses::CmdAck2;
    break;

  case 6: //00h
    //Confirm MSB
    DataOut = MC_Sector >> 8;
    break;

  case 7: //00h
    //Confirm LSB
    DataOut = (MC_Sector & 0xFF);
    Checksum_Out = (MC_Sector >> 8) ^ (MC_Sector & 0xFF);
    break;

    // Cases 8 through 135 overloaded to default operator below

  case 136:
    DataOut = Checksum_Out;
    break;

  case 137:
    DataOut = MC_Responses::GoodRW;
    bSendAck = false;
    break;

  default:
    if (Cmnd_Ticks >= 8 && Cmnd_Ticks <= 135) //Stay here for 128 bytes
    {
      DataOut = pgm_read_byte_near((FakeData + (MC_Sector * (uint16_t)128) + Sector_Offset));
      Checksum_Out ^= DataOut;
      Sector_Offset++;
    }
    else
    {
      DataOut = MC_Responses::Idle_High;
      GoIdle();
    }
    break;
  }

  Cmnd_Ticks++;

  return DataOut;
}

byte inline WriteCmnd_Tick(byte &DataIn)
{
  byte DataOut;

  bSendAck = true; // Default true;

  switch (Cmnd_Ticks)
  {
    //Data is sent and received simultaneously,
    //so the data we send isn't received by the system
    //until the next bytes are exchanged. In this way,
    //you have to basically respond one byte earlier than
    //actually occurs between Slave and Master.
    //Offset "Send" bytes noted from nocash's psx specs.

  case 0: //52h
    DataOut = MC_Responses::ID1;
    break;

  case 1: //00h
    DataOut = MC_Responses::ID2;
    break;

  case 2: //00h
    DataOut = MC_Responses::Dummy;
    break;

  case 3: //MSB
    //Store upper 8 bits of sector
    MC_Sector = (DataIn << 8);
    //Reply with (pre)
    DataOut = DataIn;
    break;

  case 4: //LSB
    //Store lower 8 bits of sector
    MC_Sector |= DataIn;
    Checksum_Out = (MC_Sector >> 8) ^ (MC_Sector & 0xFF);
    DataOut = DataIn;

    break;

  default:
    if (Cmnd_Ticks >= 5 && Cmnd_Ticks <= 132)
    {
      //To-do: Store data
      //Calculate checksum
      Checksum_Out ^= DataIn;
      //Reply with (pre)
      DataOut = DataIn;
    }
    else if (Cmnd_Ticks == 133) //CHK
    {
      Checksum_In = DataIn;
      DataOut = MC_Responses::CmdAck1;
    }
    else if (Cmnd_Ticks == 134) //00h
    {
      DataOut = MC_Responses::CmdAck2;
    }
    else if (Cmnd_Ticks == 135) //00h
    {
      if (Checksum_In == Checksum_Out)
      {
        FLAG = MC_Flags::Directory_Read;
        DataOut = MC_Responses::GoodRW;
      }
      else
      {
        DataOut = MC_Responses::BadChecksum;
      }

      GoIdle();
    }
    else
    {
      GoIdle();
    }

    break;
  }

  Cmnd_Ticks++;

  return DataOut;
}

bool inline SPI_Data_Ready()
{
  // Return whether SPIF bit is set,
  // indicating the transmission is complete.
  return (SPSR & (1 << SPIF));
}

void loop()
{
  byte DataIn = 0x00;
  byte DataOut = 0xFF;
  bool bTempAck = false;
  bool cmdRouted = false;

  while (1)
  {

    //if SS high, reset status
    //else, poll for spi data
    //Switch satement program status, pipe code out accordingly
    //Process incoming
    //Push response to SPDR

    if (digitalReadFast(SS) == HIGH)
    {
      if (CurrentSPICommand != PS1_SPICommands::Idle)
      {
        CurrentSPICommand = PS1_SPICommands::Idle; // Clear last command
        GoIdle();                                  // Reset Memory Card State
        ENABLE_SPI();
      }
      while (Serial.available() > 0)
      {
        bPCLinkActive = true;
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
          //WriteFrame(Serial.read() | Serial.read() << 8);
          break;
        }
      }
    }
    else
    {
      if (CurrentSPICommand != PS1_SPICommands::Ignore)
      {

        if (SPI_Data_Ready())
        {
          noInterrupts();

          DataIn = SPDR;
          bTempAck = bSendAck;

          if (CurrentSPICommand == PS1_SPICommands::Idle)
            CurrentSPICommand = DataIn;

          switch (CurrentSPICommand)
          {

          case PS1_SPICommands::MC_Access:
            cmdRouted = false;
            while (!cmdRouted)
            {
              switch (Cur_Cmnd)
              {
              case MC_Commands::None:
                Cur_Cmnd = DataIn;
                DataOut = FLAG;
                cmdRouted = true;
                break;

              case MC_Commands::Access:
                if (DataIn == MC_Commands::Read || DataIn == MC_Commands::Write || DataIn == MC_Commands::Get_ID)
                {
                  //TODO: Possible to avoid code duplication around here
                  //NOTE: Same first 2 replies for all commands: Card ID1 + ID2
                  //Read and Write share the 2 commands after, MSB+LSB receive
                  //Get_ID not a required command
                  Cur_Cmnd = DataIn;
                  cmdRouted = false;
                }
                break;

              case MC_Commands::Read:
                DataOut = ReadCmnd_Tick(DataIn);
                cmdRouted = true;
                break;

              case MC_Commands::Write:
                DataOut = WriteCmnd_Tick(DataIn);
                cmdRouted = true;
                break;

              default:
                DataOut = MC_Responses::Idle_High;
                GoIdle();
                cmdRouted = true;
                break;
              }
            }
            break;

          // Ignore pad, cascade to default ignore behavior
          case PS1_SPICommands::PAD_Access:
          default:          // Bad/Unexpected/Unsupported slave select command
            DataOut = 0xFF; // Ignore
            CurrentSPICommand = PS1_SPICommands::Ignore;
            DISABLE_SPI();
          }

          SPDR = DataOut;
          if (bTempAck)
            SEND_ACK();
          interrupts();
        }
      }
    }
  }
}