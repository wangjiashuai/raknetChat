
#include <iostream>

#include "Source/MessageIdentifiers.h"

#include "Source/RakPeerInterface.h"
#include "Source/RakNetStatistics.h"
#include "Source/RakNetTypes.h"
#include "Source/BitStream.h"
#include "Source/RakSleep.h"
#include "Source/PacketLogger.h"
#include <assert.h>
#include <cstdio>
#include <cstring>
#include <stdlib.h>
#include "Source/Kbhit.h"
#include <stdio.h>
#include <string.h>
#include "Source/Gets.h"
//这是一个注释

unsigned char GetPacketIdentifier(RakNet::Packet *p);
int main(void)
{
    printf("raknet Server\n");
    RakNet::RakPeerInterface *server = RakNet::RakPeerInterface::GetInstance();

    RakNet::RakNetStatistics *rss;
    server->SetIncomingPassword("Rumpelstiltskin", (int)strlen("Rumpelstiltskin"));
    server->SetTimeoutTime(30000,RakNet::UNASSIGNED_SYSTEM_ADDRESS);


    // Holds packets
    RakNet::Packet* p;
    unsigned char packetIdentifier;
    RakNet::SystemAddress clientID = RakNet::UNASSIGNED_SYSTEM_ADDRESS;
    char portstring[30];
    strcpy(portstring, "1234");

    RakNet::SocketDescriptor socketDescriptors[2];
    socketDescriptors[0].port = atoi(portstring);
    socketDescriptors[0].socketFamily = AF_INET;
    socketDescriptors[1].port = atoi(portstring);
    socketDescriptors[1].socketFamily = AF_INET6;
    bool b = server->Startup(4, socketDescriptors, 2 ) == RakNet::RAKNET_STARTED;
    server->SetMaximumIncomingConnections(4);

    if (!b)
    {
        printf("Failed to start dual IPV4 and IPV6 ports. Trying IPV4 only.\n");
        b = server->Startup(4, socketDescriptors, 1 )==RakNet::RAKNET_STARTED;
        if (!b)
        {
            puts("Server failed to start.  Terminating.");
            exit(1);
        }
    }


    DataStructures::List< RakNet::RakNetSocket2* > sockets;
    server->GetSockets(sockets);
    printf("Socket addresses used by RakNet:\n");
    for (unsigned int i=0; i < sockets.Size(); i++)
    {
        printf("%i. %s\n", i+1, sockets[i]->GetBoundAddress().ToString(true));
    }

    printf("\nMy IP addresses:\n");
    for (unsigned int i=0; i < server->GetNumberOfAddresses(); i++)
    {
        RakNet::SystemAddress sa = server->GetInternalID(RakNet::UNASSIGNED_SYSTEM_ADDRESS, i);
        printf("%i. %s (LAN=%i)\n", i+1, sa.ToString(false), sa.IsLANAddress());
    }

    printf("\nMy GUID is %s\n", server->GetGuidFromSystemAddress(RakNet::UNASSIGNED_SYSTEM_ADDRESS).ToString());

    char message[2048];
    while(1){
        RakSleep(30);

        if (kbhit())
        {
            Gets(message,sizeof(message));
        }

        if (strcmp(message, "quit")==0)
        {
            puts("Quitting.");
            break;
        }

        if (strcmp(message, "stat")==0)
        {
            rss = server->GetStatistics(server->GetSystemAddressFromIndex(0));
            StatisticsToString(rss, message, 2);
            printf("%s", message);
            printf("Ping %i\n", server->GetAveragePing(server->GetSystemAddressFromIndex(0)));

            continue;
        }

        if (strcmp(message, "ping")==0)
        {
            server->Ping(clientID);

            continue;
        }

        if (strcmp(message, "pingip")==0)
        {
            printf("Enter IP: ");
            Gets(message,sizeof(message));
            printf("Enter port: ");
            Gets(portstring,sizeof(portstring));
            if (portstring[0]==0)
                strcpy(portstring, "1234");
            server->Ping(message, atoi(portstring), false);

            continue;
        }

        if (strcmp(message, "kick")==0)
        {
            server->CloseConnection(clientID, true, 0);

            continue;
        }

        if (strcmp(message, "getconnectionlist")==0)
        {
            RakNet::SystemAddress systems[10];
            unsigned short numConnections=10;
            server->GetConnectionList((RakNet::SystemAddress*) &systems, &numConnections);
            for (int i=0; i < numConnections; i++)
            {
                printf("%i. %s\n", i+1, systems[i].ToString(true));
            }
            continue;
        }

        if (strcmp(message, "ban")==0)
        {
            printf("Enter IP to ban.  You can use * as a wildcard\n");
            Gets(message,sizeof(message));
            server->AddToBanList(message);
            printf("IP %s added to ban list.\n", message);

            continue;
        }


        char message2[2048];
        message2[0]=0;
        strcpy(message2, "Server: ");
        strcat(message2, message);

        // message2 is the data to send
        // strlen(message2)+1 is to send the null terminator
        // HIGH_PRIORITY doesn't actually matter here because we don't use any other priority
        // RELIABLE_ORDERED means make sure the message arrives in the right order
        // We arbitrarily pick 0 for the ordering stream
        // RakNet::UNASSIGNED_SYSTEM_ADDRESS means don't exclude anyone from the broadcast
        // true means broadcast the message to everyone connected
        if(strlen(message))
            server->Send(message2, (const int) strlen(message2)+1, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);


        for (p=server->Receive(); p; server->DeallocatePacket(p), p=server->Receive())
        {
            packetIdentifier = GetPacketIdentifier(p);
            switch (packetIdentifier)
            {
            case ID_DISCONNECTION_NOTIFICATION:
                printf("ID_DISCONNECTION_NOTIFICATION from %s\n", p->systemAddress.ToString(true));;
                break;


            case ID_NEW_INCOMING_CONNECTION:
                printf("ID_NEW_INCOMING_CONNECTION from %s with GUID %s\n", p->systemAddress.ToString(true), p->guid.ToString());
                clientID=p->systemAddress; // Record the player ID of the client

                printf("Remote internal IDs:\n");
                for (int index=0; index < MAXIMUM_NUMBER_OF_INTERNAL_IDS; index++)
                {
                    RakNet::SystemAddress internalId = server->GetInternalID(p->systemAddress, index);
                    if (internalId!=RakNet::UNASSIGNED_SYSTEM_ADDRESS)
                    {
                        printf("%i. %s\n", index+1, internalId.ToString(true));
                    }
                }

                break;

            case ID_INCOMPATIBLE_PROTOCOL_VERSION:
                printf("ID_INCOMPATIBLE_PROTOCOL_VERSION\n");
                break;

            case ID_CONNECTED_PING:
            case ID_UNCONNECTED_PING:
                printf("Ping from %s\n", p->systemAddress.ToString(true));
                break;

            case ID_CONNECTION_LOST:
                printf("ID_CONNECTION_LOST from %s\n", p->systemAddress.ToString(true));;
                break;

            default:
                printf("%s\n", p->data);

                sprintf(message, "%s", p->data);
                server->Send(message, (const int) strlen(message)+1, HIGH_PRIORITY, RELIABLE_ORDERED, 0, p->systemAddress, true);

                break;
            }

        }
        memset(message,0,sizeof(message));
    }

    server->Shutdown(300);
    // We're done with the network
    RakNet::RakPeerInterface::DestroyInstance(server);
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
