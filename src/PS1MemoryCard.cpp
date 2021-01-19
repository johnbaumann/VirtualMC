#include "PS1MemoryCard.h"

PS1MemoryCard::PS1MemoryCard()
{
    this->FLAG = MC_Flags::Directory_Unread;

    this->GoIdle();
}

void PS1MemoryCard::GoIdle()
{
    this->Cur_Cmnd = MC_Commands::None;
    this->MC_Sector = 0x0000;
    this->Cmnd_Ticks = 0;
    this->bSendAck = false;
    this->Sector_Offset = 0;
}

byte PS1MemoryCard::Process(byte DataIn)
{
    bool cmdRouted = false;
    byte DataOut;

    while (!cmdRouted)
    {
        switch (this->Cur_Cmnd)
        {
        case MC_Commands::None:
            this->Cur_Cmnd = DataIn;
            this->bSendAck = true;
            DataOut = this->FLAG;
            cmdRouted = true;
            break;

        case MC_Commands::Access:
            if (DataIn == MC_Commands::Read || DataIn == MC_Commands::Write || DataIn == MC_Commands::Get_ID)
            {
                //TODO: Possible to avoid code duplication around here
                //NOTE: Same first 2 replies for all commands: Card ID1 + ID2
                //Read and Write share the 2 commands after, MSB+LSB receive
                //Get_ID not a required command
                this->Cur_Cmnd = DataIn;
                cmdRouted = false;
            }
            break;

        case MC_Commands::Read:
            DataOut = this->ReadCmnd_Tick(DataIn);
            cmdRouted = true;
            break;

        case MC_Commands::Write:
            DataOut = this->WriteCmnd_Tick(DataIn);
            cmdRouted = true;
            break;

        case MC_Commands::Get_ID:
            DataOut = this->GetIDCmnd_Tick(DataIn);
            cmdRouted = true;
            break;

        default:
            DataOut = MC_Responses::Idle_High;
            this->GoIdle();
            cmdRouted = true;
            break;
        }
    }

    return DataOut;
}

byte PS1MemoryCard::ReadCmnd_Tick(byte &DataIn)
{

    byte DataOut;

    this->bSendAck = true; // Default true;

    switch (this->Cmnd_Ticks)
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
        this->MC_Sector = (DataIn << 8);
        //Reply with (pre)
        DataOut = DataIn;
        break;

    case 4: //LSB
        //Store lower 8 bits of sector
        this->MC_Sector |= DataIn;
        DataOut = MC_Responses::CmdAck1;
        break;

    case 5: //00h
        DataOut = MC_Responses::CmdAck2;
        break;

    case 6: //00h
        //Confirm MSB
        DataOut = this->MC_Sector >> 8;
        break;

    case 7: //00h
        //Confirm LSB
        DataOut = (this->MC_Sector & 0xFF);
        this->Checksum_Out = (MC_Sector >> 8) ^ (MC_Sector & 0xFF);
        break;

    default:
        if (this->Cmnd_Ticks >= 8 && this->Cmnd_Ticks <= 135) //Stay here for 128 bytes
        {
            DataOut = pgm_read_byte_near((FakeData + (MC_Sector * (uint16_t)128) + this->Sector_Offset));
            this->Checksum_Out ^= DataOut;
            Sector_Offset++;
        }
        else if (this->Cmnd_Ticks == 136)
        {
            DataOut = Checksum_Out;
        }
        else if (this->Cmnd_Ticks == 137)
        {
            DataOut = MC_Responses::GoodRW;
        }
        else
        {
            DataOut = MC_Responses::Idle_High;
            this->GoIdle();
        }
        break;
    }

    this->Cmnd_Ticks++;

    return DataOut;
}

byte PS1MemoryCard::WriteCmnd_Tick(byte &DataIn)
{
    byte DataOut;

    this->bSendAck = true; // Default true;

    switch (this->Cmnd_Ticks)
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
        this->MC_Sector = (DataIn << 8);
        //Reply with (pre)
        DataOut = DataIn;
        break;

    case 4: //LSB
        //Store lower 8 bits of sector
        this->MC_Sector |= DataIn;
        this->Checksum_Out = (MC_Sector >> 8) ^ (MC_Sector & 0xFF);
        DataOut = DataIn;

        break;

    default:
        if (this->Cmnd_Ticks >= 5 && this->Cmnd_Ticks <= 132)
        {
            //To-do: Store data
            //Calculate checksum
            this->Checksum_Out ^= DataIn;
            //Reply with (pre)
            DataOut = DataIn;
        }
        else if (this->Cmnd_Ticks == 133) //CHK
        {
            this->Checksum_In = DataIn;
            DataOut = MC_Responses::CmdAck1;
        }
        else if (this->Cmnd_Ticks == 134) //00h
        {
            DataOut = MC_Responses::CmdAck2;
        }
        else if (this->Cmnd_Ticks == 135) //00h
        {
            if (this->Checksum_In == this->Checksum_Out)
            {
                DataOut = MC_Responses::GoodRW;
            }
            else
            {
                DataOut = MC_Responses::BadChecksum;
            }

            this->GoIdle();
        }
        else
        {
            this->GoIdle();
        }

        break;
    }

    this->Cmnd_Ticks++;

    return DataOut;
}

byte PS1MemoryCard::GetIDCmnd_Tick(byte &DataIn)
{

    return MC_Responses::Idle_High;
}

bool PS1MemoryCard::SendAck()
{
    return this->bSendAck;
}