#include "Server.h"

#include <iostream>

std::string handle_input ( std::string input ){
    std::cout << "Received message: " << input << std::endl;;
    return "OK";
}

int main(int argc, char *argv[])
{
    std::cout << "Starting bluetooth server..." << std::endl;
    Server server ( SocketType::BLUETOOTH );

    // Run server and process callbacks
    server.serve ( handle_input );
}
