#ifndef CAMERA_CHECKER_H
#define CAMERA_CHECKER_H

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
     */
    CameraChecker();

    /*!
     * @brief Destructor
     */
    ~CameraChecker();

    /*!
     * @brief Kick off the image-checking loop
     * @param imagePath The location of the image on disk that this object
     *        will check
     * @param callback A callback function to call when an apriltag is
     *        detected in the image
     */
    void start_checking ( const std::string & imagePath,
                          std::function<void ( int )> callback  );

private:
    /*!
     * @brief The image that we'll check
     */
    std::string image;

    /*!
     * @brief The callback that we'll call when we find an apriltag
     */
    std::function<void ( int )> callback;

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
