#include <Arduino.h>
#include <SPI.h>
#include <avr/pgmspace.h> // Allows reading memory from program storage space.

#include "avr_digitalWriteFast.h" //https://code.google.com/archive/p/digitalwritefast/
#include "avr_spi.h"
#include "sio.h"
#include "sio_memory_card.h"

//Written for the 328p running 3.3V @ 8MHz
/*
  -----------------------
  PS Side to Arduino digital as follows
  -----------------------
  1 -> 12 // DATA/MISO
  2 -> 11 // CMND/MOSI
  3 -> RAW // 7.6V to regulated input
  4 -> GND // Ground to ground
  5 -> Unused
  6 -> 10 // ATT/SS
  7 -> 13 // Clock/CLK/SCK
  8 -> 9 // ACK/Acknowledge


  -----------------------
  FTDI to Arduino
  -----------------------
  TXD -> RXD
  RXD -> TXD
  GND -> GND
*/

namespace VirtualMC
{
  void Initialize(void)
  {
    Serial.end();
    cli(); // Disable interrupts for whole project, don't need them.
    sio::SIO_Init();
    avr::spi::Initialize();
  }

  void main()
  {
    // TODO(portability):
    // Remove AVR specific code from sio namespace
    //
    while (1)
    {

      // if SS high, reset status
      // else, poll for spi data
      // Switch satement program status, pipe code out accordingly
      // Process incoming
      // Push response to SPDR

      // Slave bus not selected
      if (digitalReadFast(SS) == LOW)
      {
        // Disable Interrupts
        /*if(sio::CurrentSIOCommand == sio::PS1_SIOCommands::Idle)
          cli();*/
        sio::SIO_ProcessEvents();
      }
      else
      {
        if (sio::CurrentSIOCommand != sio::PS1_SIOCommands::Idle)
        {
          // Clear last command
          sio::CurrentSIOCommand = sio::PS1_SIOCommands::Idle;

          // Reset Memory Card commands/variables
          sio::memory_card::GoIdle();

          // Quietly listen on SPI
          avr::spi::EnablePassiveMode();
          avr::spi::Enable();
          // Prepare the SPI Register with some fill data
          //SPDR = 0xFF;

          // Enable Interrupts
          //sei();
        }
      }
    }
  }
}

void setup(void)
{
  VirtualMC::Initialize();
}

void loop()
{
  VirtualMC::main();
}