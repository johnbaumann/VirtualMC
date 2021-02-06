#include "main.h"

//TODO: Flash optiboot, implement flash page writes
//      Began adding memcarduino command support

//Written for the 328p running 3.3V @ 8MHz
//Should work on other chips with native SPI so long as logic is 3.3V
//ACK     2   (PS1: ACK)
//SS      10  (PS1: ATT)
//MOSI    11  (PS1: CMND)
//MISO    12  (PS1: Data)

void setup(void)
{
  Serial_Init();
  SPI_Init();
  // Reset Memory Card commands/variables
  MC_GoIdle();
}

void loop()
{
  bool SlaveSelected = false;
  while (1)
  {

    //if SS high, reset status
    //else, poll for spi data
    //Switch satement program status, pipe code out accordingly
    //Process incoming
    //Push response to SPDR

    // Slave bus not selected
    if (digitalReadFast(SS) == HIGH)
    {
      SlaveSelected = false;
      if(SIO_IdleTicks < SIOMAXIDLETICKS)
        SIO_IdleTicks++;
    }
    else
    {
      SlaveSelected = true;
      SIO_IdleTicks = 0;
    }

    if (!SlaveSelected || Serial_Busy())
    {
      // Status not idle, reset SIO/SPI state
      if (CurrentSIOCommand != PS1_SIOCommands::Idle)
      {
        // Clear last command
        CurrentSIOCommand = PS1_SIOCommands::Idle;

        // Reset Memory Card commands/variables
        MC_GoIdle();
        PAD_GoIdle();

        // Quietly listen on SPI
        SPI_Passive();
        SPI_Enable();
        // Prepare the SPI Register with some fill data
        SPDR = 0xFF;

        // Serial requires interrupts, toggle on and resume serial
        interrupts();
        Serial.begin(38400);
      }
      if(RTS_Status == false && SIO_IdleTicks >= SIOMAXIDLETICKS && Serial_ActiveTicks < SERIALMAXACTIVETICKS)
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
      SIO_ProcessEvents();
      // Exit SIO Loop
    }
  }
}