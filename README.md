# VirtualMC
  Experimental implementation of an Arduino as a Playstation 1 Memory Card - This is still a work in progress, updates are mostly stable but may have unexpected results
  
  Game save data is stored in program memory, with space for 3 blocks = Directory + 2 save files
  
  Read/Write functions working on console

  Read/Write working over serial using memcarduino protocol

  Basic pad imitation working, no way to send button states yet

  Serial commands can be used to set pad/card SIO interfaces to active/inactive

  
# ISSUES
  Directory entries past 3rd block not supressed, though the sectors themselves are.

  If device is powered externally and disconnected from a console, serial commands are unavailable unless the slave select pin is pulled high.

# To-do
  Upload PC software for serial communication, currently using modified code from https://github.com/taka-tuos/memcarduinoplus
  
  Supress directory entries beyond available blocks.
 
  Sleep SIO if ATT+SCK stay low - i.e. if not connected to console

  Reevaluate/rewrite ACK exchange behavior
  
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

  8 -> 2    // ACK/Acknowledge
  
  FTDI to Arduino

  TXD -> RXD

  RXD -> TXD

  GND -> GND

  Small(0.1uF etc) capacitor between RST and RTS for auto reset

  Reset not explicitly required, useful for reflashing
  