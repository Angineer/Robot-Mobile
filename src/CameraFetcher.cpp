#include "CameraFetcher.h"

#include <fstream>
#include <signal.h>
#include <sys/inotify.h>

#include "ImageChecker.h"

CameraFetcher::CameraFetcher ( const std::string &imagePath,
                               std::function<void ( int )> callback ) :
    m_StopFlag ( false )
{
    // Find the camera process id
    std::ifstream pid_reader;
    pid_reader.open ( "/home/pi/cam_pid" );
    pid_reader >> m_CamPid;
    pid_reader.close();

    m_Notify = inotify_init();
    inotify_add_watch ( m_Notify, imagePath.c_str(), IN_CLOSE_WRITE );

    auto checkFunc = [ this, &imagePath, callback ]() {
        this->processImages ( imagePath, callback );
    };

    m_Thread = std::thread ( checkFunc );
}

CameraFetcher::~CameraFetcher()
{
    // Signal and wait for the thread to finish up
    m_StopFlag.store ( false );
    if ( m_Thread.joinable() ){
        m_Thread.join();
    }
}

void CameraFetcher::processImages ( const std::string& imagePath,
                                    std::function<void ( int )> callback )
{
    // Snap an initial pic to set up our size parameters
    kill ( m_CamPid, SIGUSR1 );
    inotify_event event;
    read ( m_Notify, &event, sizeof ( event ) );

    // Set up buffer for storing image data
    auto buffer = std::make_shared<ImageBuffer> ( imagePath );

    // Create object to check the newest image for april tags
    ImageChecker checker ( buffer, callback );

    while ( !this->m_StopFlag.load() ) {
        // Send signal to camera process so it grabs a new image
        kill ( m_CamPid, SIGUSR1 );

        // Wait for the image to be written
        read ( m_Notify, &event, sizeof ( event ) );

        // Read image into the buffer
        buffer->readImage ( imagePath );
        //image_u8_write_pnm ( writeBuff, "debug.pnm" );

        // Notify the april tag checker
        checker.notify();
    }
}
