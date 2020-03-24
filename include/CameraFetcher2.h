#ifndef CAMERA_FETCHER_H
#define CAMERA_FETCHER_H

#include <atomic>
#include <functional>
#include <thread>

#include "ImageBuffer.h"

// A class that gets a new image from the camera. Once the image is ready to be
// processed, this class will notify the ImageChecker.
class CameraFetcher {
public:
    // Constructor
    // @param imagePath The location of the image on disk that this object
    //        will look for
    // @param callback A callback function to call when a new image has been
    //        read in
    CameraFetcher ( const std::string & imagePath,
                    std::function<void ( int )> callback );

    // Destructor
    ~CameraFetcher();

    // Tell the fetcher to stop fetching images
    void stop();

private:
    // Ask the camera for an image, wait for it to be written to disk,
    // read it in, and notify the image checker
    // @param imagePath The location of the image on disk that this object
    //        will look for
    // @param callback A callback function to call when a new image has been
    //        read in
    void processImages ( const std::string& imagePath,
                         std::function<void ( int )> callback );

    // Read bitmap image. It will be converted to grayscale.
    // @param buffer A pointer to the array where the image data should be
    //        stored
    // @param file The file to read
    void read_bmp ( std::shared_ptr<ImageBuffer> buffer,
                    const std::string& file );

    // Get the camera ready for access via mmal; based on raspistill
    bool startCamera();

    // PID of the raspistill process that will take the actual pictures
    unsigned int m_CamPid;

    // Thread to run the image check loop
    std::thread m_Thread;

    // Flag to stop the check loop
    std::atomic<bool> m_StopFlag;

    // inotify file descriptor
    int m_Notify;
};

#endif
