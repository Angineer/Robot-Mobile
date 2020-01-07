#include "CameraChecker.h"

CameraChecker::CameraChecker ( const std::string & imagePath,
                               std::function<void ( int )> callback ) :
    stopFlag ( false )
{
    // Find the camera process id
    // TODO

    // Start checking
    auto checkFunc = [ imagePath, callback, this ] {
        // Send signal to camera process so it grabs a new image
        // TODO

        // Give the camera some time to write the image to disk
        std::this_thread::sleep_for ( std::chrono::milliseconds ( 100 ) );

        // Run until we get a signal to stop
        while ( !this->stopFlag.load() ) {
            // Read image
            //TODO

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
