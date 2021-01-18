#include "PS1MemoryCard.h"

PS1MemoryCard::PS1MemoryCard()
{
    this->Cur_Cmnd = MC_Commands::None;
    this->FLAG = MC_Flags::Directory_Unread;
    this->MC_Sector = 0x0000;
    this->Cmnd_Ticks = 0;
    this->bSendAck = false;
}

void PS1MemoryCard::SlaveSelectLow()
{
    this->Cur_Cmnd = MC_Commands::None;
    this->FLAG = MC_Flags::Directory_Unread;
    this->MC_Sector = 0x0000;
    this->Cmnd_Ticks = 0;
    this->bSendAck = false;
}

bool PS1MemoryCard::IsActiveDevice()
{
    return this->bDeviceActive;
}

byte PS1MemoryCard::Process(byte DataIn)
{
    bool cmdRouted = false;

    while (!cmdRouted)
    {
        switch (this->Cur_Cmnd)
        {
        case MC_Commands::None:
            if (DataIn == MC_Commands::Access)
            {
                cmdRouted = true;
                this->bSendAck = true;
                return this->FLAG;
            }
            break;

        case MC_Commands::Access:
            if (DataIn == MC_Commands::Read || DataIn == MC_Commands::Write || DataIn == MC_Commands::Get_ID)
            {
                this->Cur_Cmnd = DataIn;
                cmdRouted = false;
            }
            break;

        case MC_Commands::Read:
            return this->ReadCmnd_Tick(DataIn);
            cmdRouted = true;
            break;

        default:
            return MC_Responses::Idle_High;
            cmdRouted = true;
            break;
        }
    }
    return 0xFF;
}

byte PS1MemoryCard::ReadCmnd_Tick(byte &DataIn)
{
    this->bSendAck = true; // Default true;
    switch (this->Cmnd_Ticks)
    {
    case 0:
        return MC_Responses::ID1;
        break;

    case 1:
        return MC_Responses::ID2;
        break;

    case 2:
        return MC_Responses::Dummy;
        break;

    case 3:
        return DataIn;
        break;

    default:
        return MC_Responses::Idle_High;
        break;
    }

    this->Cmnd_Ticks++;
}

bool PS1MemoryCard::SendAck()
{
    if (this->bSendAck == true)
    {
        this->bSendAck = false;
        return this->bSendAck;
    }
    else
    {
        return false;
    }
}