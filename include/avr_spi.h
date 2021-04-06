#ifndef AVR_SPI_H
#define AVR_SPI_H

#include <Arduino.h>
#include <stdint.h>

#include "avr_digitalWriteFast.h"

namespace VirtualMC
{
    namespace avr
    {
        namespace spi
        {
            void EnableActiveMode();
            void EnablePassiveMode();

            inline bool IsDataReady() __attribute__((always_inline));

            void Disable();
            void Enable();
            void Initialize();

            inline void SendACKInterrupt() __attribute__((always_inline));

            static const uint8_t kACKInterruptPin = 9;

            bool IsDataReady()
            {
                // Return whether SPIF bit is set,
                // indicating the transmission is complete.
                return (SPSR & (1 << SPIF));
            }

            void SendACKInterrupt()
            {
                digitalWriteFast(kACKInterruptPin, LOW);
                // Keep ACK low for 4 uS
                delayMicroseconds(4);
                digitalWriteFast(kACKInterruptPin, HIGH);
            }
        }
    }
}

#endif // AVR_SPI_H