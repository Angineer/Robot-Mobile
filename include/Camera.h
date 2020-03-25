#ifndef CAMERA_H
#define CAMERA_H

#include "interface/mmal/mmal.h"
#include "interface/mmal/util/mmal_connection.h"

#include "RaspiCamControl.h"
#include "RaspiCommonSettings.h"

// Structure to hold camera settings
struct CameraSettings
{
   RASPICOMMONSETTINGS_PARAMETERS common_settings;     /// Common settings
   int timeout;                        /// Time taken before frame is grabbed and app then shuts down. Units are milliseconds
   int quality;                        /// JPEG quality setting (1-100)
   int wantRAW;                        /// Flag for whether the JPEG metadata also contains the RAW bayer image
   char *linkname;                     /// filename of output file
   int frameStart;                     /// First number of frame output counter
   MMAL_PARAM_THUMBNAIL_CONFIG_T thumbnailConfig;
   int demoMode;                       /// Run app in demo mode
   int demoInterval;                   /// Interval between camera settings changes
   MMAL_FOURCC_T encoding;             /// Encoding to use for the output file.
   int numExifTags;                    /// Number of supplied tags
   int enableExifTags;                 /// Enable/Disable EXIF tags in output
   int timelapse;                      /// Delay between each picture in timelapse mode. If 0, disable timelapse
   int fullResPreview;                 /// If set, the camera preview port runs at capture resolution. Reduces fps.
   int frameNextMethod;                /// Which method to use to advance to next frame
   int useGL;                          /// Render preview using OpenGL
   int glCapture;                      /// Save the GL frame-buffer instead of camera output
   int burstCaptureMode;               /// Enable burst mode
   int datetime;                       /// Use DateTime instead of frame#
   int timestamp;                      /// Use timestamp instead of frame#
   int restart_interval;               /// JPEG restart interval. 0 for none.

   RASPICAM_CAMERA_PARAMETERS camera_parameters; /// Camera setup parameters

   MMAL_COMPONENT_T *camera_component;    /// Pointer to the camera component
   MMAL_CONNECTION_T *encoder_connection; /// Pointer to the connection from camera to encoder

   MMAL_POOL_T *encoder_pool; /// Pointer to the pool of buffers used by encoder output port
};

// A class that provides access to the Raspi camera via mmal. Inspired by/based
// on RaspiStill.c.
class Camera {
public:
    // Constructor
    Camera();

    // Destructor
    ~Camera() = default;

private:
    // Main function for running the camera. Captures images from the camera
    // as fast as it can
    void run();

    // Reset all settings to their default values
    void reset();

    // Create the camera component, set up its ports
    // @return MMAL_SUCCESS if all OK, something else otherwise
    MMAL_STATUS_T createMmalComponent();

    // Destroy the camera component
    void destroyMmalComponent();

    // Camera settings
    CameraSettings m_Settings;

    // Camera port
    MMAL_PORT_T *m_CameraStillPort = NULL;
};

#endif

