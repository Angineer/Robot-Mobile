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

Client::Client(std::string host, int portno) : Socket(host, portno)
{
    server = gethostbyname(host.c_str());
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
}
void Client::connect_client(){

    int connect_success = connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

    if (connect_success < 0){
        std::cout << "ERROR connecting to server!" << std::endl;
    }
}
void Client::send(std::string message){
    // TODO: expand for messages of more than MSG_LENGTH bytes

    // First send the message length
    len = message.length();

    memcpy(buffer, &len, sizeof(size_t));
    n = write(sockfd, buffer, sizeof(size_t));
    if (n < 0)
         std::cout << "ERROR writing to socket" << std::endl;

    // Then send the actual message
    memcpy(buffer, message.c_str(), len);
    n = write(sockfd, buffer, len);
    if (n < 0)
         std::cout << "ERROR writing to socket" << std::endl;
}
void Client::disconnect(){
    close(sockfd);
}

Server::Server(std::string host, int portno) : Socket(host, portno)
{
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    int bind_success = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

    if (bind_success < 0){
        std::cout << "ERROR binding to socket!" << std::endl;
        exit(1);
    }
}
void Server::serve(std::function<void(char*, int)> callback_func){
    std::signal(SIGCHLD, SIG_IGN); // Let child processes fully die

    // Server runs forever
    while (true){
        std::cout << "Listening for connections" << std::endl;
        listen(sockfd, 5);
        clilen = sizeof(cli_addr);
        int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

        if (newsockfd > 0){
            std::cout << "Connection established!" << std::endl;

            pID = fork(); // Create child process

            if (pID == 0){
                std::cout << "Child process started!" << std::endl;
                close(sockfd);
                // Process data until disconnect
                bool connect = true;
                while(connect){
                    // Clear buffer
                    memset(buffer, 0, MSG_LENGTH);

                    // Read length of message from socket
                    n = read(newsockfd, buffer, sizeof(size_t));
                    memcpy(&len, buffer, sizeof(size_t));
                    if (n < 0)
                        std::cout << "ERROR reading from socket!" << std::endl;

                    // Then read message
                    n = read(newsockfd, buffer, len);
                    if (n < 0)
                        std::cout << "ERROR reading from socket!" << std::endl;

                    // Check for disconnect
                    if ((n == 0)){
                        close(newsockfd);
                        std::cout << "Disconnected!" << std::endl;
                        connect = false;
                    }
                    else{
                        // Call callback using input
                        callback_func(buffer, len);
                    }
                }
                exit(0);
            }
            else{
                close(newsockfd);
            }
        }
    }
}
void Server::shutdown(){
    close(sockfd);
}

std::string Message::get_serial(){
    return this->serial;
}

Command::Command(std::string command){
    this->serial = "c" + command;
}

Order::Order(std::vector<ItemType> items, std::vector<int> quantities){
    {
        cereal::BinaryOutputArchive oarchive(ss); // Create an output archive

        oarchive(items, quantities); // Write the data to the archive
    }

    std::string serial_str = ss.str();

    this->serial = "o" + serial_str;
}

Status::Status(){
    //foo
}

}
