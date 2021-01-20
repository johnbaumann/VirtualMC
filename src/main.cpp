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
  delayMicroseconds(7);
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

  SPDR = 0xFF;    // Keep Slave out high, PS1 slave detect?
  noInterrupts(); //Not using interrupts, evidently they throw off timing.
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

  while (1)
  {
    bool bTempAck = false;

    //if SS high, reset status
    //else, poll for spi data
    //Switch satement program status, pipe code out accordingly
    //Process incoming
    //Push response to SPDR

    if (digitalReadFast(SS) == HIGH)
    {
      CurrentSPICommand = PS1_SPICommands::Idle; // Clear last command
      MemCard1.GoIdle();                         // Reset Memory Card State
    }
    else if (SPI_Data_Ready())
    {
      DataIn = SPDR;

      if (CurrentSPICommand == PS1_SPICommands::Idle)
        CurrentSPICommand = DataIn;

      switch (CurrentSPICommand)
      {
      case PS1_SPICommands::MC_Access:
        bTempAck = MemCard1.SendAck();
        DataOut = MemCard1.Process(DataIn);
        if (bTempAck)
          SEND_ACK();

        break;

      // Ignore pad, cascade to default ignore behavior
      case PS1_SPICommands::PAD_Access:
      default:          // Bad/Unexpected/Unsupported slave select command
        DataOut = 0xFF; // Ignore
        CurrentSPICommand = PS1_SPICommands::Idle;
      }

      SPDR = DataOut;
    }
  }
}