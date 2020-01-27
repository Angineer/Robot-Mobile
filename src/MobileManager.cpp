#include "MobileManager.h"

#include <iostream>
#include <chrono>
#include <mutex>
#include <thread>

#include "cereal/cereal.hpp"
#include "cereal/archives/binary.hpp"
#include "cereal/types/string.hpp"
#include "cereal/types/map.hpp"

MobileManager::MobileManager() :
    arduino ( "/dev/ttyACM0" ),
    state ( State::IDLE ),
    server ( SocketType::BLUETOOTH ),
    checker ( "/home/pi/camera.bmp",
              [ this ] ( int id ) {
                  this->handle_cam_update ( id );
              } )
{}

void MobileManager::run()
{
    // Run server and process callbacks
    auto callback_func = [ this ] ( std::string input ) {
                             return this->handle_input ( input );
                         };
    server.serve ( callback_func );
}

std::string MobileManager::handle_input ( const std::string& input ){
    std::cout << "Received message: " << input << std::endl;
    std::string response { "OK" };

    char type = input[0];
    if ( type == 'c' ){
        std::string command = input.substr(1, std::string::npos);
        if ( command == "status" ) {
            std::lock_guard<std::mutex> lock ( access_mutex );
            response = stateToString ( state );
        }
    } else if ( type == 'o' ) {
        // Read in new order so Robie knows where he's headed
        std::string order = input.substr ( 1, std::string::npos );
        std::stringstream ss ( order );
        std::map<std::string, int> items;

        {
            cereal::BinaryInputArchive iarchive ( ss ); // Create an input archive
            iarchive ( destination, items ); // Read the data from the archive
        }

        std::cout << "Headed to " << destination << std::endl;

        std::lock_guard<std::mutex> lock ( access_mutex );
        state = State::DELIVER;
    }

    return response;
}

void MobileManager::handle_cam_update ( int location_id )
{
    //TODO
    std::cout << "Location update: " << location_id << std::endl;
}
