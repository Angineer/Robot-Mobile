#ifndef IMAGE_CHECKER_H
#define IMAGE_CHECKER_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <thread>

#include "ImageBuffer.h"

// A class that runs a polling thread to read in the newest image from
// the camera and check it for an apriltag. If one is found, it lets the
// MobileManager know.
class ImageChecker {
public:
     // Constructor
     // @param callback A callback function to call when an apriltag is
     //        detected in the image
    ImageChecker ( std::shared_ptr<ImageBuffer> buffer,
                   std::function<void ( int )> callback );

    // Destructor
    ~ImageChecker();

    // Let this object know that there is new image data in the buffer to check
    void notify();

private:
    // Check the buffer to see if there are any tags visible. If there are,
    // call the callback
    void checkForTags ( std::function<void ( int )> callback );

    // AprilTag objects
    apriltag_detector_t * m_Detector;
    apriltag_family_t * m_Family;

    // Thread to run the image check loop
    std::thread m_Thread;

    // Flag to stop the check loop
    std::atomic<bool> m_StopFlag;

    // The buffer where the image data are stored
    std::shared_ptr<ImageBuffer> m_Buffer;

    // Objects for notifying that that there are new image data ready
    std::mutex m_Mutex;
    std::condition_variable m_ReadyCV;
    bool m_ReadyFlag;
};

#endif
