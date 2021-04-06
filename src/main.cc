#include <Arduino.h>
#include <SPI.h>
#include <avr/pgmspace.h> // Allows reading memory from program storage space.

#include "avr_digitalWriteFast.h" //https://code.google.com/archive/p/digitalwritefast/
#include "avr_optiboot.h"
#include "avr_serial.h"
#include "avr_spi.h"

#include "sio.h"
#include "sio_memory_card.h"
#include "sio_controller.h"
#include "sio_net_yaroze.h"
#include "sio_tty.h"

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
  CTS ->2
  RTS ->3 //Not monitored by arduino atm, keeping here for future compatibility.
*/

namespace VirtualMC
{
  void Initialize(void)
  {
    sio::SIO_Init();
    avr::Serial_Init();
    avr::spi::Initialize();

    sio::memory_card::Enable();
    sio::controller::Enable();
    sio::net_yaroze::Enable();

    while (!Serial)
    {
      ; // wait for serial port to connect. Needed for native USB port only
    }

    Serial.print("Initializing SD card...");

    if (!SD.begin(5))
    {
      //Serial.println("SD initialization failed!");
      while (1)
        ;
    }
    Serial.println("SD initialization done.");

    sio::memory_card::myFile = SD.open("tonyhax.mcr");
    
    if (sio::memory_card::myFile)
      sio::memory_card::myFile.peek();
  }

  void main()
  {
    // TODO(portability):
    // Remove AVR specific code from sio namespace
    //

    bool SlaveSelected = false;

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
        SlaveSelected = true;
      }
      else
      {
        if (SlaveSelected)
        {
          SlaveSelected = false;
        }
      }

      if (!SlaveSelected || avr::Serial_Busy())
      {
        // Status not idle, reset SIO/SPI state
        if (sio::CurrentSIOCommand != sio::PS1_SIOCommands::Idle)
        {
          // Clear last command
          sio::CurrentSIOCommand = sio::PS1_SIOCommands::Idle;

          // Reset Memory Card commands/variables
          sio::memory_card::GoIdle();
          sio::controller::GoIdle();
          sio::net_yaroze::GoIdle();

          // Quietly listen on SPI
          avr::spi::EnablePassiveMode();
          avr::spi::Enable();
          // Prepare the SPI Register with some fill data
          SPDR = 0xFF;
        }
        // Check for serial commands,
        // Update Serial_IdleTicks
        avr::Serial_ProcessEvents();
      }
      // Slave bus selected
      else if (SlaveSelected && !avr::Serial_Busy())
      {
        // Disable interrupts
        asm volatile("cli");
        sio::SIO_ProcessEvents();
        // Restore interrupts
        asm volatile("sei");
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