#include <Arduino.h>
#include <SD.h>
//#include <SPI.h>

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
  // set up variables using the SD utility library functions:
  Sd2Card card;
  SdVolume volume;
  SdFile root;
  SdFile mc_file;

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
      ; // wait for serial port to connect.
    }

    //Serial.print("\nInitializing SD card...");

    // we'll use the initialization code from the utility libraries
    // since we're just testing if the card is working!
    /*if (!card.init(SPI_HALF_SPEED, BUILTIN_SDCARD))
    {
      //Serial.println("initialization failed. Things to check:");
      //Serial.println("* is a card inserted?");
      //Serial.println("* is your wiring correct?");
      //Serial.println("* did you change the chipSelect pin to match your shield or module?");
      return;
    }
    else
    {
      //Serial.println("Wiring is correct and a card is present.");
    }

    // print the type of card
    //Serial.print("\nCard type: ");
    switch (card.type())
    {
    case SD_CARD_TYPE_SD1:
      //Serial.println("SD1");
      break;
    case SD_CARD_TYPE_SD2:
      //Serial.println("SD2");
      break;
    case SD_CARD_TYPE_SDHC:
      //Serial.println("SDHC");
      break;
    default:
      Serial.println("Unknown");
    }

    // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
    if (!volume.init(card))
    {
      //Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
      return;
    }

    // print the type and size of the first FAT-type volume
    uint32_t volumesize;
    //Serial.print("\nVolume type is FAT");
    //Serial.println(volume.fatType(), DEC);
    //Serial.println();

    volumesize = volume.blocksPerCluster(); // clusters are collections of blocks
    volumesize *= volume.clusterCount();    // we'll have a lot of clusters
    if (volumesize < 8388608ul)
    {
      //Serial.print("Volume size (bytes): ");
      //Serial.println(volumesize * 512); // SD card blocks are always 512 bytes
    }
    //Serial.print("Volume size (Kbytes): ");
    volumesize /= 2;
    //Serial.println(volumesize);
    //Serial.print("Volume size (Mbytes): ");
    volumesize /= 1024;
    //Serial.println(volumesize);

    //Serial.println("\nFiles found on the card (name, date and size in bytes): ");
    root.openRoot(volume);

    // list all files in the card with date and size
    //root.ls(LS_R | LS_DATE | LS_SIZE);
    mc_file.open(root, "TONYHAX.MCR", O_READ);
    //Serial.println("Attempting to open file");
    if (mc_file.isOpen())
    {
      //Serial.println("tonyhax.mcr opened");
      //Serial.print("File size: ");
      //Serial.print(mc_file.fileSize());
      //Serial.println(" byte(s).");
      if (mc_file.fileSize() == sizeof(sio::memory_card::memory_card_1))
      {
        //Serial.println("Proceeding with file read");
        mc_file.read(sio::memory_card::memory_card_1, mc_file.fileSize());
        //Serial.println("Finished reading file.");
        mc_file.write(1);
      }

      mc_file.close();
    }
    else
    {
      //Serial.println("Failed");
      root.close();
      return;
    }
    root.close();*/
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
      if (digitalRead(SS) == LOW)
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
        noInterrupts();
        sio::SIO_ProcessEvents();
        // Restore interrupts
        interrupts();
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