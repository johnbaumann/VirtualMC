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
  SPI_Init();
  // Reset Memory Card commands/variables
  MC_GoIdle();
}

void loop()
{
  byte DataIn = 0x00;
  byte DataOut = 0xFF;
  bool bTempAck = false;
  bool cmdRouted = false;

  while (1)
  {

    //if SS high, reset status
    //else, poll for spi data
    //Switch satement program status, pipe code out accordingly
    //Process incoming
    //Push response to SPDR

    // Slave bus not selected
    if (digitalReadFast(SS) == HIGH || Serial_IdleTicks < SERIALTIMEOUTTICKS)
    {
      // Status not idle, reset SIO/SPI state
      if (CurrentSIOCommand != PS1_SIOCommands::Idle)
      {
        // Clear last command
        CurrentSIOCommand = PS1_SIOCommands::Idle;

        // Reset Memory Card commands/variables
        MC_GoIdle();

        // Quietly listen on SPI
        SPI_Passive();
        SPI_Enable();
        // Prepare the SPI Register with some fill data
        SPDR = MC_Responses::Idle_High;

        // Serial requires interrupts, toggle on and resume serial
        interrupts();
        Serial.begin(38400);
      }
      // Check for serial commands,
      // Enter any loops required
      // Update Serial_IdleTicks
      ProcessSerialEvents();
    }
    // Slave bus selected
    else
    {
      // If ignore, do nothing
      if (CurrentSIOCommand == PS1_SIOCommands::Ignore )
      {
        continue;
      }
      else // Enter SIO Loop
      {
        // Nothing done yet, prepare for SIO/SPI
        if (CurrentSIOCommand == PS1_SIOCommands::Idle)
        {
          // Terminate Serial comms
          Serial.end();
          // Re-enable SPI output
          SPI_Active();
          // Turn off interrupts for SIO timing
          noInterrupts();
          // Wait for SPI transmission
          CurrentSIOCommand = PS1_SIOCommands::Wait;
        }

        // Check SPI status register
        if (SPI_Data_Ready())
        {
          // Store incoming data to variable
          DataIn = SPDR;
          // Byte exchange is offset by one
          // This offsets the ACK signal accordingly
          bTempAck = MC_SendAck;

          // Waiting for command, store incoming byte as command
          if (CurrentSIOCommand == PS1_SIOCommands::Wait)
            CurrentSIOCommand = DataIn;

          // Interpret incoming command
          switch (CurrentSIOCommand)
          {

          // Console requests memory card, continue interpreting command
          case PS1_SIOCommands::MC_Access:
            //Loop until command is properly handled
            cmdRouted = false;
            while (!cmdRouted)
            {
              switch (MC_Cur_Cmnd)
              {
                // No command yet
              case MC_Commands::None:
                // Waiting for command, store incoming byte as command
                MC_Cur_Cmnd = DataIn;
                // Store FLAG byte in outbound variable
                DataOut = MC_FLAG;
                // Safe to exit interpret loop
                cmdRouted = true;
                break;

              // Last command was MC Access, expecting
              case MC_Commands::Access:
                if (DataIn == MC_Commands::Read || DataIn == MC_Commands::Write || DataIn == MC_Commands::Get_ID)
                {
                  MC_Cur_Cmnd = DataIn;
                  // Re-evaluate command
                  cmdRouted = false;
                }
                else
                {
                  // Unknown command
                  MC_Cur_Cmnd = MC_Commands::Error;
                  cmdRouted = false;
                }
                break;

              case MC_Commands::Read:
                DataOut = MC_ReadCmnd_Tick(DataIn);
                cmdRouted = true;
                break;

              case MC_Commands::Write:
                DataOut = MC_WriteCmnd_Tick(DataIn);
                cmdRouted = true;
                break;

              case MC_Commands::Get_ID: // Un-implemented, need data capture
              case MC_Commands::Error:  // Unexpected/Unsupported command
              default:
                DataOut = MC_Responses::Idle_High;
                MC_GoIdle();
                cmdRouted = true;
                break;
              }
            }
            break;

          // Ignore pad, cascade to default ignore behavior
          case PS1_SIOCommands::PAD_Access:
          default: // Bad/Unexpected/Unsupported slave select command
            CurrentSIOCommand = PS1_SIOCommands::Ignore;
            SPI_Disable();
            bTempAck = false;
          }
          // Push outbound data to the SPI Data Register
          // Data will be transferred in the next byte pair
          SPDR = DataOut;

          // 
          if (bTempAck)
            SEND_ACK();

          // If data is ready for card, store it.
          if (MC_BufferFull)
            MC_CommitWrite();
        }
      }
      // Exit SIO Loop
    }
  }
}