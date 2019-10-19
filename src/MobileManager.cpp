#include "MobileManager.h"

#include <iostream>
#include <chrono>
#include <mutex>
#include <thread>

MobileManager::MobileManager() :
    server ( SocketType::BLUETOOTH ),
    state ( State::IDLE )
{}

void MobileManager::run()
{
    // Bind function for processing inputs
    std::function<std::string ( std::string )> callback_func (
        bind ( &MobileManager::handle_input, this, std::placeholders::_1 ) );

    // Run server and process callbacks
    server.serve ( callback_func );
}

std::string MobileManager::handle_input ( const std::string& input ){
    std::cout << "Received message: " << input << std::endl;;
    char type = input[0];
    if ( type == 'c' ){
        std::string command = input.substr(1, std::string::npos);
        if ( command == "status" ) {
            std::lock_guard<std::mutex> lock ( access_mutex );
            if ( state == State::IDLE ) return "IDLE";
            if ( state == State::DISPENSE ) return "DISPENSE";
            if ( state == State::DELIVER ) return "DELIVER";
            if ( state == State::ERROR ) return "ERROR";
        }
    } else if ( type == 'o' ) {
        // TODO: How does Robie know where to go?
        // TODO: How does Robie know when the base is done dispensing?
        std::lock_guard<std::mutex> lock ( access_mutex );
        state = State::DISPENSE;
    }
}
