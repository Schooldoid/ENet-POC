#include <enet/enet.h>
#include <iostream>
#include <string>
#include <pthread.h>
#include <vector>
#include <map>

const std::vector<std::string> messages = { "Bericht: 0", "Bericht: 1", "Bericht: 2", "Bericht: 3", "Bericht: 4" };
static int CLIENT_ID = -1;

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

void send_assured_packet(ENetPeer* peer, const char* data)
{
    // +1 for 0 at end of string
    // packet will be reliably send
    ENetPacket* packet = enet_packet_create(data, strlen(data) + 1, ENET_PACKET_FLAG_RELIABLE);
    // channel 0
    enet_peer_send(peer, 0, packet);
}

void parse_data(char* data){
    std::cout << "PARSING: " << data << '\n';

    int data_type;
    int id;
    sscanf(data, "%d|%d", &data_type, &id);

    switch(data_type){
        case 1:
            if(id != CLIENT_ID){
                char msg[80];
                sscanf(data, "%*d|%*d|%[^|]", &msg);

                std::cout << client_map[id]->get_username() << ": " << msg << '\n';
            }
            break;
        case 2:{
            if(id != CLIENT_ID){
                char username[80];
                sscanf(data, "%*d|%*d|%[^|]", &username);

                client_map[id] = new ClientData(id);
                client_map[id]->set_username(username);
            }
            break;
        }
        case 3:{
            CLIENT_ID = id;
            break;
        }
    }
}

// should probably be in its own thread
void* msg_loop(ENetHost* client) {
    while(true){
        ENetEvent event;
        /* This will sleep the code until the time has passed */
        while(enet_host_service(client, &event, 1000) > 0){
            switch(event.type){
                case ENET_EVENT_TYPE_RECEIVE:
                    parse_data(reinterpret_cast<char *>(event.packet->data));
                    enet_packet_destroy(event.packet);
                    break;
                    case ENET_EVENT_TYPE_NONE:
                        break;
                    case ENET_EVENT_TYPE_CONNECT:
                        break;
                    case ENET_EVENT_TYPE_DISCONNECT:
                        // SERVER CAN SHUT DOWN, SO THERE SHOULD BE CODE FOR THAT HERE
                        break;
            }
        }
    }
}

int main (int argc, char ** argv)
{
    if (enet_initialize () != 0)
    {
        std::cout << "An error occurred while initializing ENet.\n";
        return EXIT_FAILURE;
    }
    atexit (enet_deinitialize);

    std::cout << "Please enter username.\n";
    std::string username;
    std::cin >> username;

    ENetHost* client;
    client = enet_host_create(NULL, /* Null, so we create a client */
                              1,    /* Only connection will be server */
                              1,    /* 1 channel for simplification */
                              0     /* any amount of incoming bandwidth */,
                              0     /* any amount of outgoing bandwidth */);

    if (client == NULL)
    {
        fprintf (stderr,
                 "An error occurred while trying to create an ENet client host.\n");
        exit (EXIT_FAILURE);
    }

    ENetAddress address;
    ENetEvent event;
    ENetPeer* peer; /* In this case the server (ties in with outgoing connection) */

    std::string ip;
    std::cout << "Submit IP to be used (127.0.0.1 can be used for local testing):\n";
    std::cin >> ip;

    enet_address_set_host(&address, ip.c_str()); /* Ip address to connect to (pretty selfexplanatory probably) */

    std::cout << "Submit port to be used (7777 can be used for testing):\n";
    std::cin >> address.port;

    address.port = 7777;

    peer = enet_host_connect(client,
                             &address,
                             1 /* Amount of channels */,
                             0 /* Any data we want to send immediately */);

    if(peer == NULL){
        fprintf (stderr,
                 "No available peers for initiating ENet connection.\n");
        exit (EXIT_FAILURE);
    }

    //check for response from host
    if(enet_host_service(client,
                         &event, /* will be filled with response */
                         5000    /* Amount of milliseconds to wait for response */) > 0 && event.type == ENET_EVENT_TYPE_CONNECT)
    {
        std::cout << "Connection to 127.0.0.1:7777 succeeded. \n\n";
    }
    else {
        enet_peer_reset(peer);
        std::cout << "Connection to 127.0.0.1:7777 failed. \n\n";
        return EXIT_SUCCESS; // Connection failed, but the program did not. (error message)
    }

    // <GAME LOOP>

    //send SERVER the user's username
    char str_data[80] = "2|";
    strcat(str_data, username.c_str());
    send_assured_packet(peer, str_data);

    // create thread for recieving data
    pthread_t thread;
    pthread_create(&thread, NULL, reinterpret_cast<void *(*)(void *)>(msg_loop), client);

    username.append(": ");
    for (auto & message : messages)
    {
        char msg_data[80] = "1|";
        strcat(msg_data, message.c_str());
        send_assured_packet(peer, msg_data);

        Sleep(5000);
    }

    // close thread
    pthread_join(thread, NULL);
    // </GAME LOOP>

    //disconnecting
    enet_peer_disconnect(peer, 0 /* last data to send */);

    // wait until server responded to our disconnect
    while(enet_host_service(client, &event, 3000) > 0)
    {
        switch(event.type){
            case ENET_EVENT_TYPE_RECEIVE:
                // we want to disconnect, so we dont care about these packets anymore
                enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_NONE:
                break;
            case ENET_EVENT_TYPE_CONNECT:
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                std::cout << "Disconnection succeeded. \n\n";
                break;
        }
    }

    return EXIT_SUCCESS;
}