#include "sio_memory_card.h"

#include <Arduino.h>

#include "avr_flashdata.h"
#include "sio.h"

namespace VirtualMC
{
  namespace sio
  {
    namespace memory_card
    {

      byte FLAG = Flags::kDirectoryUnread;

      byte Cur_Cmnd;
      uint8_t Cmnd_Ticks;
      uint16_t Sector;
      uint8_t Sector_Offset;
      byte Checksum_In;
      byte Checksum_Out;
      bool SendAck = true;
      bool UncommitedWrite = false;

struct CompressedRange compressedrange[] = 
{
	{2U, 126U, 0x00},
	{157U, 254U, 0x00},
	{285U, 382U, 0x00},
	{414U, 510U, 0x00},
	{541U, 638U, 0x00},
	{669U, 766U, 0x00},
	{798U, 894U, 0x00},
	{906U, 1022U, 0x00},
	{1034U, 1150U, 0x00},
	{1162U, 1278U, 0x00},
	{1290U, 1406U, 0x00},
	{1418U, 1534U, 0x00},
	{1546U, 1662U, 0x00},
	{1674U, 1790U, 0x00},
	{1802U, 1918U, 0x00},
	{1930U, 2046U, 0x00},
	{2058U, 2175U, 0x00},
	{2186U, 2303U, 0x00},
	{2314U, 2431U, 0x00},
	{2442U, 2559U, 0x00},
	{2570U, 2687U, 0x00},
	{2698U, 2815U, 0x00},
	{2826U, 2943U, 0x00},
	{2954U, 3071U, 0x00},
	{3082U, 3199U, 0x00},
	{3210U, 3327U, 0x00},
	{3338U, 3455U, 0x00},
	{3466U, 3583U, 0x00},
	{3594U, 3711U, 0x00},
	{3722U, 3839U, 0x00},
	{3850U, 3967U, 0x00},
	{3978U, 4095U, 0x00},
	{4106U, 4223U, 0x00},
	{4234U, 4351U, 0x00},
	{4362U, 4479U, 0x00},
	{4490U, 4607U, 0x00},
	{4608U, 8063U, 0xFF},
	{8066U, 8190U, 0x00},
	{8448U, 8703U, 0x00},
	{8984U, 9047U, 0xFF},
	{9116U, 9179U, 0xFF},
	{9248U, 9311U, 0xFF},
	{9908U, 9971U, 0xFF},
	{9988U, 10619U, 0xFF},
	{10624U, 11643U, 0xFF},
	{11768U, 11908U, 0x00},
	{11952U, 12451U, 0x00},
	{12460U, 12542U, 0x00},
	{12573U, 12739U, 0x00},
	{12741U, 12963U, 0x00},
	{13037U, 16383U, 0x00},
	{16640U, 16895U, 0x00},
	{17176U, 17239U, 0xFF},
	{17308U, 17371U, 0xFF},
	{17440U, 17503U, 0xFF},
	{18100U, 18163U, 0xFF},
	{18180U, 18811U, 0xFF},
	{18816U, 19835U, 0xFF},
	{19960U, 20100U, 0x00},
	{20144U, 20643U, 0x00},
	{20652U, 20734U, 0x00},
	{20765U, 20931U, 0x00},
	{20933U, 21155U, 0x00},
	{21229U, 24575U, 0x00},
	{25368U, 25431U, 0xFF},
	{25500U, 25563U, 0xFF},
	{25632U, 25695U, 0xFF},
	{26292U, 26355U, 0xFF},
	{26372U, 27003U, 0xFF},
	{27008U, 28027U, 0xFF},
	{28152U, 28292U, 0x00},
	{28336U, 28835U, 0x00},
	{28844U, 28926U, 0x00},
	{28957U, 29123U, 0x00},
	{29125U, 29347U, 0x00},
	{29421U, 32767U, 0x00},
	{33342U, 33423U, 0x00},
	{33570U, 33651U, 0x00},
	{33798U, 33879U, 0x00},
	{34026U, 34107U, 0x00},
	{34254U, 34335U, 0x00},
	{34482U, 34563U, 0x00},
	{34710U, 34791U, 0x00},
	{34938U, 35019U, 0x00},
	{35166U, 35247U, 0x00},
	{35394U, 35475U, 0x00},
	{35622U, 35703U, 0x00},
	{35850U, 35931U, 0x00},
	{36078U, 36159U, 0x00},
	{36306U, 36387U, 0x00},
	{36534U, 36615U, 0x00},
	{36762U, 36843U, 0x00},
	{36990U, 37071U, 0x00},
	{37218U, 37299U, 0x00},
	{37446U, 37527U, 0x00},
	{37674U, 37755U, 0x00},
	{38547U, 38667U, 0x00},
	{38689U, 38857U, 0x00},
	{38975U, 39071U, 0x00},
	{39737U, 40063U, 0x00},
	{40064U, 40959U, 0xFF},
	{47830U, 49151U, 0x00},
	{49664U, 131071U, 0xFF}
};

      //uint8_t DataBuffer[SPM_PAGESIZE]; //128 on 328P

      void Commit()
      {
        if (UncommitedWrite)
        {
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

      void GoIdle()
      {
        Cur_Cmnd = Commands::kNone;
        Cmnd_Ticks = 0;
        SendAck = true;
        Sector_Offset = 0;
      }

      byte ProcessEvents(byte DataIn)
      {
        byte DataOut;
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

      byte TickReadCommand(byte &DataIn)
      {
        byte DataOut;

        uint32_t adjusted_address;
        uint32_t sector_address;
        bool using_adjusted_byte = false;

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
            adjusted_address = sector_address = ((uint32_t)Sector * 128U) + Sector_Offset;
            for (uint8_t i = 0; i < COMPRESSED_SECTIONS; i++)
            {
              if (sector_address > compressedrange[i].offset_end)
              {
                adjusted_address -= (compressedrange[i].offset_end - compressedrange[i].offset_begin) + 1;
              }
              if (sector_address >= compressedrange[i].offset_begin && sector_address <= compressedrange[i].offset_end)
              {
                DataOut = compressedrange[i].repeating_byte;
                using_adjusted_byte = true;
                i = COMPRESSED_SECTIONS; //Break for loop
              }
            }

            if (!using_adjusted_byte)
              DataOut = pgm_read_byte_near(FlashData + adjusted_address);
            //DataOut = pgm_read_byte_near((FlashData + (Sector * (uint16_t)128) + Sector_Offset));

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

      byte TickWriteCommand(byte &DataIn)
      {
        byte DataOut;

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

          // Cases 5 through 132 overloaded to default operator below

        case 133: // CHK
          Checksum_In = DataIn;
          DataOut = Responses::kCommandAcknowledge1;
          break;

        case 134: // 00h
          DataOut = Responses::kCommandAcknowledge2;
          break;

        case 135: // 00h
          if (Sector < 0 || Sector > 1024)
          {
            DataOut = Responses::kBadSector;
          }
          else if (Checksum_In == Checksum_Out)
          {
            FLAG = Flags::kDirectoryRead;
            DataOut = Responses::kGoodReadWrite;
            // If the incoming sector is within our storage, store it
            //if (Sector + 1 <= NUMBER_OF_PAGES)
            //{
            UncommitedWrite = true;
            //}
          }
          else
          {
            DataOut = Responses::kBadChecksum;
          }

          GoIdle();
          break;

        default:
          if (Cmnd_Ticks >= 5 && Cmnd_Ticks <= 132)
          {
            // Store data
            //DataBuffer[Cmnd_Ticks - 5] = DataIn;
            // Calculate checksum
            Checksum_Out ^= DataIn;
            // Reply with (pre)
            DataOut = DataIn;
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