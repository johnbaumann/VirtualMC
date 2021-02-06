#include "memcard.h"
#include "testdata.h"



//Allocate space for memory card data
//SPM_PAGESIZE = 128 bytes on ATMEGA328P

byte MC_FLAG = MC_Flags::Directory_Unread;

byte MC_Cur_Cmnd;
uint8_t MC_Cmnd_Ticks;
uint16_t MC_Sector;
uint8_t MC_Sector_Offset;
byte MC_Checksum_Out;
byte MC_Checksum_In;
bool MC_SendAck = true;
bool MC_UncommitedWrite = false;

uint8_t MC_DataBuffer[SPM_PAGESIZE]; //128 on 328P

void MC_CommitUncommited()
{
  if (MC_UncommitedWrite)
    MC_CommitWrite();
}

void MC_GoIdle()
{
  MC_Cur_Cmnd = MC_Commands::None;
  MC_Cmnd_Ticks = 0;
  MC_SendAck = true;
  MC_Sector_Offset = 0;
}

byte MC_ProcessEvents(byte DataIn)
{
  byte DataOut;
  bool cmdRouted = false;

  //Loop until command is properly routed
  while (!cmdRouted)
  {
    switch (MC_Cur_Cmnd)
    {
      // No command yet
    case MC_Commands::None:
      // Waiting for command, store incoming byte as command
      MC_Cur_Cmnd = DataIn;
      // Store FLAG byte in outbound variable
      DataOut = MC_FLAG;
      // Safe to exit interpret loop
      cmdRouted = true;
      break;

    // Last command was MC Access, expecting
    case MC_Commands::Access:
      if (DataIn == MC_Commands::Read || DataIn == MC_Commands::Write || DataIn == MC_Commands::Get_ID)
      {
        MC_Cur_Cmnd = DataIn;
        // Re-evaluate command
      }
      else
      {
        // Unknown command
        MC_Cur_Cmnd = MC_Commands::Error;
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

    case MC_Commands::Get_ID: // Un-implemented, need data capture
    case MC_Commands::Error:  // Unexpected/Unsupported command
    default:
      DataOut = MC_Responses::Idle_High;
      MC_GoIdle();
      cmdRouted = true;
      break;
    }
  }

  return DataOut;
}

byte MC_ReadCmnd_Tick(byte &DataIn)
{
  byte DataOut;

  MC_SendAck = true; // Default true;

  switch (MC_Cmnd_Ticks)
  {
    //Data is sent and received simultaneously,
    //so the data we send isn't received by the system
    //until the next bytes are exchanged. In this way,
    //you have to basically respond one byte earlier than
    //actually occurs between Slave and Master.
    //Offset "Send" bytes noted from nocash's psx specs.

  case 0: //52h
    DataOut = MC_Responses::ID1;
    break;

  case 1: //00h
    DataOut = MC_Responses::ID2;
    break;

  case 2: //00h
    DataOut = MC_Responses::Dummy;
    break;

  case 3: //MSB
    //Store upper 8 bits of sector
    MC_Sector = (DataIn << 8);
    //Reply with (pre)
    DataOut = DataIn;
    break;

  case 4: //LSB
    //Store lower 8 bits of sector
    MC_Sector |= DataIn;
    DataOut = MC_Responses::CmdAck1;
    break;

  case 5: //00h
    DataOut = MC_Responses::CmdAck2;
    break;

  case 6: //00h
    //Confirm MSB
    DataOut = MC_Sector >> 8;
    break;

  case 7: //00h
    //Confirm LSB
    DataOut = (MC_Sector & 0xFF);
    MC_Checksum_Out = highByte(MC_Sector) ^ lowByte(MC_Sector);
    break;

    // Cases 8 through 135 overloaded to default operator below

  case 136:
    DataOut = MC_Checksum_Out;
    break;

  case 137:
    DataOut = MC_Responses::GoodRW;
    MC_SendAck = false;
    break;

  default:
    if (MC_Cmnd_Ticks >= 8 && MC_Cmnd_Ticks <= 135) //Stay here for 128 bytes
    {
      DataOut = pgm_read_byte_near((FlashData + (MC_Sector * (uint16_t)128) + MC_Sector_Offset));
      MC_Checksum_Out ^= DataOut;
      MC_Sector_Offset++;
    }
    else
    {
      DataOut = MC_Responses::Idle_High;
      MC_GoIdle();
    }
    break;
  }

  MC_Cmnd_Ticks++;

  return DataOut;
}

byte MC_WriteCmnd_Tick(byte &DataIn)
{
  byte DataOut;

  MC_SendAck = true; // Default true;

  switch (MC_Cmnd_Ticks)
  {
    // Data is sent and received simultaneously,
    // so the data we send isn't received by the system
    // until the next bytes are exchanged. In this way,
    // you have to basically respond one byte earlier than
    // actually occurs between Slave and Master.
    // Offset "Send" bytes noted from nocash's psx specs.

  case 0: // 52h
    DataOut = MC_Responses::ID1;
    break;

  case 1: // 00h
    DataOut = MC_Responses::ID2;
    break;

  case 2: // 00h
    DataOut = MC_Responses::Dummy;
    break;

  case 3: // MSB
    // Store upper 8 bits of sector
    MC_Sector = (DataIn << 8);
    // Reply with (pre)
    DataOut = DataIn;
    break;

  case 4: // LSB
    // Store lower 8 bits of sector
    MC_Sector |= DataIn;
    MC_Checksum_Out = (MC_Sector >> 8) ^ (MC_Sector & 0xFF);
    DataOut = DataIn;

    break;

  default:
    if (MC_Cmnd_Ticks >= 5 && MC_Cmnd_Ticks <= 132)
    {
      // Store data
      MC_DataBuffer[MC_Cmnd_Ticks - 5] = DataIn;
      // Calculate checksum
      MC_Checksum_Out ^= DataIn;
      // Reply with (pre)
      DataOut = DataIn;
    }
    else if (MC_Cmnd_Ticks == 133) // CHK
    {
      MC_Checksum_In = DataIn;
      DataOut = MC_Responses::CmdAck1;
    }
    else if (MC_Cmnd_Ticks == 134) // 00h
    {
      DataOut = MC_Responses::CmdAck2;
    }
    else if (MC_Cmnd_Ticks == 135) // 00h
    {
      if (MC_Sector < 0 || MC_Sector > 1024)
      {
        DataOut = MC_Responses::BadSector;
      }
      else if (MC_Checksum_In == MC_Checksum_Out)
      {
        MC_FLAG = MC_Flags::Directory_Read;
        DataOut = MC_Responses::GoodRW;
        // If the incoming sector is within our storage, store it
        if (MC_Sector + 1 <= NUMBER_OF_PAGES)
        {
          MC_UncommitedWrite = true;
        }
      }
      else
      {
        DataOut = MC_Responses::BadChecksum;
      }

      MC_GoIdle();
    }
    else
    {
      MC_GoIdle();
    }

    break;
  }

  MC_Cmnd_Ticks++;

  return DataOut;
}

void MC_CommitWrite()
{
  // Write buffer to memory page
  optiboot_writePage(FlashData, MC_DataBuffer, MC_Sector + 1);

  // Directory structure was updated, reset directory status
  if (MC_Sector == 0x0000)
    MC_FLAG = MC_Flags::Directory_Unread;

  // Clear buffer status before return
  MC_UncommitedWrite = false;
}