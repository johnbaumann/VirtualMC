# VirtualMC
  Experimental implementation of an Arduino as a Playstation 1 Memory Card
  
  Game save data is stored in program memory, with space for 3 blocks = Directory + 2 save files
  
  Read/Write functions working on console

  Read/Write working over serial using memcarduino protocol

# ISSUES
  Still working on juggling the Serial and SIO comms, may still be causing some comms interference.
  
  Project is still a bit cobbled together and documentation lacking. As things are more refined I will work on updating this.

# To-do
  Fix SIO Ignore/Sleep mode

  Supress directory entries beyond available blocks
   
  Sleep SIO if ATT+SCK stay low - i.e. if not connected to console
  
  Fix up platformio.ini

