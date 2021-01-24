#include <Arduino.h>
#include <SPI.h>

#include "digitalWriteFast.h" //https://code.google.com/archive/p/digitalwritefast/
#include "PS1MemoryCard.h"

void loop();
void setup(void);

byte SerialOut[16];

enum PS1_SPICommands : byte
{
  Idle = 0x00,
  PAD_Access = 0x01,
  MC_Access = 0x81,
  Ignore = 0xFF
};

byte CurrentSPICommand = PS1_SPICommands::Idle;

PS1MemoryCard MemCard1;

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
  pinModeFast(MISO, OUTPUT);
  pinModeFast(ACK, OUTPUT);
}

void inline ENABLE_SPI()
{
  SPCR |= _BV(SPE); //Enable SPI
  pinModeFast(MISO, OUTPUT);
  pinModeFast(ACK, OUTPUT);
}

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

  //Serial.begin(115200);
  //Serial.println("Hello Computer");

  // Set bits in the SPCR register
  SPCR = 0x6F;

  //Clear SPSR and SPDR by reading their values
  clr = SPDR;
  clr = SPSR;

  clr = clr; // Supress clr not used warning. We're using it to clear flag registers above.

  SPDR = 0xFF; // Keep Slave out high, PS1 slave detect?
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

  while (1)
  {

    //if SS high, reset status
    //else, poll for spi data
    //Switch satement program status, pipe code out accordingly
    //Process incoming
    //Push response to SPDR

    if (digitalReadFast(SS) == HIGH)
    {
      //Serial.println("Idle");
      if (CurrentSPICommand != PS1_SPICommands::Idle)
      {
        CurrentSPICommand = PS1_SPICommands::Idle; // Clear last command
        MemCard1.GoIdle();                         // Reset Memory Card State
        ENABLE_SPI();
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
          bTempAck = MemCard1.SendAck();

          if (CurrentSPICommand == PS1_SPICommands::Idle)
            CurrentSPICommand = DataIn;

          switch (CurrentSPICommand)
          {

          case PS1_SPICommands::MC_Access:
            DataOut = MemCard1.Process(DataIn);
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