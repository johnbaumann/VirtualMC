#include "sio_net_yaroze.h"

#include <Arduino.h>

#include "sio.h"

byte Cur_Cmnd;

namespace sio
{
    namespace net_yaroze
    {
        bool SendAck = true;

        void GoIdle()
        {
            Cur_Cmnd = Commands::kNone;
            SendAck = true;
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
                    DataOut = Responses::kID;
                    // Safe to exit interpret loop
                    cmdRouted = true;
                    break;

                case Commands::kAccess:
                    //Code not actually reached, value for debug
                    DataOut = 0xFC;
                    //NY_GoIdle();
                    SendAck = false;
                    cmdRouted = true;
                    break;

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
    } // namespace net_yaroze
} // namespace sio