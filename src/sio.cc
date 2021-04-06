#include "sio.h"

#include <Arduino.h>

#include "avr_digitalWriteFast.h"
//#include "avr_flashdata.h"
#include "avr_serial.h"
#include "avr_spi.h"
#include "sio_memory_card.h"
#include "sio_net_yaroze.h"
#include "sio_controller.h"

namespace VirtualMC
{
    namespace sio
    {
        byte CurrentSIOCommand = PS1_SIOCommands::Idle;

        uint16_t SIO_IdleTicks = 0;

        bool bMemCardEnabled = false;
        bool bPadEnabled = false;
        bool bNYEnabled = false;

        void SIO_Init()
        {
            // Reset Memory Card commands/variables
            controller::GoIdle();
            memory_card::GoIdle();
            net_yaroze::GoIdle();
        }

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
                    avr::Serial_ActiveTicks = 0;
                    // Re-enable SPI output
                    VirtualMC::avr::spi::EnableActiveMode();
                    // Turn off interrupts for SIO timing
                    //noInterrupts();
                    // Wait for SPI transmission
                    CurrentSIOCommand = PS1_SIOCommands::Wait;
                }

                // Check SPI status register
                if (VirtualMC::avr::spi::IsDataReady())
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
                            bTempAck = memory_card::SendAck;
                            DataOut = memory_card::ProcessEvents(DataIn);
                        }
                        else
                        {
                            CurrentSIOCommand = PS1_SIOCommands::Ignore;
                            VirtualMC::avr::spi::Disable();
                            bTempAck = false;
                        }

                        break;

                    case PS1_SIOCommands::PAD_Access:
                        if (bPadEnabled)
                        {
                            bTempAck = controller::SendAck;
                            DataOut = controller::ProcessEvents(DataIn);
                        }
                        else
                        {
                            CurrentSIOCommand = PS1_SIOCommands::Ignore;
                            VirtualMC::avr::spi::Disable();
                            bTempAck = false;
                        }
                        break;

                    case PS1_SIOCommands::NY_Access:
                        if (bNYEnabled)
                        {
                            bTempAck = net_yaroze::SendAck;
                            DataOut = net_yaroze::ProcessEvents(DataIn);
                        }
                        else
                        {
                            CurrentSIOCommand = PS1_SIOCommands::Ignore;
                            VirtualMC::avr::spi::Disable();
                            bTempAck = false;
                        }
                        break;

                    default: // Bad/Unexpected/Unsupported slave select command
                        CurrentSIOCommand = PS1_SIOCommands::Ignore;
                        VirtualMC::avr::spi::Disable();
                        bTempAck = false;
                    }
                    // Push outbound data to the SPI Data Register
                    // Data will be transferred in the next byte pair
                    SPDR = DataOut;

                    // Actual delay here varies by a few uS, but trying to match real hardware delays
                    delayMicroseconds(2);

                    // Only send ACK if slave still selected
                    if (bTempAck && digitalReadFast(SS) == LOW)
                        VirtualMC::avr::spi::SendACKInterrupt();

                    // If data is ready for card, store it.
                    // This takes a bit so this is done after SPDR + ACK
                    memory_card::Commit();
                }
                else
                {
                    if (sio::SIO_IdleTicks < SIOMAXIDLETICKS)
                    {
                        sio::SIO_IdleTicks++;
                    }
                    else
                    {
                        // SPI status held low too long, possibly disconnected?
                    }
                }
            }
        }
    } // namespace sio
} // namespace VirtualMC