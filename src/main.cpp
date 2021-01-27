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
  byte clr = 0;

  // Set MISO and ACK as output, the rest as input.
  pinModeFast(SCK, INPUT);
  pinModeFast(SS, INPUT);
  pinModeFast(MOSI, INPUT);
  pinModeFast(MISO, OUTPUT);
  pinModeFast(ACK, OUTPUT);

  // ACK high on idle
  digitalWriteFast(ACK, HIGH);

  Serial.begin(38400);

  // Set bits in the SPCR register
  SPCR = 0x6F;

  //Clear SPSR and SPDR by reading their values
  clr = SPDR;
  clr = SPSR;

  clr = clr; // Supress clr not used warning. We're using it to clear flag registers above.

  SPDR = 0xFF; // Keep Slave out high
  MC_GoIdle();
}

void inline SEND_ACK()
{
  //delayMicroseconds(7); //No delay needed, burning enough cycles
  digitalWriteFast(ACK, LOW);
  delayMicroseconds(4);
  digitalWriteFast(ACK, HIGH);
}

void inline DISABLE_SPI()
{
  SPCR &= ~_BV(SPE); //Disable SPI
  pinModeFast(MISO, INPUT);
  pinModeFast(ACK, INPUT);
  return;
}

void inline ENABLE_SPI()
{
  SPCR |= _BV(SPE); //Enable SPI
  pinModeFast(MISO, OUTPUT);
  pinModeFast(ACK, OUTPUT);
}

bool inline SPI_Data_Ready()
{
  // Return whether SPIF bit is set,
  // indicating the transmission is complete.
  return (SPSR & (1 << SPIF));
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

    if (digitalReadFast(SS) == HIGH)
    {
      if (CurrentSIOCommand != PS1_SIOCommands::Idle)
      {
        CurrentSIOCommand = PS1_SIOCommands::Idle; // Clear last command
        MC_GoIdle();                                  // Reset Memory Card State
        ENABLE_SPI();
      }
      ProcessSerialEvents();
    }
    else
    {
      if (CurrentSIOCommand != PS1_SIOCommands::Ignore)
      {

        if (SPI_Data_Ready())
        {
          noInterrupts();

          DataIn = SPDR;
          bTempAck = MC_SendAck;

          if (CurrentSIOCommand == PS1_SIOCommands::Idle)
            CurrentSIOCommand = DataIn;

          switch (CurrentSIOCommand)
          {

          case PS1_SIOCommands::MC_Access:
            cmdRouted = false;
            while (!cmdRouted)
            {
              switch (MC_Cur_Cmnd)
              {
              case MC_Commands::None:
                MC_Cur_Cmnd = DataIn;
                DataOut = MC_FLAG;
                cmdRouted = true;
                break;

              case MC_Commands::Access:
                if (DataIn == MC_Commands::Read || DataIn == MC_Commands::Write || DataIn == MC_Commands::Get_ID)
                {
                  MC_Cur_Cmnd = DataIn;
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
          default:          // Bad/Unexpected/Unsupported slave select command
            DataOut = 0xFF; // Ignore
            CurrentSIOCommand = PS1_SIOCommands::Ignore;
            DISABLE_SPI();
          }

          SPDR = DataOut;
          if (bTempAck)
            SEND_ACK();
          interrupts();
        }
      }
    }
  }
}