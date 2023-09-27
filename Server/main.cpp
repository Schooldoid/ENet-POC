#include <enet/enet.h>
#include <iostream>
#include <utility>
#include <map>

class ClientData{
private:
    int m_id;
    std::string m_username;

public:
    ClientData(int id) : m_id(id) {}

    void set_username(std::string username) {m_username = std::move(username);}

    int get_id(){return m_id;}
    std::string get_username(){return m_username;}

};

std::map<int, ClientData*> client_map;

void broadcast_packet(ENetHost* server, const char* data){
    ENetPacket* packet = enet_packet_create(data, strlen(data)+1, ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast(server, 0, packet);
}

void send_assured_packet(ENetPeer* peer, const char* data)
{
    // +1 for 0 at end of string
    // packet will be reliably send
    ENetPacket* packet = enet_packet_create(data, strlen(data) + 1, ENET_PACKET_FLAG_RELIABLE);
    // channel 0
    enet_peer_send(peer, 0, packet);
}

void parse_data(ENetHost* server, int id, char* data){
    std::cout << "PARSING: " << data << '\n';

    int data_type;
    sscanf(data, "%d|", &data_type);

    switch(data_type){
        case 1: {
            char msg[80];
            sscanf(data, "%*d|%[^\n]", &msg);

            char send_data[1024] = {'\0'};
            sprintf(send_data, "1|%d|%s", id, msg);

            std::cout << "PARSED: " << client_map[id]->get_username() << ": " << msg << '\n' << '\n';
            broadcast_packet(server, send_data);
            break;
        }
        case 2:{
            char username[80];
            sscanf(data, "2|%[^\n]", &username);

            char send_data[1024] = {'\0'};
            sprintf(send_data, "2|%d|%s", id, username);
            std::cout << "BROADCASTED: " << send_data << '\n' << '\n';

            broadcast_packet(server, send_data);
            client_map[id]->set_username(username);

            break;
        }
    }
}

int main (int argc, char ** argv)
{
    std::cout << "---SERVER--- \n\n";

    if (enet_initialize() != 0)
    {
        std::cout << "An error occurred while initializing ENet.\n";
        return EXIT_FAILURE;
    }
    atexit (enet_deinitialize);

    ENetAddress address;
    ENetHost* server;
    ENetEvent event;


    std::cout << "Submit port to be used (7777 can be used for testing):\n";
    std::cin >> address.port;

    address.host = ENET_HOST_ANY; // bit schitzo, idk 100% what it do
    // address.port = 7777;

    server = enet_host_create(&address, /* connect to the address */
                              3,    /* amount of peers you want to connect */
                              1,    /* 1 channel for simplification */
                              0     /* any amount of incoming bandwidth */,
                              0     /* any amount of outgoing bandwidth */);

    if (server == NULL)
    {
        fprintf (stderr,
                 "An error occurred while trying to create an ENet server host.\n");
        exit (EXIT_FAILURE);
    }


    // <GAME LOOP>

    int new_player_id = 0;
    while(true) { // why? because suck a cock
        while(enet_host_service(server, &event, 1000) > 0)
        {
            switch(event.type){
                case ENET_EVENT_TYPE_CONNECT: {
                    //incoming connection
                    printf("new client connected from %x:%u. \n",
                           event.peer->address.host,
                           event.peer->address.port);

                    for (auto const &x: client_map) {
                        char send_data[1024] = {'\0'};
                        sprintf(send_data, "2|%d|%s", x.first, x.second->get_username().c_str());

                        broadcast_packet(server, send_data);
                    }

                    new_player_id++;
                    client_map[new_player_id] = new ClientData(new_player_id);
                    event.peer->data = client_map[new_player_id];

                    char data_to_send[126] = {'\0'};
                    sprintf(data_to_send, "3|%d", new_player_id);

                    send_assured_packet(event.peer, data_to_send);
                    break;
                }
                case ENET_EVENT_TYPE_RECEIVE: {
                    parse_data(server, static_cast<ClientData *>(event.peer->data)->get_id(),
                               reinterpret_cast<char *>(event.packet->data));
                    enet_packet_destroy(event.packet);
                    break;
                }
                case ENET_EVENT_TYPE_DISCONNECT: {
                    printf("%x:%u disconnected. \n",
                           event.peer->address.host,
                           event.peer->address.port);

                    char disconnected_data[126] = {'\0'};
                    sprintf(disconnected_data, "4|%d", static_cast<ClientData*>(event.peer->data)->get_id());
                    broadcast_packet(server, disconnected_data);

                    event.peer -> data = NULL;
                    break;
                }
                case ENET_EVENT_TYPE_NONE:
                    break;
            }
        }
    }

    // </GAME LOOP>

    // destroy the funny server
    enet_host_destroy(server);

    return EXIT_SUCCESS;
}