#ifndef CAMERA_CHECKER_H
#define CAMERA_CHECKER_H

#include <apriltag.h>
#include <atomic>
#include <functional>
#include <thread>

/*!
 * @brief A class that runs a polling thread to read in the newest image from
 *        the camera and check it for an apriltag. If one is found, it lets the
 *        MobileManager know.
 */
class CameraChecker {
public:
    /*!
     * @brief Constructor
     * @param imagePath The location of the image on disk that this object
     *        will check
     * @param callback A callback function to call when an apriltag is
     *        detected in the image
     */
    CameraChecker ( const std::string & imagePath,
                    std::function<void ( int )> callback );

    /*!
     * @brief Destructor
     */
    ~CameraChecker();

private:
    // @brief Read bitmap image
    // @param file The file to read
    // @param output A pointer to the array where the image data will be
    //        stored
    void read_bmp ( const std::string& file,
                    image_u8_t* output );

    // @brief PID of the raspistill process that will take the actual pictures
    unsigned int cam_pid;

    /*!
     * @brief Thread to run the image check loop
     */
    std::thread thread;

    /*!
     * @brief Flag to stop the check loop
     */
    std::atomic<bool> stopFlag;
};

#endif
