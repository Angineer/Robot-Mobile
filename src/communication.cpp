#include "../include/communication.h"

namespace robot
{

Socket::Socket(std::string host, int portno)
{
    portno = portno;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        std::cout << "ERROR opening socket" << std::endl;
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
}

ClientSock::ClientSock(std::string host, int portno) : Socket(host, portno)
{
    server = gethostbyname(host.c_str());
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
}
void ClientSock::connect_client(){

    int connect_success = connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

    if (connect_success < 0){
        std::cout << "ERROR connecting to server!" << std::endl;
    }
}
void ClientSock::send(char* buffer){
    n = write(sockfd, buffer, strlen(buffer));
    if (n < 0)
         std::cout << "ERROR writing to socket" << std::endl;
}
void ClientSock::disconnect(){
    close(sockfd);
}

ServerSock::ServerSock(std::string host, int portno) : Socket(host, portno)
{
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    int bind_success = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

    if (bind_success < 0){
        std::cout << "ERROR binding to socket!" << std::endl;
    }
}
void ServerSock::serve(){
    // Server runs forever
    while (true){
        std::cout << "Listening for connections" << std::endl;
        listen(sockfd, 5);
        clilen = sizeof(cli_addr);
        int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

        if (newsockfd > 0){
            std::cout << "Connection established!" << std::endl;
        }

        // Process data until disconnect
        bool connect = true;
        while(connect){
            memset(buffer, 0, 8); //TODO: Fix message size

            n = read(newsockfd, buffer, 8);

            if (n == 0){
                connect = false;
            }

            std::cout << n << std::endl;
            std::cout << buffer;

            // Disconnect
            if ((buffer[0] == 'd') || (n == 0)){
                close(newsockfd);
                std::cout << "Disconnected!" << std::endl;
                connect = false;
            }
        }
    }
    close(sockfd);
}

std::string Message::get_serial(){
    return this->serial;
}

Command::Command(std::string command){
    this->size = sizeof(command);
    std::string size_str = std::to_string(size);
    this->serial = "c" + size_str + command;
}

Order::Order(){
    this->size = 8;
    std::string size_str = std::to_string(size);
    this->serial = "o" + size_str + "1a";
}

Status::Status(){
    //foo
}

}
