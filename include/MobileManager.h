#ifndef MOBILE_MANAGER_H
#define MOBILE_MANAGER_H

#include <Server.h>
#include <State.h>

#include <mutex>

class MobileManager {
public:
    // Constructor
    MobileManager();

    // Run the Manager
    void run();
private:
    // Handle input from connections on the bluetooth server
    std::string handle_input ( const std::string& input );

    // Mutex for managing access to current state
    std::mutex access_mutex;

    // The current state of the mobile platform
    State state;

    // The bluetooth server
    Server server;
};

#endif
