#include "avr_spi.h"

#include <Arduino.h>

#include <avr_emulation.h>
#include <SPI.h>


namespace VirtualMC
{
    namespace avr
    {
        namespace spi
        {

            // Bit of a work around here to prevent drowning out other SIO devices while inactive
            void EnablePassiveMode()
            {
                digitalWrite(MISO, HIGH);
                pinMode(MISO, INPUT);
                digitalWrite(kACKInterruptPin, HIGH);
                pinMode(kACKInterruptPin, INPUT);
            }

            // Reactivate regular SPI communication
            void EnableActiveMode()
            {
                digitalWrite(MISO, HIGH);
                pinMode(MISO, OUTPUT);
                pinMode(kACKInterruptPin, OUTPUT);
            }

            // Toggle SPI off, used for ignoring commands on same slave bus
             void Disable()
            {
                SPCR &= ~_BV(SPE); //Disable SPI
                EnablePassiveMode();
                return;
            }

            // Toggle SPI back on
            void Enable()
            {
                SPCR |= _BV(SPE); //Enable SPI
            }



            void Initialize()
            {
                byte clr = 0;

                // Set everything as input, try not to interrupt SIO
                pinMode(SCK, INPUT);
                pinMode(SS, INPUT_PULLUP);
                pinMode(MOSI, INPUT);
                pinMode(MISO, INPUT);
                pinMode(kACKInterruptPin, INPUT);

                // Set bits in the SPCR register
                //SPCR |= _BV(SPR0) | _BV(SPR1) | _BV(CPHA) | _BV(CPOL) & ~_BV(MSTR) | _BV(DORD) | _BV(SPE) & ~_BV(SPIE); // = 0x6F
                //SPCR = 0x6F;

                SPCR.setMISO(MISO);
                SPCR.setMOSI(MOSI);
                SPCR.setSCK(SCK);
                SPCR.enable_pins();
                SPCR &= ~_BV(MSTR);
                SPCR &= ~_BV(SPIE);
                SPCR |= _BV(CPHA);
                SPCR |= _BV(CPOL);
                SPCR |= _BV(DORD);
                SPCR |= _BV(SPR0);
                SPCR |= _BV(SPR1);
                SPCR |= _BV(SPE);

                //Clear SPSR and SPDR by reading their values
                clr = SPDR;
                clr = SPSR;
                clr = clr; // Supress clr not used warning. We're using it to clear flag registers above.

                // Push some data to the register
                SPDR = 0xFF;
            }
        }
    }

}