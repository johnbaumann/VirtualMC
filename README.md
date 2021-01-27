# VirtualMC
  Experimental implementation of an Arduino as a Playstation 1 Memory Card
  
  MC1Data.h, stored in program memory, contains two blocks of a memory card: The first block(directory) and the second block is a random FF7 save.
  
  Read/Write functions working on actual console. At this time, the Write function simply computes the checksum and clears the FLAG bits.

  Read/Write working over serial using memcarduino protocol

# To-do
Check/Fix interference between memory card slots.
Supress directory entries beyond available blocks
Add write code to memcard write call
Fix up platformio.ini
Check timing on calls with new bootloader
Sleep SIO/SPI for a few seconds on serial comms
