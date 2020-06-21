#ifndef CAMERA_FETCHER_H
#define CAMERA_FETCHER_H

#include <atomic>
#include <functional>
#include <thread>

#include "ImageBuffer.h"

// A class that prompts the camera for a new image and reads it into memory.
// Once the image is ready to be processed, this class will notify the
// ImageChecker.
class CameraFetcher {
public:
    // Constructor
    // @param callback A callback function to call when a new image has been
    //        read in
    CameraFetcher ( std::function<void ( int )> callback );

    // Destructor
    ~CameraFetcher();

private:
    // Ask the camera for an image, read it in, and notify the image checker
    // @param callback A callback function to call when a new image has been
    //        read in
    void processImages ( std::function<void ( int )> callback );

    // Read bitmap image. It will be converted to grayscale.
    // @param buffer A pointer to the array where the image data should be
    //        stored
    // @param file The file to read
    void read_bmp ( std::shared_ptr<ImageBuffer> buffer,
                    const std::string& file );

    // Thread to run the image check loop
    std::thread m_Thread;

    // Flag to stop the check loop
    std::atomic<bool> m_StopFlag;
};

#endif
