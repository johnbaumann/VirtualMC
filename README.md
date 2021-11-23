#### Quick notes:
This project is no longer in development, I have moved my efforts to the ESP32: https://github.com/johnbaumann/OpenMC

Check out the [just_the_card](https://github.com/johnbaumann/VirtualMC/tree/just_the_card) branch for a stripped down version of the project which removes everything but the memory card functionality, hopefully this makes the code easier to digest. The self-write call is commented out, so the stock arduino bootloader can be used.

# VirtualMC

  Experimental implementation of an Arduino as a Playstation 1 Memory Card(and now game pad) - This is still a work in progress, updates are mostly stable but may have unexpected results
  
  Game save data is stored in program memory, with space for 3 blocks = Directory + 2 save files -Plans to adapt the code for MEGA2560 to expand storage
  
  Read/Write functions working on console

  Read/Write working over serial using memcarduino protocol, compatible with MemcardRex

  Device can function as a digital pad. Pad state is sent over serial, accurate timing requires a pin for RTS/CTS flow control

  Serial commands can be used to set pad/card SIO interfaces to active/inactive
  
  Memory Card and Pad are enabled by default

  
# ISSUES
  Directory entries past 3rd block not suppressed, though the sectors themselves are.

  If device is powered externally and disconnected from a console, serial commands are unavailable unless the slave select pin is pulled high.

# To-do
  Write a program or python script for mapping inputs and sending them over serial
  
  Suppress directory entries beyond available blocks.
 
  Sleep SIO if ATT+SCK stay low - i.e. if not connected to console
 
  Fix up platformio.ini

  Rewrite code for flash memory page sizes + far memory access for future Mega 2560 support

# Connections / Hardware
  Arduino Pro Mini 328P 8Mhz @ 3.3V w/ Minicore by MCUDude - https://github.com/MCUdude/MiniCore/

  FTDI TTL


  PS Side to Arduino digital as follows

  1 -> 12   // DATA/MISO

  2 -> 11   // CMND/MOSI

  3 -> RAW  // 7.6V to regulated input

  4 -> GND  // Ground to ground

  5 -> Unused

  6 -> 10   // ATT/SS

  7 -> 13   // Clock/CLK/SCK

  8 -> 9    // ACK/Acknowledge
  
  FTDI to Arduino

  TXD -> RXD

  RXD -> TXD

  GND -> GND

  CTS ->2

  RTS ->3 //Not monitored by arduino atm, keeping here for future compatibility.

Various code and snippets from the following sources

https://github.com/ShendoXT/memcarduino

https://github.com/taka-tuos/memcarduinoplus

https://problemkaputt.de/psx-spx.htm

