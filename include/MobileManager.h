#ifndef MOBILE_MANAGER_H
#define MOBILE_MANAGER_H

#include <mutex>

#include <Arduino.h>
#include <Server.h>
#include <State.h>

class MobileManager {
public:
    // Constructor
    MobileManager();

    // Run the Manager
    void run();
private:
    // Handle input from connections on the bluetooth server
    std::string handle_input ( const std::string& input );

    // Arduino that does the low-level motor control and sensing
    Arduino arduino;

    // Mutex for managing access to current state
    std::mutex access_mutex;

    // Where the current delivery should be taken
    std::string destination;

    // The current state of the mobile platform
    State state;

    // The bluetooth server
    Server server;
};

#endif
