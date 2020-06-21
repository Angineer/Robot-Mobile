struct RASPISTILL_STATE;

/*!
 * @brief Create a new camera object and return a pointer to it. Accepts the
 *        same command line arguments as the raspistill app, if you feel so
 *        inclined.
 */
struct RASPISTILL_STATE* createCam();

/*!
 * @brief Clean up a camera object
 */
void destroyCam ( struct RASPISTILL_STATE* state );

/*!
 * @brief Capture a frame from the camera and save it to memeory
 */
void capture ( struct RASPISTILL_STATE* state, void* im_buffer );
