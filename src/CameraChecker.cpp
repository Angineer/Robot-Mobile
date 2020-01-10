#include "CameraChecker.h"
#include <cstring>
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
        // Allocate space to save the image data
        // TODO: make this more robust
        auto img = new unsigned char[800*600*3];

        // Run until we get a signal to stop
        while ( !this->stopFlag.load() ) {
            // Send signal to camera process so it grabs a new image
            kill ( cam_pid, SIGUSR1 );

            // Give the camera some time to write the image to disk
            std::this_thread::sleep_for ( std::chrono::milliseconds ( 100 ) );

            // Read image
            std::cout << "Reading image" << std::endl;
            read_bmp ( imagePath, img );

            // Check for apriltag
            int tag_id { -1 };
            // TODO

            // If we found one, let the MobileManager know
            if ( tag_id != -1 ){
                callback ( tag_id );
            }
        }
        delete[] img;
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

void CameraChecker::read_bmp ( const std::string & file_path,
                                         unsigned char* output )
{
    // I make some assumptions about the format here, specifically
    // that we're working with the BITMAPINFOHEADER format

    std::ifstream reader { file_path, std::ios_base::in | std::ios_base::binary };

    // Read in the bmp and dib headers
    char header[54];
    reader.read ( header, 54 );

    // Read the size from the header
    long width, height;
    memcpy ( &width, &header[18], sizeof ( width ) );
    memcpy ( &height, &header[22], sizeof ( height ) );

    std::cout << "DEBUG: " << width << "x" << height << std::endl;

    // Read in the data
    int size = 3 * width * height;
    reader.read ( reinterpret_cast<char*> ( output ), size );
}
