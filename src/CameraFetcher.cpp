#include "CameraFetcher.h"

#include <apriltag.h>
#include <iostream>
#include <signal.h>

#include "ImageChecker.h"

extern "C"
{
    #include "RaspiCam.h"
}

CameraFetcher::CameraFetcher ( std::function<void ( int )> callback ) :
    m_StopFlag ( false )
{
    auto checkFunc = [ this, callback ]() {
        this->processImages ( callback );
    };

    m_Thread = std::thread ( checkFunc );
}

CameraFetcher::~CameraFetcher()
{
    // Signal and wait for the thread to finish up
    m_StopFlag.store ( true );
    if ( m_Thread.joinable() ){
        m_Thread.join();
    }
    std::cout << "CameraFetcher stopped" << std::endl;
}

void CameraFetcher::processImages ( std::function<void ( int )> callback )
{
    // Set up buffer for storing image data
    // TODO: determine the correct size
    image_u8_t* buffer = image_u8_create ( 640, 480 );

    // Create object to check the newest image for april tags
    ImageChecker checker ( buffer, callback );

    auto camera = createCam();

    while ( !this->m_StopFlag.load() ) {

        // Read image into the buffer
        capture ( camera, buffer );

        //image_u8_write_pnm ( writeBuff, "debug.pnm" );

        // Notify the april tag checker
        checker.notify();
    }
    checker.notify();

    image_u8_destroy ( buffer );
}
