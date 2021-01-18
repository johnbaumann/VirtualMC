#include <Arduino.h>
#include <SPI.h>

#include "digitalWriteFast.h" //https://code.google.com/archive/p/digitalwritefast/
#include "PS1MemoryCard.h"

static const uint8_t ACK = 2;

void loop();
void inline SEND_ACK();
void setup(void);

enum PS1_SPICommands : byte
{
  Idle = 0x00,
  PAD_Access = 0x01,
  MC_Access = 0x81
};

byte CurrentSPICommand = PS1_SPICommands::Idle;

void inline SEND_ACK()
{
  digitalWriteFast(ACK, LOW);
  delayMicroseconds(4);
  digitalWriteFast(ACK, HIGH);
}

PS1MemoryCard MemCard1;

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
  bool cmdRouted = false;
  byte DataIn = 0x00;
  byte DataOut = 0xFF;

  while (1)
  {

    //if SS high, reset status
    //else, poll for spi data
    //Switch satement program status, pipe code out accordingly
    //Process incoming
    //Push response to SPDR

    if (digitalReadFast(SS) == HIGH)
    {
      CurrentSPICommand = PS1_SPICommands::Idle;
      // Reset Memory Card()
    }
    else if (SPI_Data_Ready())
    {
      DataIn = SPDR;
      cmdRouted = false;

      while (!cmdRouted)
      {
        switch (CurrentSPICommand)
        {
        case PS1_SPICommands::Idle:
          CurrentSPICommand = DataIn;
          break;

        case PS1_SPICommands::MC_Access:
          SPDR = MemCard1.Process(DataIn);
          cmdRouted = true;
          break;

        case PS1_SPICommands::PAD_Access:
          SPDR = 0xFF; // Ignore
          CurrentSPICommand = PS1_SPICommands::Idle;
          cmdRouted = true;
          continue;
          break;

        default:       // Bad/Unexpected/Unsupported slave select command
          SPDR = 0xFF; // Ignore
          CurrentSPICommand = PS1_SPICommands::Idle;
          cmdRouted = true;
        }
      }
    }
  }
}