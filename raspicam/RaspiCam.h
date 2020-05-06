#include "RaspiStill.h"

/*!
 * @brief Create a new camera object and return a pointer to it. Accepts the
 *        same command line arguments as the raspistill app, if you feel so
 *        inclined.
 */
RASPISTILL_STATE* createCam ( int argc, const char** argv );

/*!
 * @brief Clean up a camera object
 */
void destroyCam ( RASPISTILL_STATE* state );

/*!
 * @brief Capture a frame from the camera and save it to memeory
 */
void capture ( RASPISTILL_STATE* state, void* im_buffer );
