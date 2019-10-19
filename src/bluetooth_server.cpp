#include "Server.h"
#include "State.h"

#include <iostream>
#include <chrono>
#include <thread>

State status { State::IDLE };

std::string handle_input ( std::string input ){
    std::cout << "Received message: " << input << std::endl;;
    char type = input[0];
    if ( type == 'c' ){
        std::string command = input.substr(1, std::string::npos);
        if ( command == "status" ) {
            if ( status == State::IDLE ) return "IDLE";
            if ( status == State::DISPENSE ) return "DISPENSE";
            if ( status == State::DELIVER ) return "DELIVER";
            if ( status == State::ERROR ) return "ERROR";
        }
    } else if ( type == 'o' ) {
        // TODO: How does Robie know where to go?
        // TODO: How does Robie know when the base is done dispensing?
        status = State::DISPENSE;
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        status = State::DELIVER;
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        status = State::IDLE;

    }
}

int main(int argc, char *argv[])
{
    std::cout << "Starting bluetooth server..." << std::endl;
    Server server ( SocketType::BLUETOOTH );

    // Run server and process callbacks
    server.serve ( handle_input );
}
