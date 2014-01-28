#include "Source/MessageIdentifiers.h"

#include "Source/RakPeerInterface.h"
#include "Source/RakNetStatistics.h"
#include "Source/RakNetTypes.h"
#include "Source/BitStream.h"
#include "Source/PacketLogger.h"
#include <assert.h>
#include <cstdio>
#include <cstring>
#include <stdlib.h>
#include "Source/RakNetTypes.h"

#include "Source/Kbhit.h"
#include <unistd.h> // usleep
#include "Source/Gets.h"

unsigned char GetPacketIdentifier(RakNet::Packet *p);
int main()
{
    RakNet::RakNetStatistics *rss;
    RakNet::RakPeerInterface *client=RakNet::RakPeerInterface::GetInstance();
    RakNet::Packet* p;
    unsigned char packetIdentifier;
    bool isServer;

    RakNet::SystemAddress clientID=RakNet::UNASSIGNED_SYSTEM_ADDRESS;

    char ip[64], serverPort[30], clientPort[30];

    isServer=false;
    strcpy(clientPort, "0");

    client->AllowConnectionResponseIPMigration(false);
    strcpy(ip, "127.0.0.1");


    strcpy(serverPort, "1234");

    RakNet::SocketDescriptor socketDescriptor(atoi(clientPort),0);
    socketDescriptor.socketFamily=AF_INET;
    client->Startup(8,&socketDescriptor, 1);
    client->SetOccasionalPing(true);


    RakNet::ConnectionAttemptResult car = client->Connect(ip, atoi(serverPort), "Rumpelstiltskin", (int) strlen("Rumpelstiltskin"));
    RakAssert(car==RakNet::CONNECTION_ATTEMPT_STARTED);

    unsigned int i;
    for (i=0; i < client->GetNumberOfAddresses(); i++)
    {
        printf("%i. %s\n", i+1, client->GetLocalIP(i));
    }

    printf("My GUID is %s\n", client->GetGuidFromSystemAddress(RakNet::UNASSIGNED_SYSTEM_ADDRESS).ToString());
    char message[2048];

    while (1)
    {
        usleep(30 * 1000);
        if (kbhit())
        {
            Gets(message,sizeof(message));

            if (strcmp(message, "quit")==0)
            {
                puts("Quitting.");
                break;
            }

            if (strcmp(message, "stat")==0)
            {

                rss=client->GetStatistics(client->GetSystemAddressFromIndex(0));
                StatisticsToString(rss, message, 2);
                printf("%s", message);
                printf("Ping=%i\n", client->GetAveragePing(client->GetSystemAddressFromIndex(0)));

                continue;
            }

            if (strcmp(message, "disconnect")==0)
            {
                printf("Enter index to disconnect: ");
                char str[32];
                Gets(str, sizeof(str));
                if (str[0]==0)
                    strcpy(str,"0");
                int index = atoi(str);
                client->CloseConnection(client->GetSystemAddressFromIndex(index),false);
                printf("Disconnecting.\n");
                continue;
            }

            if (strcmp(message, "shutdown")==0)
            {
                client->Shutdown(100);
                printf("Shutdown.\n");
                continue;
            }

            if (strcmp(message, "startup")==0)
            {
                bool b = client->Startup(8,&socketDescriptor, 1)==RakNet::RAKNET_STARTED;
                if (b)
                    printf("Started.\n");
                else
                    printf("Startup failed.\n");
                continue;
            }


            if (strcmp(message, "connect")==0)
            {
                printf("Enter server ip: ");
                Gets(ip, sizeof(ip));
                if (ip[0]==0)
                    strcpy(ip, "127.0.0.1");

                printf("Enter server port: ");
                Gets(serverPort,sizeof(serverPort));
                if (serverPort[0]==0)
                    strcpy(serverPort, "1234");


                bool b = client->Connect(ip, atoi(serverPort), "Rumpelstiltskin", (int) strlen("Rumpelstiltskin"))==RakNet::CONNECTION_ATTEMPT_STARTED;

                if (b)
                    puts("Attempting connection");
                else
                {
                    puts("Bad connection attempt.  Terminating.");
                    exit(1);
                }
                continue;
            }

            if (strcmp(message, "ping")==0)
            {
                if (client->GetSystemAddressFromIndex(0)!=RakNet::UNASSIGNED_SYSTEM_ADDRESS)
                    client->Ping(client->GetSystemAddressFromIndex(0));

                continue;
            }

            if (strcmp(message, "getlastping")==0)
            {
                if (client->GetSystemAddressFromIndex(0)!=RakNet::UNASSIGNED_SYSTEM_ADDRESS)
                    printf("Last ping is %i\n", client->GetLastPing(client->GetSystemAddressFromIndex(0)));

                continue;
            }

            client->Send(message, (int) strlen(message)+1, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
        }

        for (p=client->Receive(); p; client->DeallocatePacket(p), p=client->Receive())
        {
            packetIdentifier = GetPacketIdentifier(p);

            switch (packetIdentifier)
            {
            case ID_DISCONNECTION_NOTIFICATION:
                printf("ID_DISCONNECTION_NOTIFICATION\n");
                break;
            case ID_ALREADY_CONNECTED:
                printf("ID_ALREADY_CONNECTED with guid s%" PRINTF_64_BIT_MODIFIER "u\n", p->guid.ToString());
                break;
            case ID_INCOMPATIBLE_PROTOCOL_VERSION:
                printf("ID_INCOMPATIBLE_PROTOCOL_VERSION\n");
                break;
            case ID_REMOTE_DISCONNECTION_NOTIFICATION:
                printf("ID_REMOTE_DISCONNECTION_NOTIFICATION\n");
                break;
            case ID_REMOTE_CONNECTION_LOST:
                printf("ID_REMOTE_CONNECTION_LOST\n");
                break;
            case ID_REMOTE_NEW_INCOMING_CONNECTION:
                printf("ID_REMOTE_NEW_INCOMING_CONNECTION\n");
                break;
            case ID_CONNECTION_BANNED:
                printf("We are banned from this server.\n");
                break;
            case ID_CONNECTION_ATTEMPT_FAILED:
                printf("Connection attempt failed\n");
                break;
            case ID_NO_FREE_INCOMING_CONNECTIONS:
                printf("ID_NO_FREE_INCOMING_CONNECTIONS\n");
                break;

            case ID_INVALID_PASSWORD:
                printf("ID_INVALID_PASSWORD\n");
                break;

            case ID_CONNECTION_LOST:
                printf("ID_CONNECTION_LOST\n");
                break;

            case ID_CONNECTION_REQUEST_ACCEPTED:
                printf("ID_CONNECTION_REQUEST_ACCEPTED to %s with GUID %s\n", p->systemAddress.ToString(true), p->guid.ToString());
                printf("My external address is %s\n", client->GetExternalID(p->systemAddress).ToString(true));
                break;
            case ID_CONNECTED_PING:
            case ID_UNCONNECTED_PING:
                printf("Ping from %s\n", p->systemAddress.ToString(true));
                break;
            default:
                printf("%s\n", p->data);
                break;
            }
        }
    }
    client->Shutdown(300);

    // We're done with the network
    RakNet::RakPeerInterface::DestroyInstance(client);

    return 0;
}

unsigned char GetPacketIdentifier(RakNet::Packet *p)
{
    if (p==0)
        return 255;

    if ((unsigned char)p->data[0] == ID_TIMESTAMP)
    {
        RakAssert(p->length > sizeof(RakNet::MessageID) + sizeof(RakNet::Time));
        return (unsigned char) p->data[sizeof(RakNet::MessageID) + sizeof(RakNet::Time)];
    }
    else
        return (unsigned char) p->data[0];
}
