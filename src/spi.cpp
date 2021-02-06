#include "spi.h"

void SEND_ACK()
{
    // Sometimes the code executes quicker than real hardware
    // There seems to be enough delay in the code that there's no need for an additional delay
    //delayMicroseconds(3);
    digitalWriteFast(ACK_Pin, LOW);
    // Keep ACK low for 4 uS
    delayMicroseconds(4);
    digitalWriteFast(ACK_Pin, HIGH);
}

// Bit of a work around here to prevent drowning out other SIO devices while inactive
void SPI_Passive()
{
    digitalWriteFast(MISO, HIGH);
    pinModeFast(MISO, INPUT);
    digitalWriteFast(ACK_Pin, HIGH);
    pinModeFast(ACK_Pin, INPUT);
}

// Reactivate regular SPI communication
void SPI_Active()
{
    digitalWriteFast(MISO, HIGH);
    pinModeFast(MISO, OUTPUT);
    pinModeFast(ACK_Pin, OUTPUT);
}

// Toggle SPI off, used for ignoring pad commands on same slave bus
void SPI_Disable()
{
    SPCR &= ~_BV(SPE); //Disable SPI
    SPI_Passive();

    return;
}

// Toggle SPI back on
void SPI_Enable()
{
    SPCR |= _BV(SPE); //Enable SPI
}

bool SPI_Data_Ready()
{
    // Return whether SPIF bit is set,
    // indicating the transmission is complete.
    return (SPSR & (1 << SPIF));
}

void SPI_Init()
{
    byte clr = 0;

    // Set everything as input, try not to interrupt SIO
    pinModeFast(SCK, INPUT);
    pinModeFast(SS, INPUT);
    pinModeFast(MOSI, INPUT);
    pinModeFast(MISO, INPUT);
    pinModeFast(ACK_Pin, INPUT);

    // Set bits in the SPCR register
    //SPCR |= _BV(SPR0) | _BV(SPR1) | _BV(CPHA) | _BV(CPOL) & ~_BV(MSTR) | _BV(DORD) | _BV(SPE) & ~_BV(SPIE); // = 0x6F
    SPCR = 0x6F;

    //Clear SPSR and SPDR by reading their values
    clr = SPDR;
    clr = SPSR;
    clr = clr; // Supress clr not used warning. We're using it to clear flag registers above.

    // Push some data to the register
    SPDR = 0xFF;
}