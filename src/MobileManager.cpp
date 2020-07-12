#include "MobileManager.h"

#include <iostream>
#include <chrono>
#include <mutex>
#include <thread>

#include "cereal/cereal.hpp"
#include "cereal/archives/binary.hpp"
#include "cereal/types/string.hpp"
#include "cereal/types/map.hpp"

#include "MobileConfiguration.h"

MobileManager::MobileManager() :
    arduino ( "/dev/ttyACM0" ),
    state ( State::IDLE ),
    server ( SocketType::BLUETOOTH ),
    fetcher ( [ this ] ( int id ) {
                  this->handle_cam_update ( id );
              } )
{
    arduino.sendByte ( 'h' ); // halt
}

void MobileManager::run()
{
    // Run server and process callbacks
    auto callback_func = [ this ] ( std::string input ) {
                             return this->handle_input ( input );
                         };
    server.serve ( callback_func );
}

std::string MobileManager::handle_input ( const std::string& input ){
    //std::cout << "Received message: " << input << std::endl;
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
        std::string destinationStr;

        {
            cereal::BinaryInputArchive iarchive ( ss ); // Create an input archive
            iarchive ( destinationStr, items ); // Read the data from the archive
        }

        std::lock_guard<std::mutex> lock ( access_mutex );
        destination = config.getConfig<int> ( destinationStr + "_id" );

        std::cout << "New order received; headed to "
                  << destinationStr << " (" << destination << ")"
                  << std::endl;

        state = State::DELIVER;
        arduino.sendByte ( 'd' ); // drive
    }

    return response;
}

void MobileManager::handle_cam_update ( int location_id )
{
    std::cout << "Location update: " << location_id << std::endl;
    std::lock_guard<std::mutex> lock ( access_mutex );

    if ( state == State::DELIVER &&
         location_id == destination ) {
        // If we are making a delivery and have reached our destination, stop
        // for the user to grab their snack, then head back home
        arduino.sendByte ( 'h' );
        state = State::RETURN;
        std::this_thread::sleep_for ( std::chrono::seconds (
            config.getConfig<int> ( "sleep_time_s" ) ) );
        arduino.sendByte ( 'd' );
    } else if ( state == State::RETURN &&
                location_id == config.getConfig<int> ( "home_id" ) ) {
        // If we are on our way back from a delivery and have arrived at home,
        // stop and await new instructions
        arduino.sendByte ( 'h' );
        state = State::IDLE;
    }

}
