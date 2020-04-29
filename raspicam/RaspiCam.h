#include "RaspiStill.h"

/*!
 * @brief Create a new camera object and return a pointer to it
 */
RASPISTILL_STATE* createCam ( int argc, const char** argv );

/*!
 * @brief Clean up a camera object
 */
void destroyCam ( RASPISTILL_STATE* state );
