#ifndef AVR_SPI_H
#define AVR_SPI_H

#include <stdint.h>

namespace VirtualMC
{
    namespace avr
    {
        namespace spi
        {
            void EnableActiveMode();
            void EnablePassiveMode();

            bool IsDataReady();

            void Disable();
            void Enable();
            void Initialize();

            void SendACKInterrupt();

            static const uint8_t kACKInterruptPin = 9;
        }
    }
}

#endif // AVR_SPI_H