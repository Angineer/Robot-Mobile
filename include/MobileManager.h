#ifndef MOBILE_MANAGER_H
#define MOBILE_MANAGER_H

#include <mutex>

#include <Arduino.h>
#include "CameraChecker.h"
#include "MobileConfiguration.h"
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

    // Handle update from the CameraChecker
    void handle_cam_update ( int location_id );

    // Arduino that does the low-level motor control and sensing
    Arduino arduino;

    // Mutex for managing access to current state
    std::mutex access_mutex;

    // Where the current delivery should be taken
    int destination;

    // The current state of the mobile platform
    State state;

    // The bluetooth server
    Server server;

    // Class to check for apriltags in the camera's view and let us know
    CameraChecker checker;

    // 
    MobileConfiguration config;
};

#endif
