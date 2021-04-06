#include "sio_memory_card.h"

#include <Arduino.h>
#include <stdint.h>

//#include "avr_flashdata.h"
//#include "avr_optiboot.h"
#include "SD.h"
#include "sio.h"

namespace VirtualMC
{
  namespace sio
  {
    namespace memory_card
    {

      byte FLAG = Flags::kDirectoryUnread;

      uint8_t Cur_Cmnd = Commands::kNone;
      uint8_t Cmnd_Ticks = 0;
      uint16_t Sector = 0;
      uint8_t Sector_Offset = 0;
      uint8_t Checksum_In = 0;
      uint8_t Checksum_Out = 0;
      bool SendAck = true;
      bool UncommitedWrite = false;

      uint8_t DataBuffer[SPM_PAGESIZE];

      File myFile;

      void Commit()
      {
        if (UncommitedWrite)
        {
          if (SPM_PAGESIZE > 128)
          {
            for (uint16_t i = 0; i + 128 < SPM_PAGESIZE; i++)
            {
              //DataBuffer[i] = pgm_read_byte_near((FlashData + ((Sector) * 128) + i + 128));
            }
          }
          // Write buffer to memory page
          //optiboot_writePage(FlashData, DataBuffer, Sector + 1);

          // Directory structure was updated, reset directory status
          if (Sector == 0x0000)
            FLAG = Flags::kDirectoryUnread;

          // Clear buffer status before return
          UncommitedWrite = false;
        }

        return;
      }

      void Enable()
      {
        if (!bMemCardEnabled)
        {
          bMemCardEnabled = true;
          FLAG = Flags::kDirectoryUnread;
          GoIdle();
        }
      }

      void Disable()
      {
        if (bMemCardEnabled)
        {
          bMemCardEnabled = false;
          GoIdle();
        }
      }

      void GoIdle()
      {
        Cur_Cmnd = Commands::kNone;
        Cmnd_Ticks = 0;
        SendAck = true;
        Sector_Offset = 0;
      }

      uint8_t ProcessEvents(uint8_t DataIn)
      {
        uint8_t DataOut;
        bool cmdRouted = false;

        //Loop until command is properly routed
        while (!cmdRouted)
        {
          switch (Cur_Cmnd)
          {
            // No command yet
          case Commands::kNone:
            // Store incoming byte as command
            Cur_Cmnd = DataIn;
            // Store FLAG byte in outbound variable
            DataOut = FLAG;
            // Safe to exit interpret loop
            cmdRouted = true;
            break;

          // Last command was MC Access, expecting
          case Commands::kAccess:
            if (DataIn == Commands::kRead || DataIn == Commands::kWrite || DataIn == Commands::kGetID)
            {
              Cur_Cmnd = DataIn;
              // Re-evaluate command
              // cmdRouted = false;
            }
            else
            {
              // Unknown command
              Cur_Cmnd = Commands::kError;
              // Re-evaluate command
              // cmdRouted = false;
            }
            break;

          case Commands::kRead:
            DataOut = TickReadCommand(DataIn);
            cmdRouted = true;
            break;

          case Commands::kWrite:
            DataOut = TickWriteCommand(DataIn);
            cmdRouted = true;
            break;

          case Commands::kGetID: // Un-implemented, need data capture
          case Commands::kError: // Unexpected/Unsupported command
          default:
            DataOut = Responses::kIdleHighZ;
            GoIdle();
            cmdRouted = true;
            break;
          }
        }

        return DataOut;
      }

      uint8_t TickReadCommand(uint8_t &DataIn)
      {
        uint8_t DataOut;

        SendAck = true; // Default true;

        switch (Cmnd_Ticks)
        {
          //Data is sent and received simultaneously,
          //so the data we send isn't received by the system
          //until the next bytes are exchanged. In this way,
          //you have to basically respond one byte earlier than
          //actually occurs between Slave and Master.
          //Offset "Send" bytes noted from nocash's psx specs.

        case 0: //52h
          DataOut = Responses::kID1;
          break;

        case 1: //00h
          DataOut = Responses::kID2;
          break;

        case 2: //00h
          DataOut = Responses::kDummy;
          break;

        case 3: //MSB
          //Store upper 8 bits of sector
          Sector = (DataIn << 8);
          //Reply with (pre)
          DataOut = DataIn;
          break;

        case 4: //LSB
          //Store lower 8 bits of sector
          Sector |= DataIn;
          DataOut = Responses::kCommandAcknowledge1;
          break;

        case 5: //00h
          DataOut = Responses::kCommandAcknowledge2;
          break;

        case 6: //00h
          //Confirm MSB
          DataOut = Sector >> 8;
          break;

        case 7: //00h
          //Confirm LSB
          DataOut = (Sector & 0xFF);
          Checksum_Out = highByte(Sector) ^ lowByte(Sector);
          if (myFile.position() != ((uint32_t)Sector * 128u))
            myFile.seek((uint32_t)Sector * 128u);
          else
            myFile.peek();
          break;

          // Cases 8 through 135 overloaded to default operator below

        case 136:
          DataOut = Checksum_Out;
          break;

        case 137:
          DataOut = Responses::kGoodReadWrite;
          SendAck = false;
          break;

        default:
          if (Cmnd_Ticks >= 8 && Cmnd_Ticks <= 135) //Stay here for 128 bytes
          {
            //DataOut = pgm_read_byte_near((FlashData + (Sector * (uint16_t)128) + Sector_Offset));
            myFile.read(&DataOut, 1);
            Checksum_Out ^= DataOut;
            Sector_Offset++;
          }
          else
          {
            DataOut = Responses::kIdleHighZ;
            GoIdle();
          }
          break;
        }

        Cmnd_Ticks++;

        return DataOut;
      }

      uint8_t TickWriteCommand(uint8_t &DataIn)
      {
        uint8_t DataOut;

        SendAck = true; // Default true;

        switch (Cmnd_Ticks)
        {
          // Data is sent and received simultaneously,
          // so the data we send isn't received by the system
          // until the next bytes are exchanged. In this way,
          // you have to basically respond one byte earlier than
          // actually occurs between Slave and Master.
          // Offset "Send" bytes noted from nocash's psx specs.

        case 0: // 52h
          DataOut = Responses::kID1;
          break;

        case 1: // 00h
          DataOut = Responses::kID2;
          break;

        case 2: // 00h
          DataOut = Responses::kDummy;
          break;

        case 3: // MSB
          // Store upper 8 bits of sector
          Sector = (DataIn << 8);
          // Reply with (pre)
          DataOut = DataIn;
          break;

        case 4: // LSB
          // Store lower 8 bits of sector
          Sector |= DataIn;
          Checksum_Out = (Sector >> 8) ^ (Sector & 0xFF);
          DataOut = DataIn;

          break;

        default:
          if (Cmnd_Ticks >= 5 && Cmnd_Ticks <= 132)
          {
            // Store data
            DataBuffer[Cmnd_Ticks - 5] = DataIn;
            // Calculate checksum
            Checksum_Out ^= DataIn;
            // Reply with (pre)
            DataOut = DataIn;
          }
          else if (Cmnd_Ticks == 133) // CHK
          {
            Checksum_In = DataIn;
            DataOut = Responses::kCommandAcknowledge1;
          }
          else if (Cmnd_Ticks == 134) // 00h
          {
            DataOut = Responses::kCommandAcknowledge2;
          }
          else if (Cmnd_Ticks == 135) // 00h
          {
            if (Sector < 0 || Sector > 1024)
            {
              DataOut = Responses::kBadSector;
            }
            else if (Checksum_In == Checksum_Out)
            {
              FLAG = Flags::kDirectoryRead;
              DataOut = Responses::kGoodReadWrite;
              UncommitedWrite = true;
            }
            else
            {
              DataOut = Responses::kBadChecksum;
            }
            // Pre-advance file offset
            myFile.seek((uint32_t)(Sector+1) * 128u);
            
            GoIdle();
          }
          else
          {
            GoIdle();
          }

          break;
        }

        Cmnd_Ticks++;

        return DataOut;
      }
    } // namespace memory_card
  }   // namespace sio
} // namespace VirtualMC