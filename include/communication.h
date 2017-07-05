#include <csignal>
#include <functional>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sstream>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "cereal/cereal.hpp"
#include "cereal/archives/binary.hpp"
#include "cereal/types/string.hpp"
#include "inventory_manager.h"

namespace robot
{

class Socket
{
    protected:
        int sockfd, portno, n;
        struct sockaddr_in serv_addr;
        struct hostent *server;
        char buffer[256];
    public:
        Socket(std::string host, int portno);
};

class Client: public Socket
{
    private:
        struct hostent *server;
    public:
        Client(std::string host, int portno);
        void connect_client();
        void disconnect();
        void send(char* buffer);
};

class Server: public Socket
{
    private:
        struct sockaddr_in cli_addr;
        socklen_t clilen;
    public:
        Server(std::string host, int portno);
        void serve(std::function<void(char*)> callback_func);
        void shutdown();
};

class Message
{
    protected:
        char size;
        std::string serial;

    public:
        std::string get_serial();
};

class Command: public Message
{
    public:
        Command(std::string command);
};

class Order: public Message
{
    public:
        Order(ItemType item, int quantity);
};

class Status: public Message
{
    public:
        Status();
};

}
