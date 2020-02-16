#include "ImageChecker.h"

#include <tag16h5.h>
#include <iostream>

ImageChecker::ImageChecker ( std::shared_ptr<ImageBuffer> buffer,
                             std::function<void ( int )> callback ) :
    m_Buffer ( buffer ),
    m_StopFlag ( false ),
    m_ReadyFlag ( false )
{
    m_Detector = apriltag_detector_create();
    m_Family = tag16h5_create();
    apriltag_detector_add_family ( m_Detector, m_Family );

    // Start checking
    auto checkFunc = [ callback, this ] {
        this->checkForTags ( callback );
    };
    m_Thread = std::thread ( checkFunc );
}

ImageChecker::~ImageChecker()
{
    // Signal and wait for the thread to finish up
    m_StopFlag.store ( false );
    if ( m_Thread.joinable() ){
        m_Thread.join();
    }

    // Clean up
    tag16h5_destroy ( m_Family );
    apriltag_detector_destroy ( m_Detector );
}

void ImageChecker::checkForTags ( std::function<void ( int )> callback )
{
    // Run until we get a signal to stop
    while ( !this->m_StopFlag.load() ) {
        // Wait on condition variable
        std::unique_lock<std::mutex> lock ( m_Mutex );
        m_ReadyCV.wait ( lock, [ this ]{ return m_ReadyFlag;} );
        m_ReadyFlag = false;
        lock.unlock();

        // Read image
        auto img = m_Buffer->getImage();

        // Check for apriltags
        int tag_id { -1 };
        //td->qtp.min_white_black_diff = 10;
        zarray_t *detections =
            apriltag_detector_detect ( m_Detector, img );

        for (int i = 0; i < zarray_size(detections); i++) {
            apriltag_detection_t *det;
            zarray_get(detections, i, &det);

            // Do stuff with detections here.
            if ( det->decision_margin > 70 ) {
                tag_id = det->id;
            }
            apriltag_detection_destroy ( det );
        }

        zarray_destroy ( detections );

        // If we found one, let the MobileManager know
        if ( tag_id != -1 ){
            std::cout << "ID detected: " << tag_id << std::endl;
            callback ( tag_id );
        }
    }
}

void ImageChecker::notify()
{
    {
        std::lock_guard<std::mutex> lock ( m_Mutex );
        m_ReadyFlag = true;
    }
    m_ReadyCV.notify_all();
}
