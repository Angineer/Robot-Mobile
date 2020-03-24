#include "CameraFetcher2.h"

#include <iostream>

// Raspi includes (for camera access)
#include "bcm_host.h"
#include "interface/mmal/mmal.h"
#include "interface/mmal/util/mmal_default_components.h"
#include "interface/mmal/util/mmal_util_params.h"

// Robie includes
#include "ImageChecker.h"

CameraFetcher::CameraFetcher ( const std::string &imagePath,
                               std::function<void ( int )> callback ) :
    m_StopFlag ( false )
{
    // Raspberry Pi requires that the bcm_host_init() function is called first
    // before any GPU calls can be made
    bcm_host_init();

    // Set up MMAL stuff
    MMAL_PORT_T *camera_still_port = NULL;
    bool success = startCamera();

    // Set up thread and start it
    auto checkFunc = [ this, imagePath, callback ]() {
        this->processImages ( imagePath, callback );
    };

    m_Thread = std::thread ( checkFunc );
}

CameraFetcher::~CameraFetcher()
{
    // Signal and wait for the thread to finish up
    m_StopFlag.store ( false );
    if ( m_Thread.joinable() ){
        m_Thread.join();
    }

    // TODO: MMAL cleanup
}

void CameraFetcher::processImages ( const std::string& imagePath,
                                    std::function<void ( int )> callback )
{
    // Set up buffer for storing image data
    auto buffer = std::make_shared<ImageBuffer> ( imagePath );

    // Create object to check the newest image for april tags
    ImageChecker checker ( buffer, callback );

    while ( !this->m_StopFlag.load() ) {
        // Grab a new image from the camera

        // Read image into the buffer
        buffer->readImage ( imagePath );
        //image_u8_write_pnm ( writeBuff, "debug.pnm" );

        // Notify the april tag checker
        checker.notify();
    }
}

bool CameraFetcher::startCamera ()
{

   MMAL_COMPONENT_T *camera = 0;
   MMAL_ES_FORMAT_T *format;
   MMAL_PORT_T *still_port = NULL;
   MMAL_STATUS_T status;

   /* Create the component */
   status = mmal_component_create(MMAL_COMPONENT_DEFAULT_CAMERA, &camera);

   if (status != MMAL_SUCCESS)
   {
      std::cout << "Failed to create camera component";
      goto error;
   }

   //status = raspicamcontrol_set_stereo_mode(camera->output[0], &state->camera_parameters.stereo_mode);
   //status += raspicamcontrol_set_stereo_mode(camera->output[1], &state->camera_parameters.stereo_mode);
   //status += raspicamcontrol_set_stereo_mode(camera->output[2], &state->camera_parameters.stereo_mode);

   //if (status != MMAL_SUCCESS)
   //{
      //std::cout << "Could not set stereo mode : error %d", status;
      //goto error;
   //}

   MMAL_PARAMETER_INT32_T camera_num =
   	{{MMAL_PARAMETER_CAMERA_NUM, sizeof(camera_num)}, 0};

   status = mmal_port_parameter_set(camera->control, &camera_num.hdr);

   if (status != MMAL_SUCCESS)
   {
      std::cout << "Could not select camera : error %d", status;
      goto error;
   }

   if (!camera->output_num)
   {
      status = MMAL_ENOSYS;
      std::cout << "Camera doesn't have output ports";
      goto error;
   }

   status = mmal_port_parameter_set_uint32 ( camera->control,
		   			     MMAL_PARAMETER_CAMERA_CUSTOM_SENSOR_CONFIG,
					     0);

   if (status != MMAL_SUCCESS)
   {
      std::cout << "Could not set sensor mode : error %d", status;
      goto error;
   }

   still_port = camera->output[2];

   // Enable the camera, and set its control callback function. This callback
   // function does nothing right now.
   //auto emptyCallback =
	   //[](MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer){};
   //status = mmal_port_enable(camera->control, emptyCallback);

   //if (status != MMAL_SUCCESS)
   //{
      //std::cout << "Unable to enable control port : error %d", status;
      //goto error;
   //}

   //  set up the camera configuration
   {
      MMAL_PARAMETER_CAMERA_CONFIG_T cam_config =
      {
         { MMAL_PARAMETER_CAMERA_CONFIG, sizeof(cam_config) },
         .max_stills_w = 640,
         .max_stills_h = 480,
         .stills_yuv422 = 0,
         .one_shot_stills = 1,
         .max_preview_video_w = 640,
         .max_preview_video_h = 480,
         .num_preview_video_frames = 3,
         .stills_capture_circular_buffer_height = 0,
         .fast_preview_resume = 0,
         .use_stc_timestamp = MMAL_PARAM_TIMESTAMP_MODE_RESET_STC
      };

      mmal_port_parameter_set(camera->control, &cam_config.hdr);
   }

   //raspicamcontrol_set_all_parameters(camera, &state->camera_parameters);

   // Now set up the port format
   format = still_port->format;

   if(state->camera_parameters.shutter_speed > 6000000)
   {
      MMAL_PARAMETER_FPS_RANGE_T fps_range = {{MMAL_PARAMETER_FPS_RANGE, sizeof(fps_range)},
         { 50, 1000 }, {166, 1000}
      };
      mmal_port_parameter_set(still_port, &fps_range.hdr);
   }
   else if(state->camera_parameters.shutter_speed > 1000000)
   {
      MMAL_PARAMETER_FPS_RANGE_T fps_range = {{MMAL_PARAMETER_FPS_RANGE, sizeof(fps_range)},
         { 167, 1000 }, {999, 1000}
      };
      mmal_port_parameter_set(still_port, &fps_range.hdr);
   }
   // Set our stills format on the stills (for encoder) port
   format->encoding = MMAL_ENCODING_OPAQUE;
   format->es->video.width = VCOS_ALIGN_UP(state->common_settings.width, 32);
   format->es->video.height = VCOS_ALIGN_UP(state->common_settings.height, 16);
   format->es->video.crop.x = 0;
   format->es->video.crop.y = 0;
   format->es->video.crop.width = state->common_settings.width;
   format->es->video.crop.height = state->common_settings.height;
   format->es->video.frame_rate.num = STILLS_FRAME_RATE_NUM;
   format->es->video.frame_rate.den = STILLS_FRAME_RATE_DEN;

   status = mmal_port_format_commit(still_port);

   if (status != MMAL_SUCCESS)
   {
      std::cout << "camera still format couldn't be set";
      goto error;
   }

   /* Ensure there are enough buffers to avoid dropping frames */
   if (still_port->buffer_num < VIDEO_OUTPUT_BUFFERS_NUM)
      still_port->buffer_num = VIDEO_OUTPUT_BUFFERS_NUM;

   /* Enable component */
   status = mmal_component_enable(camera);

   if (status != MMAL_SUCCESS)
   {
      std::cout << "camera component couldn't be enabled";
      goto error;
   }

   if (state->common_settings.verbose)
      fprintf(stderr, "Camera component done\n");

   return status;

error:

   if (camera)
      mmal_component_destroy(camera);

   return status;
}
