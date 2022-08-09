#include <enet/enet.h>
#include <iostream>
#include <thread>
#include <string>
#include <conio.h>

ENetAddress address;
ENetHost* server = nullptr;
ENetHost* client = nullptr;

void SendMessagesToClient(std::string serverName);
void SendMessagesToServer(std::string clientName);
bool CreateServer();
bool CreateClient();
void HandleServer();
void HandleClient();
void DestroyHosts();

int main(int argc, char** argv)
{
    if (enet_initialize() != 0)
    {
        fprintf(stderr, "An error occurred while initializing ENet.\n");
        std::cout << "An error occurred while initializing ENet." << std::endl;
        return EXIT_FAILURE;
    }

    atexit(enet_deinitialize);

    std::cout << "1) Create Server " << std::endl;
    std::cout << "2) Create Client " << std::endl;
    int UserInput;
    std::cin >> UserInput;

    if (UserInput == 1)
    {
        HandleServer();
    }
    else if (UserInput == 2)
    {
        HandleClient();
        
    }
    else
    {
        std::cout << "Invalid Input" << std::endl;
    }
    DestroyHosts();
    return EXIT_SUCCESS;
}

bool CreateClient()
{
    client = enet_host_create(NULL,1, 2, 0, 0);
    return client != nullptr;
}

bool CreateServer()
{
    address.host = ENET_HOST_ANY;
    /* Bind the server to port 1234. */
    address.port = 1234;
    server = enet_host_create(&address, 32, 2, 0, 0);
    return server != nullptr;
}

void HandleServer()
{
    if (!CreateServer())
    {
        fprintf(stderr,
            "An error occurred while trying to create an ENet server host.\n");
        exit(EXIT_FAILURE);
    }
    std::string serverName;
    std::cout << "What is you user name? ";
    std::cin >> serverName;

    /* Wait up to 1000 milliseconds for an event. */

    std::thread* ServerToClient = nullptr;
    while (1)
    {
        ENetEvent event;
        while (enet_host_service(server, &event, 1000) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
                printf("A new client connected from %x:%u.\n",
                    event.peer->address.host,
                    event.peer->address.port);
                event.peer->data = (void*)("Client information");

                {
                    std::string connectionMessage = serverName + " has connected with you";
                    ENetPacket* packet = enet_packet_create(connectionMessage.c_str(),
                        strlen(connectionMessage.c_str()) + 1,
                        ENET_PACKET_FLAG_RELIABLE);
                    //enet_host_broadcast (server, 0, packet);         
                    enet_peer_send(event.peer, 0, packet);
                    enet_host_flush(server);

                }
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                std::cout <<
                    "Incoming Message: " << std::endl
                    << (char*)event.packet->data
                    << std::endl;
                enet_packet_destroy(event.packet);
                if (ServerToClient == nullptr)
                {
                    std::thread* ServerToClient = new std::thread(SendMessagesToClient, serverName);
                }
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                printf("%s disconnected.\n", (char*)event.peer->data);
                /* Reset the peer's client information. */
                event.peer->data = NULL;
            }

        }
        if (ServerToClient != nullptr)
        {
            ServerToClient->join();
        }
    }
}

void HandleClient()
{
    std::string clientName;
    std::cout << "What is you user name? ";
    std::cin >> clientName;
    if (!CreateClient())
    {
        fprintf(stderr,
            "An error occurred while trying to create an ENet client host.\n");  
        exit(EXIT_FAILURE);
    }
    ENetAddress address;
    ENetEvent event;
    ENetPeer* peer;
    /* Connect to some.server.net:1234. */
    enet_address_set_host(&address, "127.0.0.1");
    address.port = 1234;
    /* Initiate the connection, allocating the two channels 0 and 1. */
    peer = enet_host_connect(client, &address, 2, 0);
    if (peer == NULL)
    {
        fprintf(stderr,
            "No available peers for initiating an ENet connection.\n");
        exit(EXIT_FAILURE);
    }
    /* Wait up to 5 seconds for the connection attempt to succeed. */
    if (enet_host_service(client, &event, 5000) > 0 &&
        event.type == ENET_EVENT_TYPE_CONNECT)
    {
        std::cout << "Connection to 127.0.0.1:1234 succeeded." << std::endl;
    }
    else
    {
        /* Either the 5 seconds are up or a disconnect event was */
        /* received. Reset the peer in the event the 5 seconds   */
        /* had run out without any significant event.            */
        enet_peer_reset(peer);
        std::cout << "Connection to 127.0.0.1:1234 failed." << std::endl;
    }

    while (1)
    {
        ENetEvent event;
        std::thread* ClientToServer = nullptr;
        while (enet_host_service(client, &event, 100000) > 0)
        {
            {
                std::string connectionMessage = clientName + " has connected with you";
                ENetPacket* packet = enet_packet_create(connectionMessage.c_str(),
                    strlen(connectionMessage.c_str()) + 1,
                    ENET_PACKET_FLAG_RELIABLE);
                enet_host_broadcast(client, 0, packet);
                enet_host_flush(client);
            }
            switch (event.type)
            {
            case ENET_EVENT_TYPE_RECEIVE:
                std::cout <<
                    "Incoming Message: " << std::endl
                    << (char*)event.packet->data
                    << std::endl;
                enet_packet_destroy(event.packet);
                if (ClientToServer == nullptr)
                {
                    ClientToServer = new std::thread(SendMessagesToServer, clientName);
                }
            }

        }
        if (ClientToServer != nullptr)
        {
            ClientToServer->join();
        }
    }
}

void SendMessagesToClient(std::string serverName)
{
    std::string message = "";
    while (message != "quit")
    {
        if (_kbhit())
        {
            getline(std::cin, message);
            std::string messageToSend = serverName + ": " + message;
            ENetPacket* packet = enet_packet_create(messageToSend.c_str(),
                messageToSend.length() + 1,
                ENET_PACKET_FLAG_RELIABLE);
            //enet_peer_send(event.peer, 0, packet);
            enet_host_broadcast(server, 0, packet);
            enet_host_flush(server);
        }
    }
}

void SendMessagesToServer(std::string clientName)
{
    std::string message = "";
    while (message != "quit")
    {
        if (_kbhit())
        {
            getline(std::cin, message);
            std::string messageToSend = clientName + ": " + message;
            ENetPacket* packet = enet_packet_create(messageToSend.c_str(),
                messageToSend.length() + 1,
                ENET_PACKET_FLAG_RELIABLE);
            //enet_peer_send(event.peer, 0, packet);
            enet_host_broadcast(client, 0, packet);
            enet_host_flush(client);
        }
    }
}

void DestroyHosts()
{
    if (server != nullptr)
    {
        enet_host_destroy(server);
    }

    if (client != nullptr)
    {
        enet_host_destroy(client);
    }
}