#include "MobileManager.h"

#include <iostream>            

int main ( int argc, char *argv[] ) 
{ 
    std::cout << "Starting Mobile Manager" << std::endl;

    // Create the manager      
    MobileManager manager;

    // Run forever
    manager.run();
}
