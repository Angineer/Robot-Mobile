#include "CameraChecker.h"

CameraChecker::CameraChecker() :
    stopFlag ( false )
{}

void CameraChecker::start_checking ( const std::string & imagePath,
                                     std::function<void ( int )> callback  )
{
    // Start checking
    auto checkFunc = [ imagePath, callback, this ] {
        // Run until we get a signal to stop
        while ( !stopFlag.load() ) {
            // Read image

            // Check for apriltag
            int tag_id { -1 };

            // If we found one, let the MobileManager know
            if ( tag_id != -1 ){
                callback ( tag_id );
            }

            // Sleep for a little bit
            std::this_thread::sleep_for ( std::chrono::milliseconds ( 100 ) );
        }
    };
    thread = std::thread ( checkFunc );
}

CameraChecker::~CameraChecker()
{
    stopFlag.store ( false );
    if ( thread.joinable() ){
        thread.join();
    }
}
