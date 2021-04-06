#ifndef SIO_MCRD_H
#define SIO_MCRD_H

#include <Arduino.h>

#include "SD.h"

namespace VirtualMC
{
    namespace sio
    {
        namespace memory_card
        {

            // Implement actual bit manipulation later
            // Need more info on FLAG bits.
            enum Flags : byte
            {
                kDirectoryUnread = 0x08, // Initial power on value
                kDirectoryRead = 0x00    // Cleared after good MC Write
                                         // (test write sector 3F = 0x1F80) offset
            };

            enum Commands : byte
            {
                kAccess = 0x81, // Memory Card Select
                kRead = 0x52,   // Read Command
                kGetID = 0x53,  // Get ID Command
                kWrite = 0x57,  // Write Command
                kNone = 0x00,   // No command, idle state
                kError = 0xFF   // Bad command
            };

            enum Responses : byte
            {
                kIdleHighZ = 0xFF,           // High default state
                kDummy = 0x00,               // Filler Data
                kID1 = 0x5A,                 // Memory Card ID1
                kID2 = 0x5D,                 // Memory Card ID2
                kCommandAcknowledge1 = 0x5C, // Command Acknowledge 1
                kCommandAcknowledge2 = 0x5D, // Command Acknowledge 2
                kGoodReadWrite = 0x47,       // Good Read/Write
                kBadChecksum = 0x4E,         // Bad Checksum during Write
                kBadSector = 0xFF            // Bad Memory Card Sector
            };

            extern byte FLAG;
            
            extern File myFile;

            extern uint16_t Sector;
            extern bool SendAck;

            void Commit();
            void Disable();
            void Enable();
            void GoIdle();
            uint8_t ProcessEvents(uint8_t);
            uint8_t TickReadCommand(uint8_t &);
            uint8_t TickWriteCommand(uint8_t &);
        } // namespace memory_card
    } //namespace sio
} // namespace VirtualMC

#endif //SIO_MCRD_H