#include <iostream>
#include <csignal>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

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

class ClientSock: public Socket
{
    private:
        struct hostent *server;
    public:
        ClientSock(std::string host, int portno);
        void connect_client();
        void disconnect();
        void send(char* buffer);
};

class ServerSock: public Socket
{
    private:
        struct sockaddr_in cli_addr;
        socklen_t clilen;
    public:
        ServerSock(std::string host, int portno);
        void serve();
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
        Order();
};

class Status: public Message
{
    public:
        Status();
};

}
