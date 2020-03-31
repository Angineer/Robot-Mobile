/*
Copyright (c) 2018, Raspberry Pi (Trading) Ltd.
Copyright (c) 2013, Broadcom Europe Ltd.
Copyright (c) 2013, James Hughes
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
 * This file is derived from RaspiStill.c (hence the licensing info), which was
 * downloaded from https://github.com/raspberrypi
 */

#include "Camera.h"

//#include <stdio.h>
//#include <stdlib.h>
//#include <ctype.h>
//#include <string.h>
#include <iostream>
//#include <memory.h>
//#include <unistd.h>
//#include <errno.h>
//#include <sysexits.h>

#include "bcm_host.h"
//#include "interface/vcos/vcos.h"

//#include "interface/mmal/mmal_logging.h"
//#include "interface/mmal/mmal_buffer.h"
//#include "interface/mmal/util/mmal_util.h"
#include "interface/mmal/util/mmal_util_params.h"
#include "interface/mmal/util/mmal_default_components.h"
//#include "interface/mmal/mmal_parameters_camera.h"

//#include "RaspiPreview.h"
//#include "RaspiCLI.h"
//#include "RaspiTex.h"
#include "RaspiHelpers.h"

//#include "RaspiGPS.h"

//#include <semaphore.h>
//#include <math.h>
//#include <pthread.h>
//#include <time.h>

// Standard port setting for the camera component
#define MMAL_CAMERA_CAPTURE_PORT 2

// Stills format information
// 0 implies variable
#define STILLS_FRAME_RATE_NUM 0
#define STILLS_FRAME_RATE_DEN 1

/// Video render needs at least 2 buffers.
#define VIDEO_OUTPUT_BUFFERS_NUM 3

/// Amount of time before first image taken to allow settling of
/// exposure etc. in milliseconds.
#define CAMERA_SETTLE_TIME       1000

Camera::Camera()
{
}

void Camera::reset()
{
   raspicommonsettings_set_defaults(&m_Settings.common_settings);

   m_Settings.timeout = -1; // replaced with 5000ms later if unset
   m_Settings.quality = 85;
   m_Settings.wantRAW = 0;
   m_Settings.linkname = NULL;
   m_Settings.frameStart = 0;
   m_Settings.thumbnailConfig.enable = 1;
   m_Settings.thumbnailConfig.width = 64;
   m_Settings.thumbnailConfig.height = 48;
   m_Settings.thumbnailConfig.quality = 35;
   m_Settings.demoMode = 0;
   m_Settings.demoInterval = 250; // ms
   m_Settings.camera_component = NULL;

   // Setup preview window defaults
   //raspipreview_set_defaults(&m_Settings.preview_parameters);

   // Set up the camera_parameters to default
   raspicamcontrol_set_defaults(&m_Settings.camera_parameters);

   // Set initial GL preview state
   //raspitex_set_defaults(&m_Settings.raspitex_state);
}

MMAL_STATUS_T Camera::createMmalComponent()
{
   MMAL_COMPONENT_T *camera = 0;
   MMAL_ES_FORMAT_T *format;
   MMAL_PORT_T *preview_port = NULL, *video_port = NULL, *still_port = NULL;
   MMAL_STATUS_T status;

   /* Create the component */
   status = mmal_component_create(MMAL_COMPONENT_DEFAULT_CAMERA, &camera);

   if (status != MMAL_SUCCESS)
   {
       std::cout << "Failed to create camera component" << std::endl;
       throw std::exception();
   }

   status = static_cast<MMAL_STATUS_T> (
        raspicamcontrol_set_stereo_mode(camera->output[0], &m_Settings.camera_parameters.stereo_mode) +
        raspicamcontrol_set_stereo_mode(camera->output[1], &m_Settings.camera_parameters.stereo_mode) +
        raspicamcontrol_set_stereo_mode(camera->output[2], &m_Settings.camera_parameters.stereo_mode) );

   if (status != MMAL_SUCCESS)
   {
       std::cout << "Could not set stereo mode : error " << status << std::endl;
       throw std::exception();
   }

   MMAL_PARAMETER_INT32_T camera_num =
   {{MMAL_PARAMETER_CAMERA_NUM, sizeof(camera_num)}, m_Settings.common_settings.cameraNum};

   status = mmal_port_parameter_set(camera->control, &camera_num.hdr);

   if (status != MMAL_SUCCESS)
   {
       std::cout << "Could not select camera : error " << status << std::endl;
       throw std::exception();
   }

   if (!camera->output_num)
   {
      status = MMAL_ENOSYS;
      std::cout << "Camera doesn't have output ports" << std::endl;
      throw std::exception();
   }

   status = mmal_port_parameter_set_uint32(camera->control, MMAL_PARAMETER_CAMERA_CUSTOM_SENSOR_CONFIG, m_Settings.common_settings.sensor_mode);

   if (status != MMAL_SUCCESS)
   {
       std::cout << "Could not set sensor mode : error " << status << std::endl;
       throw std::exception();
   }

   still_port = camera->output[MMAL_CAMERA_CAPTURE_PORT];

   // Enable the camera, and tell it its control callback function
   status = mmal_port_enable(camera->control, default_camera_control_callback);

   if (status != MMAL_SUCCESS)
   {
       std::cout << "Unable to enable control port : error " << status << std::endl;
       throw std::exception();
   }

   //  set up the camera configuration
   {
      MMAL_PARAMETER_CAMERA_CONFIG_T cam_config =
      {
         { MMAL_PARAMETER_CAMERA_CONFIG, sizeof(cam_config) },
         .max_stills_w = m_Settings.common_settings.width,
         .max_stills_h = m_Settings.common_settings.height,
         .stills_yuv422 = 0,
         .one_shot_stills = 1,
         .max_preview_video_w = m_Settings.common_settings.width,
         .max_preview_video_h = m_Settings.common_settings.height,
         .num_preview_video_frames = 3,
         .stills_capture_circular_buffer_height = 0,
         .fast_preview_resume = 0,
         .use_stc_timestamp = MMAL_PARAM_TIMESTAMP_MODE_RESET_STC
      };

      mmal_port_parameter_set(camera->control, &cam_config.hdr);
   }

   raspicamcontrol_set_all_parameters(camera, &m_Settings.camera_parameters);

   // Now set up the port formats
   format = still_port->format;

   if(m_Settings.camera_parameters.shutter_speed > 6000000)
   {
      MMAL_PARAMETER_FPS_RANGE_T fps_range = {{MMAL_PARAMETER_FPS_RANGE, sizeof(fps_range)},
         { 50, 1000 }, {166, 1000}
      };
      mmal_port_parameter_set(still_port, &fps_range.hdr);
   }
   else if(m_Settings.camera_parameters.shutter_speed > 1000000)
   {
      MMAL_PARAMETER_FPS_RANGE_T fps_range = {{MMAL_PARAMETER_FPS_RANGE, sizeof(fps_range)},
         { 167, 1000 }, {999, 1000}
      };
      mmal_port_parameter_set(still_port, &fps_range.hdr);
   }
   // Set our stills format on the stills (for encoder) port
   format->encoding = MMAL_ENCODING_OPAQUE;
   format->es->video.width = VCOS_ALIGN_UP(m_Settings.common_settings.width, 32);
   format->es->video.height = VCOS_ALIGN_UP(m_Settings.common_settings.height, 16);
   format->es->video.crop.x = 0;
   format->es->video.crop.y = 0;
   format->es->video.crop.width = m_Settings.common_settings.width;
   format->es->video.crop.height = m_Settings.common_settings.height;
   format->es->video.frame_rate.num = STILLS_FRAME_RATE_NUM;
   format->es->video.frame_rate.den = STILLS_FRAME_RATE_DEN;

   status = mmal_port_format_commit(still_port);

   if (status != MMAL_SUCCESS)
   {
       std::cout << "camera still format couldn't be set" << std::endl;
      throw std::exception();
   }

   /* Ensure there are enough buffers to avoid dropping frames */
   if (still_port->buffer_num < VIDEO_OUTPUT_BUFFERS_NUM)
      still_port->buffer_num = VIDEO_OUTPUT_BUFFERS_NUM;

   /* Enable component */
   status = mmal_component_enable(camera);

   if (status != MMAL_SUCCESS)
   {
       std::cout << "camera component couldn't be enabled" << std::endl;
        throw std::exception();
   }

   m_Settings.camera_component = camera;

   if (m_Settings.common_settings.verbose)
      fprintf(stderr, "Camera component done\n");

   return status;

error:

   if (camera)
      mmal_component_destroy(camera);

   return status;
}

void Camera::destroyMmalComponent()
{
   if (m_Settings.camera_component)
   {
      mmal_component_destroy(m_Settings.camera_component);
      m_Settings.camera_component = NULL;
   }
}

// Function to wait for the next frame
void Camera::waitForNextFrame ( int *frame )
{
  // Not waiting, just go to next frame.
  // Actually, we do need a slight delay here otherwise exposure goes
  // badly wrong since we never allow it frames to work it out
  // This could probably be tuned down.
  // First frame has a much longer delay to ensure we get exposure to a steady state
  if (*frame == 0)
     vcos_sleep(CAMERA_SETTLE_TIME);
  else
     vcos_sleep(30);

  *frame+=1;
}

void Camera::run()
{
    MMAL_STATUS_T status = MMAL_SUCCESS;

    bcm_host_init();

    reset();

    // Setup for sensor specific parameters
    get_sensor_defaults(m_Settings.common_settings.cameraNum, m_Settings.common_settings.camera_name,
                       &m_Settings.common_settings.width, &m_Settings.common_settings.height);

    // OK, we have a nice set of parameters. Now set up our components
    // We have three components. Camera, Preview and encoder.
    // Camera and encoder are different in stills/video, but preview
    // is the same so handed off to a separate module

    if ((status = createMmalComponent()) != MMAL_SUCCESS) {
       std::cout << "Failed to create camera component" << std::endl;
       return;
    }

    m_CameraStillPort = m_Settings.camera_component->output[MMAL_CAMERA_CAPTURE_PORT];

     {
        int frame, keep_looping = 1;
        FILE *output_file = NULL;
        char *use_filename = NULL;      // Temporary filename while image being written
        char *final_filename = NULL;    // Name that file gets once writing complete

        frame = m_Settings.frameStart - 1;

        while (keep_looping)
        {
           waitForNextFrame(&frame);

           // We only capture if a filename was specified and it opened
           {
              int num, q;

              // There is a possibility that shutter needs to be set each loop.
              if ( mmal_status_to_int ( mmal_port_parameter_set_uint32 ( m_Settings.camera_component->control, 
                                                                         MMAL_PARAMETER_SHUTTER_SPEED,
                                                                         m_Settings.camera_parameters.shutter_speed ) ) != MMAL_SUCCESS)
                 std::cout << "Unable to set shutter speed" << std::endl;

              // Enable the encoder output port
              // encoder_output_port->userdata = (struct MMAL_PORT_USERDATA_T *)&callback_data;

              // Enable the encoder output port and tell it its callback function
              //status = mmal_port_enable(encoder_output_port, encoder_buffer_callback);

              // Send all the buffers to the encoder output port
              //num = mmal_queue_length(m_Settings.encoder_pool->queue);

              std::cout << "Starting capture" << std::endl;

              if (mmal_port_parameter_set_boolean(m_CameraStillPort, MMAL_PARAMETER_CAPTURE, 1) != MMAL_SUCCESS) {
                  std::cout << "Failed to start capture" << std::endl;
              } else {
                 // Wait for capture to complete
                 // For some reason using vcos_semaphore_wait_timeout sometimes returns immediately with bad parameter error
                 // even though it appears to be all correct, so reverting to untimed one until figure out why its erratic
                 //vcos_semaphore_wait(&callback_data.complete_semaphore);
                 std::cout << "Finished capture" << std::endl;
              }

              // Disable encoder output port
              //status = mmal_port_disable(encoder_output_port);
           }
        } // end for (frame)
     }

  if (m_Settings.camera_component)
     mmal_component_disable(m_Settings.camera_component);

  //destroy_camera_component(&state);
}

