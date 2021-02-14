#include "sio.h"

byte CurrentSIOCommand = PS1_SIOCommands::Idle;

uint16_t SIO_IdleTicks = 0;

bool bMemCardEnabled = true;
bool bPadEnabled = true;

void SIO_ProcessEvents()
{
    byte DataIn = 0x00;
    byte DataOut = 0xFF;
    bool bTempAck = false;

    // If ignore, do nothing
    if (CurrentSIOCommand == PS1_SIOCommands::Ignore)
    {
        return;
    }
    else // Enter SIO Loop
    {
        // Nothing done yet, prepare for SIO/SPI
        if (CurrentSIOCommand == PS1_SIOCommands::Idle)
        {
            // Terminate Serial comms
            if (RTS_Status == true)
            {
                RTS_Status = false;
                digitalWriteFast(RTS_Pin, HIGH);
            }
            Serial_ActiveTicks = 0;
            Serial.end();
            Serial_GoIdle();
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
            SIO_IdleTicks = 0;
            // Store incoming data to variable
            DataIn = SPDR;

            // Waiting for command, store incoming byte as command
            if (CurrentSIOCommand == PS1_SIOCommands::Wait)
                CurrentSIOCommand = DataIn;

            // Interpret incoming command
            switch (CurrentSIOCommand)
            {

            // Console requests memory card, continue interpreting command
            case PS1_SIOCommands::MC_Access:
                if (bMemCardEnabled)
                {
                    // Byte exchange is offset by one
                    // This offsets the ACK signal accordingly
                    bTempAck = MC_SendAck;
                    DataOut = MC_ProcessEvents(DataIn);
                }
                else
                {
                    CurrentSIOCommand = PS1_SIOCommands::Ignore;
                    SPI_Disable();
                    bTempAck = false;
                }

                break;

            // Ignore pad, cascade to default ignore behavior
            case PS1_SIOCommands::PAD_Access:
                if (bPadEnabled)
                {
                    bTempAck = PAD_SendAck;
                    DataOut = PAD_ProcessEvents(DataIn);
                }
                else
                {
                    CurrentSIOCommand = PS1_SIOCommands::Ignore;
                    SPI_Disable();
                    bTempAck = false;
                }
                break;

            default: // Bad/Unexpected/Unsupported slave select command
                CurrentSIOCommand = PS1_SIOCommands::Ignore;
                SPI_Disable();
                bTempAck = false;
            }
            // Push outbound data to the SPI Data Register
            // Data will be transferred in the next byte pair
            SPDR = DataOut;

            // Only send ACK if slave still selected
            if (bTempAck && digitalReadFast(SS) == LOW)
                SEND_ACK();

            // If data is ready for card, store it.
            // This takes a bit so this is done after SPDR + ACK
            MC_CommitUncommited();
        }
    }
}
