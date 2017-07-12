#ifndef COMMUNICATION_H
#define COMMUNICATION_H

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
#include "cereal/types/vector.hpp"

#include "inventory_manager.h"

#define MSG_LENGTH 256

namespace robot
{

class Socket
{
    protected:
        int sockfd, portno, n;
        struct sockaddr_in serv_addr;
        struct hostent *server;
        char buffer[MSG_LENGTH]; // Message buffer
        size_t len; // Length of message
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
        void send(std::string message);
};

class Server: public Socket
{
    private:
        struct sockaddr_in cli_addr;
        socklen_t clilen;
        pid_t pID;
    public:
        Server(std::string host, int portno);
        void serve(std::function<void(char*, int)> callback_func);
        void shutdown();
};

class Message
{
    protected:
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
    private:
        std::vector<ItemType> items;
        std::vector<int> quantities;
    public:
        Order(std::vector<ItemType> items, std::vector<int> quantities);
        int get_count(unsigned int position);
        ItemType get_item(unsigned int position);
        int get_num_components();
        void serialize();
};

class Status: public Message
{
    /* Status codes:
     * -99: Unknown error
     * -2: Battery low
     * -1: Robot unavailable/powered off
     * 0: Ready to accept new orders
     * 1: Delivering order
     * 2: Waiting for pickup
     * 3: Returning to base
     * 4: Dispensing
     */
    public:
        Status(std::string status);
};

}

#endif
