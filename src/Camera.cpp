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

/** Struct used to pass information in encoder port userdata to callback
 */
/*
typedef struct
{
   FILE *file_handle;                   /// File handle to write buffer data to.
   VCOS_SEMAPHORE_T complete_semaphore; /// semaphore which is posted when we reach end of frame (indicates end of capture or fault)
   RASPISTILL_STATE *pstate;            /// pointer to our state in case required in callback
} PORT_USERDATA;

static struct
{
   char *format;
   MMAL_FOURCC_T encoding;
} encoding_xref[] =
{
   {"jpg", MMAL_ENCODING_JPEG},
   {"bmp", MMAL_ENCODING_BMP},
   {"gif", MMAL_ENCODING_GIF},
   {"png", MMAL_ENCODING_PNG},
   {"ppm", MMAL_ENCODING_PPM},
   {"tga", MMAL_ENCODING_TGA}
};

static int encoding_xref_size = sizeof(encoding_xref) / sizeof(encoding_xref[0]);
*/

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

/**
 * Dump image state parameters to stderr. Used for debugging
 *
 * @param state Pointer to state structure to assign defaults to
 */
/*
static void dump_status(RASPISTILL_STATE *state)
{
   int i;

   if (!state)
   {
      vcos_assert(0);
      return;
   }

   raspicommonsettings_dump_parameters(&state->common_settings);

   fprintf(stderr, "Quality %d, Raw %s\n", state->quality, state->wantRAW ? "yes" : "no");
   fprintf(stderr, "Thumbnail enabled %s, width %d, height %d, quality %d\n",
           state->thumbnailConfig.enable ? "Yes":"No", state->thumbnailConfig.width,
           state->thumbnailConfig.height, state->thumbnailConfig.quality);

   fprintf(stderr, "Time delay %d, Timelapse %d\n", state->timeout, state->timelapse);
   fprintf(stderr, "Link to latest frame enabled ");
   if (state->linkname)
   {
      fprintf(stderr, " yes, -> %s\n", state->linkname);
   }
   else
   {
      fprintf(stderr, " no\n");
   }
   fprintf(stderr, "Full resolution preview %s\n", state->fullResPreview ? "Yes": "No");

   fprintf(stderr, "Capture method : ");
   for (i=0; i<next_frame_description_size; i++)
   {
      if (state->frameNextMethod == next_frame_description[i].nextFrameMethod)
         fprintf(stderr, "%s", next_frame_description[i].description);
   }
   fprintf(stderr, "\n\n");

   if (state->enableExifTags)
   {
      if (state->numExifTags)
      {
         fprintf(stderr, "User supplied EXIF tags :\n");

         for (i=0; i<state->numExifTags; i++)
         {
            fprintf(stderr, "%s", state->exifTags[i]);
            if (i != state->numExifTags-1)
               fprintf(stderr, ",");
         }
         fprintf(stderr, "\n\n");
      }
   }
   else
      fprintf(stderr, "EXIF tags disabled\n");

   raspipreview_dump_parameters(&state->preview_parameters);
   raspicamcontrol_dump_parameters(&state->camera_parameters);
}

static void encoder_buffer_callback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
   int complete = 0;

   // We pass our file handle and other stuff in via the userdata field.

   PORT_USERDATA *pData = (PORT_USERDATA *)port->userdata;

   if (pData)
   {
      int bytes_written = buffer->length;

      if (buffer->length && pData->file_handle)
      {
         mmal_buffer_header_mem_lock(buffer);

         bytes_written = fwrite(buffer->data, 1, buffer->length, pData->file_handle);

         mmal_buffer_header_mem_unlock(buffer);
      }

      // We need to check we wrote what we wanted - it's possible we have run out of storage.
      if (bytes_written != buffer->length)
      {
         vcos_log_error("Unable to write buffer to file - aborting");
         complete = 1;
      }

      // Now flag if we have completed
      if (buffer->flags & (MMAL_BUFFER_HEADER_FLAG_FRAME_END | MMAL_BUFFER_HEADER_FLAG_TRANSMISSION_FAILED))
         complete = 1;
   }
   else
   {
      vcos_log_error("Received a encoder buffer callback with no state");
   }

   // release buffer back to the pool
   mmal_buffer_header_release(buffer);

   // and send one back to the port (if still open)
   if (port->is_enabled)
   {
      MMAL_STATUS_T status = MMAL_SUCCESS;
      MMAL_BUFFER_HEADER_T *new_buffer;

      new_buffer = mmal_queue_get(pData->pstate->encoder_pool->queue);

      if (new_buffer)
      {
         status = mmal_port_send_buffer(port, new_buffer);
      }
      if (!new_buffer || status != MMAL_SUCCESS)
         vcos_log_error("Unable to return a buffer to the encoder port");
   }

   if (complete)
      vcos_semaphore_post(&(pData->complete_semaphore));
}
*/

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

    PORT_USERDATA callback_data;

    camera_still_port = m_Settings.camera_component->output[MMAL_CAMERA_CAPTURE_PORT];

 VCOS_STATUS_T vcos_status;

 // Set up our userdata - this is passed though to the callback where we need the information.
 // Null until we open our filename
 callback_data.file_handle = NULL;
 callback_data.pstate = &state;
 vcos_status = vcos_semaphore_create(&callback_data.complete_semaphore, "RaspiStill-sem", 0);

 vcos_assert(vcos_status == VCOS_SUCCESS);

 {
    int frame, keep_looping = 1;
    FILE *output_file = NULL;
    char *use_filename = NULL;      // Temporary filename while image being written
    char *final_filename = NULL;    // Name that file gets once writing complete

    frame = m_Settings.frameStart - 1;

    while (keep_looping)
    {
       keep_looping = wait_for_next_frame(&state, &frame);

       if (m_Settings.datetime)
       {
          time_t rawtime;
          struct tm *timeinfo;

          time(&rawtime);
          timeinfo = localtime(&rawtime);

          frame = timeinfo->tm_mon+1;
          frame *= 100;
          frame += timeinfo->tm_mday;
          frame *= 100;
          frame += timeinfo->tm_hour;
          frame *= 100;
          frame += timeinfo->tm_min;
          frame *= 100;
          frame += timeinfo->tm_sec;
       }
       if (m_Settings.timestamp)
       {
          frame = (int)time(NULL);
       }

       // We only capture if a filename was specified and it opened
       {
          int num, q;

          // Must do this before the encoder output port is enabled since
          // once enabled no further exif data is accepted
          {
             mmal_port_parameter_set_boolean(
                m_Settings.encoder_component->output[0], MMAL_PARAMETER_EXIF_DISABLE, 1);
          }

          // Same with raw, apparently need to set it for each capture, whilst port
          // is not enabled
          if (m_Settings.wantRAW)
          {
             if (mmal_port_parameter_set_boolean(camera_still_port, MMAL_PARAMETER_ENABLE_RAW_CAPTURE, 1) != MMAL_SUCCESS)
             {
                 std::cout << "RAW was requested, but failed to enable" << std::endl;
             }
          }

          // There is a possibility that shutter needs to be set each loop.
          if (mmal_status_to_int(mmal_port_parameter_set_uint32(m_Settings.camera_component->control, MMAL_PARAMETER_SHUTTER_SPEED, m_Settings.camera_parameters.shutter_speed)) != MMAL_SUCCESS)
             std::cout << "Unable to set shutter speed" << std::endl;

          // Enable the encoder output port
          encoder_output_port->userdata = (struct MMAL_PORT_USERDATA_T *)&callback_data;

          // Enable the encoder output port and tell it its callback function
          status = mmal_port_enable(encoder_output_port, encoder_buffer_callback);

          // Send all the buffers to the encoder output port
          num = mmal_queue_length(m_Settings.encoder_pool->queue);

          for (q=0; q<num; q++)
          {
             MMAL_BUFFER_HEADER_T *buffer = mmal_queue_get(m_Settings.encoder_pool->queue);

             if (!buffer)
                std::cout << "Unable to get a required buffer from pool queue" << std::endl;

             if (mmal_port_send_buffer(encoder_output_port, buffer)!= MMAL_SUCCESS)
                std::cout << "Unable to send a buffer to encoder output port" << std::endl;
          }

          if(m_Settings.camera_parameters.enable_annotate)
          {
             if ((m_Settings.camera_parameters.enable_annotate & ANNOTATE_APP_TEXT) && m_Settings.common_settings.gps)
             {
                char *text = raspi_gps_location_string();
                raspicamcontrol_set_annotate(m_Settings.camera_component, m_Settings.camera_parameters.enable_annotate,
                                             text,
                                             m_Settings.camera_parameters.annotate_text_size,
                                             m_Settings.camera_parameters.annotate_text_colour,
                                             m_Settings.camera_parameters.annotate_bg_colour,
                                             m_Settings.camera_parameters.annotate_justify,
                                             m_Settings.camera_parameters.annotate_x,
                                             m_Settings.camera_parameters.annotate_y
                                            );
                free(text);
             }
             else
                raspicamcontrol_set_annotate(m_Settings.camera_component, m_Settings.camera_parameters.enable_annotate,
                                             m_Settings.camera_parameters.annotate_string,
                                             m_Settings.camera_parameters.annotate_text_size,
                                             m_Settings.camera_parameters.annotate_text_colour,
                                             m_Settings.camera_parameters.annotate_bg_colour,
                                             m_Settings.camera_parameters.annotate_justify,
                                             m_Settings.camera_parameters.annotate_x,
                                             m_Settings.camera_parameters.annotate_y
                                            );
          }

          if (m_Settings.common_settings.verbose)
             fprintf(stderr, "Starting capture %d\n", frame);

          if (mmal_port_parameter_set_boolean(camera_still_port, MMAL_PARAMETER_CAPTURE, 1) != MMAL_SUCCESS)
          {
              std::cout << "Failed to start capture" << std::endl;
          }
          else
          {
             // Wait for capture to complete
             // For some reason using vcos_semaphore_wait_timeout sometimes returns immediately with bad parameter error
             // even though it appears to be all correct, so reverting to untimed one until figure out why its erratic
             vcos_semaphore_wait(&callback_data.complete_semaphore);
             if (m_Settings.common_settings.verbose)
                fprintf(stderr, "Finished capture %d\n", frame);
          }

          // Ensure we don't die if get callback with no open file
          callback_data.file_handle = NULL;

          if (output_file != stdout)
          {
             rename_file(&state, output_file, final_filename, use_filename, frame);
          }
          else
          {
             fflush(output_file);
          }
          // Disable encoder output port
          status = mmal_port_disable(encoder_output_port);
       }

       if (use_filename)
       {
          free(use_filename);
          use_filename = NULL;
       }
       if (final_filename)
       {
          free(final_filename);
          final_filename = NULL;
       }
    } // end for (frame)

    vcos_semaphore_delete(&callback_data.complete_semaphore);
 }

  if (m_Settings.camera_component)
     mmal_component_disable(m_Settings.camera_component);

  //destroy_camera_component(&state);
}

