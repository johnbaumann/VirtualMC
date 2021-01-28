# VirtualMC
  Experimental implementation of an Arduino as a Playstation 1 Memory Card
  
  Game save data is stored in program memory, with space for 3 blocks = Directory + 2 save files
  
  Read/Write functions working on console

  Read/Write working over serial using memcarduino protocol

# To-do
  Supress directory entries beyond available blocks
   
  Sleep SIO if ATT+SCK stay low - i.e. if not connected to console
  
  Fix up platformio.ini

