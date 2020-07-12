#include "CameraFetcher.h"

#include <apriltag.h>
#include <iostream>
#include <signal.h>

#include "ImageChecker.h"
#include "MobileConfiguration.h"

#define DEBUG 1

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
    MobileConfiguration config;
    int width = config.getConfig<int> ( "img_width" );
    int height = config.getConfig<int> ( "img_height" );
    // Apriltag images use a specific stride
    int stride = ( width / 96 + ( width % 96 > 0 ) ) * 96;

    // Set up buffer for storing image data
    image_u8_t* buffer = image_u8_create ( width, height );

    // Create object to check the newest image for april tags
    ImageChecker checker ( buffer, callback );

    auto camera = createCam ( width, height );

    while ( !this->m_StopFlag.load() ) {

        // Read image into the buffer
        capture ( camera, buffer->buf, width, height, stride );

        if ( DEBUG ) {
            image_u8_write_pnm ( buffer, "debug.pnm" );
        }

        // Notify the april tag checker
        checker.notify();
    }
    checker.notify();

    image_u8_destroy ( buffer );
    destroyCam ( camera );
}
