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
    Serial_Init();
    VirtualMC::avr::spi::Initialize();
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
      if (digitalReadFast(SS) == HIGH)
      {
        SlaveSelected = false;
        if (sio::SIO_IdleTicks < SIOMAXIDLETICKS)
          sio::SIO_IdleTicks++;
      }
      else
      {
        SlaveSelected = true;
      }

      if (!SlaveSelected || Serial_Busy())
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
          VirtualMC::avr::spi::EnablePassiveMode();
          VirtualMC::avr::spi::Enable();
          // Prepare the SPI Register with some fill data
          SPDR = 0xFF;

          // Serial requires interrupts, toggle on and resume serial
          interrupts();
          Serial.begin(38400);
        }
        if (RTS_Status == false && sio::SIO_IdleTicks >= SIOMAXIDLETICKS && Serial_ActiveTicks < SERIALMAXACTIVETICKS)
        {
          RTS_Status = true;
          digitalWriteFast(RTS_Pin, LOW);
        }
        // Check for serial commands,
        // Update Serial_IdleTicks
        Serial_ProcessEvents();
      }
      // Slave bus selected
      else if (SlaveSelected && !Serial_Busy())
      {
        sio::SIO_ProcessEvents();
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