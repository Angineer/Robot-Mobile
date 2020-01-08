#include "CameraChecker.h"
#include <fstream>
#include <iostream>
#include <signal.h>

CameraChecker::CameraChecker ( const std::string & imagePath,
                               std::function<void ( int )> callback ) :
    stopFlag ( false )
{
    // Find the camera process id
    std::ifstream pid_reader;
    pid_reader.open ( "/home/pi/cam_pid" );
    pid_reader >> cam_pid;
    pid_reader.close();

    // Start checking
    auto checkFunc = [ imagePath, callback, this ] {
        // Run until we get a signal to stop
        while ( !this->stopFlag.load() ) {
            // Send signal to camera process so it grabs a new image
            kill ( cam_pid, SIGUSR1 );

            // Give the camera some time to write the image to disk
            std::this_thread::sleep_for ( std::chrono::milliseconds ( 100 ) );

            // Read image
            std::cout << "Reading image" << std::endl;
            read_jpeg ( imagePath );

            // Check for apriltag
            int tag_id { -1 };
            // TODO

            // If we found one, let the MobileManager know
            if ( tag_id != -1 ){
                callback ( tag_id );
            }
        }
    };
    thread = std::thread ( checkFunc );
}

CameraChecker::~CameraChecker()
{
    // Signal and wait for the thread to finish up
    stopFlag.store ( false );
    if ( thread.joinable() ){
        thread.join();
    }
}

void CameraChecker::read_jpeg ( const std::string & file_path )
{
    //TODO
}
