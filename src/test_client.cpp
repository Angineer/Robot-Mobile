#include "../include/communication.h"

robot::Client client("localhost", 5000);

void shutdown(int signum){
    client.disconnect();
    std::cout << "\nStopping Test Client" << std::endl;
    exit(0);
}

int main(int argc, char *argv[])
{
    // Kill client gracefully on ctrl+c
    std::signal(SIGINT, shutdown);

    std::cout << "Start" << std::endl;
    char buffer[256];

    while (true){
        client.connect_client();
        std::cout << "Client connected" << std::endl;
        bool connect = true;

        while (connect){

            printf("Please enter the message: ");

            bzero(buffer,256);
            fgets(buffer,255,stdin);

            std::string str_message(buffer);
            robot::Command command(str_message);

            client.send((char *)command.get_serial().c_str());
            std::cout << "Message sent" << std::endl;
        }
    }
    /*
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");


    printf("Please enter the message: ");
    bzero(buffer,256);
    fgets(buffer,255,stdin);
    n = write(sockfd,buffer,strlen(buffer));
    if (n < 0)
         error("ERROR writing to socket");
    bzero(buffer,256);
    n = read(sockfd,buffer,255);
    if (n < 0)
         error("ERROR reading from socket");
    printf("%s\n",buffer);
    close(sockfd);
    return 0;
    */
}
