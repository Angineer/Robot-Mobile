#include "CameraChecker.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <signal.h>

#include <tag16h5.h>

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
            auto img = read_bmp ( imagePath );
            //image_u8_write_pnm ( img, "debug.pnm" );

            // Check for apriltags
            int tag_id { -1 };
            apriltag_detector_t *td = apriltag_detector_create();
            apriltag_family_t *tf = tag16h5_create();
            apriltag_detector_add_family(td, tf);
            //td->qtp.min_white_black_diff = 10;
            zarray_t *detections = apriltag_detector_detect ( td, img );

            for (int i = 0; i < zarray_size(detections); i++) {
                apriltag_detection_t *det;
                zarray_get(detections, i, &det);

                // Do stuff with detections here.
                if ( det->decision_margin > 70 ) {
                    tag_id = det->id;
                }
                apriltag_detection_destroy ( det );
            }

            // Clean up
            image_u8_destroy ( img );
            zarray_destroy ( detections );
            tag16h5_destroy ( tf );
            apriltag_detector_destroy ( td );

            // If we found one, let the MobileManager know
            if ( tag_id != -1 ){
                std::cout << "ID detected: " << tag_id << std::endl;
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

image_u8_t* CameraChecker::read_bmp ( const std::string & file_path )
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

    // Prep output
    image_u8_t* img = image_u8_create ( width, height );

    // Read in the data one pixel at a time and convert to grayscale as we do
    char pixel_data[3];
    for ( unsigned long y = 0; y < height; ++y ) {
        for ( unsigned long x = 0; x < width; ++x ) {
            reader.read ( pixel_data, 3 );
            double r = pixel_data[0];
            double g = pixel_data[1];
            double b = pixel_data[2];

            // This is from the Wikipedia grayscale article
            unsigned char gray = 0.2126 * r + 0.7152 * g + 0.0722 * b;

            img->buf[( height - y - 1)*img->stride + x] = gray;
        }
    }
    return img;
}
