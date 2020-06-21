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

/**
 * \file RaspiStill.c
 * Command line program to capture a still frame and encode it to file.
 * Also optionally display a preview/viewfinder of current camera input.
 *
 * Description
 *
 * 3 components are created; camera, preview and JPG encoder.
 * Camera component has three ports, preview, video and stills.
 * This program connects preview and stills to the preview and jpg
 * encoder. Using mmal we don't need to worry about buffers between these
 * components, but we do need to handle buffers from the encoder, which
 * are simply written straight to the file in the requisite buffer callback.
 *
 * We use the RaspiCamControl code to handle the specific camera settings.
 */

#include "RaspiStill.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <unistd.h>
#include <errno.h>

// TODO
//#include "libgps_loader.h"

#include "RaspiGPS.h"

#include <semaphore.h>
#include <math.h>
#include <pthread.h>
#include <time.h>

void default_status(RASPISTILL_STATE *state)
{
   if (!state)
   {
      vcos_assert(0);
      return;
   }

   memset(state, 0, sizeof(*state));

   raspicommonsettings_set_defaults(&state->common_settings);

   state->timeout = -1; // replaced with 5000ms later if unset
   state->quality = 85;
   state->wantRAW = 0;
   state->linkname = NULL;
   state->frameStart = 0;
   state->thumbnailConfig.enable = 1;
   state->thumbnailConfig.width = 64;
   state->thumbnailConfig.height = 48;
   state->thumbnailConfig.quality = 35;
   state->demoMode = 0;
   state->demoInterval = 250; // ms
   state->camera_component = NULL;
   state->encoder_component = NULL;
   state->preview_connection = NULL;
   state->encoder_connection = NULL;
   state->encoder_pool = NULL;
   state->encoding = MMAL_ENCODING_JPEG;
   state->numExifTags = 0;
   state->enableExifTags = 1;
   state->timelapse = 0;
   state->fullResPreview = 0;
   state->frameNextMethod = FRAME_NEXT_SINGLE;
   state->useGL = 0;
   state->glCapture = 0;
   state->burstCaptureMode=0;
   state->datetime = 0;
   state->timestamp = 0;
   state->restart_interval = 0;

   // Setup preview window defaults
   raspipreview_set_defaults(&state->preview_parameters);

   // Set up the camera_parameters to default
   raspicamcontrol_set_defaults(&state->camera_parameters);

   // Set initial GL preview state
   raspitex_set_defaults(&state->raspitex_state);
}

void dump_status(RASPISTILL_STATE *state)
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

void application_help_message(char *app_name)
{
   fprintf(stdout, "Runs camera for specific time, and take JPG capture at end if requested\n\n");
   fprintf(stdout, "usage: %s [options]\n\n", app_name);

   fprintf(stdout, "Image parameter commands\n\n");

   raspicli_display_help(cmdline_commands, cmdline_commands_size);

   raspitex_display_help();

   return;
}

int parse_cmdline(int argc, const char **argv, RASPISTILL_STATE *state)
{
   // Parse the command line arguments.
   // We are looking for --<something> or -<abbreviation of something>

   int valid = 1;
   int i;

   for (i = 1; i < argc && valid; i++)
   {
      int command_id, num_parameters;

      if (!argv[i])
         continue;

      if (argv[i][0] != '-')
      {
         valid = 0;
         continue;
      }

      // Assume parameter is valid until proven otherwise
      valid = 1;

      command_id = raspicli_get_command_id(cmdline_commands, cmdline_commands_size, &argv[i][1], &num_parameters);

      // If we found a command but are missing a parameter, continue (and we will drop out of the loop)
      if (command_id != -1 && num_parameters > 0 && (i + 1 >= argc) )
         continue;

      //  We are now dealing with a command line option
      switch (command_id)
      {
      case CommandQuality: // Quality = 1-100
         if (sscanf(argv[i + 1], "%u", &state->quality) == 1)
         {
            if (state->quality > 100)
            {
               fprintf(stderr, "Setting max quality = 100\n");
               state->quality = 100;
            }
            i++;
         }
         else
            valid = 0;
         break;

      case CommandRaw: // Add raw bayer data in metadata
         state->wantRAW = 1;
         break;

      case CommandLink :
      {
         int len = strlen(argv[i+1]);
         if (len)
         {
            state->linkname = malloc(len + 10);
            vcos_assert(state->linkname);
            if (state->linkname)
               strncpy(state->linkname, argv[i + 1], len+1);
            i++;
         }
         else
            valid = 0;
         break;

      }

      case CommandFrameStart:  // use a staring value != 0
      {
         if (sscanf(argv[i + 1], "%d", &state->frameStart) == 1)
         {
            i++;
         }
         else
            valid = 0;
         break;
      }

      case CommandDateTime: // use datetime
         state->datetime = 1;
         break;

      case CommandTimeStamp: // use timestamp
         state->timestamp = 1;
         break;

      case CommandTimeout: // Time to run viewfinder for before taking picture, in seconds
      {
         if (sscanf(argv[i + 1], "%d", &state->timeout) == 1)
         {
            // Ensure that if previously selected CommandKeypress we don't overwrite it
            if (state->timeout == 0 && state->frameNextMethod == FRAME_NEXT_SINGLE)
               state->frameNextMethod = FRAME_NEXT_FOREVER;

            i++;
         }
         else
            valid = 0;
         break;
      }

      case CommandThumbnail : // thumbnail parameters - needs string "x:y:quality"
         if ( strcmp( argv[ i + 1 ], "none" ) == 0 )
         {
            state->thumbnailConfig.enable = 0;
         }
         else
         {
            sscanf(argv[i + 1], "%d:%d:%d",
                   &state->thumbnailConfig.width,
                   &state->thumbnailConfig.height,
                   &state->thumbnailConfig.quality);
         }
         i++;
         break;

      case CommandDemoMode: // Run in demo mode - no capture
      {
         // Demo mode might have a timing parameter
         // so check if a) we have another parameter, b) its not the start of the next option
         if (i + 1 < argc  && argv[i+1][0] != '-')
         {
            if (sscanf(argv[i + 1], "%u", &state->demoInterval) == 1)
            {
               // TODO : What limits do we need for timeout?
               state->demoMode = 1;
               i++;
            }
            else
               valid = 0;
         }
         else
         {
            state->demoMode = 1;
         }

         break;
      }

      case CommandEncoding :
      {
         int len = strlen(argv[i + 1]);
         valid = 0;

         if (len)
         {
            int j;
            for (j=0; j<encoding_xref_size; j++)
            {
               if (strcmp(encoding_xref[j].format, argv[i+1]) == 0)
               {
                  state->encoding = encoding_xref[j].encoding;
                  valid = 1;
                  i++;
                  break;
               }
            }
         }
         break;
      }

      case CommandExifTag:
         if ( strcmp( argv[ i + 1 ], "none" ) == 0 )
         {
            state->enableExifTags = 0;
         }
         else
         {
            store_exif_tag(state, argv[i+1]);
         }
         i++;
         break;

      case CommandTimelapse:
         if (sscanf(argv[i + 1], "%u", &state->timelapse) != 1)
            valid = 0;
         else
         {
            if (state->timelapse)
               state->frameNextMethod = FRAME_NEXT_TIMELAPSE;
            else
               state->frameNextMethod = FRAME_NEXT_IMMEDIATELY;

            i++;
         }
         break;

      case CommandFullResPreview:
         state->fullResPreview = 1;
         break;

      case CommandKeypress: // Set keypress between capture mode
         state->frameNextMethod = FRAME_NEXT_KEYPRESS;

         if (state->timeout == -1)
            state->timeout = 0;

         break;

      case CommandSignal:   // Set SIGUSR1 & SIGUSR2 between capture mode
         state->frameNextMethod = FRAME_NEXT_SIGNAL;
         // Reenable the signal
         signal(SIGUSR1, default_signal_handler);
         signal(SIGUSR2, default_signal_handler);

         if (state->timeout == -1)
            state->timeout = 0;

         break;

      case CommandGL:
         state->useGL = 1;
         break;

      case CommandGLCapture:
         state->glCapture = 1;
         break;

      case CommandBurstMode:
         state->burstCaptureMode=1;
         break;

      case CommandRestartInterval:
      {
         if (sscanf(argv[i + 1], "%u", &state->restart_interval) == 1)
         {
            i++;
         }
         else
            valid = 0;
         break;
      }

      default:
      {
         // Try parsing for any image specific parameters
         // result indicates how many parameters were used up, 0,1,2
         // but we adjust by -1 as we have used one already
         const char *second_arg = (i + 1 < argc) ? argv[i + 1] : NULL;
         int parms_used = raspicamcontrol_parse_cmdline(&state->camera_parameters, &argv[i][1], second_arg);

         // Still unused, try common settings
         if (!parms_used)
            parms_used = raspicommonsettings_parse_cmdline(&state->common_settings, &argv[i][1], second_arg, &application_help_message);

         // Still unused, try preview settings
         if (!parms_used)
            parms_used = raspipreview_parse_cmdline(&state->preview_parameters, &argv[i][1], second_arg);

         // Still unused, try GL preview options
         if (!parms_used)
            parms_used = raspitex_parse_cmdline(&state->raspitex_state, &argv[i][1], second_arg);

         // If no parms were used, this must be a bad parameters
         if (!parms_used)
            valid = 0;
         else
            i += parms_used - 1;

         break;
      }
      }
   }

   /* GL preview parameters use preview parameters as defaults unless overriden */
   if (! state->raspitex_state.gl_win_defined)
   {
      state->raspitex_state.x       = state->preview_parameters.previewWindow.x;
      state->raspitex_state.y       = state->preview_parameters.previewWindow.y;
      state->raspitex_state.width   = state->preview_parameters.previewWindow.width;
      state->raspitex_state.height  = state->preview_parameters.previewWindow.height;
   }
   /* Also pass the preview information through so GL renderer can determine
    * the real resolution of the multi-media image */
   state->raspitex_state.preview_x       = state->preview_parameters.previewWindow.x;
   state->raspitex_state.preview_y       = state->preview_parameters.previewWindow.y;
   state->raspitex_state.preview_width   = state->preview_parameters.previewWindow.width;
   state->raspitex_state.preview_height  = state->preview_parameters.previewWindow.height;
   state->raspitex_state.opacity         = state->preview_parameters.opacity;
   state->raspitex_state.verbose         = state->common_settings.verbose;

   if (!valid)
   {
      fprintf(stderr, "Invalid command line option (%s)\n", argv[i-1]);
      return 1;
   }

   return 0;
}

void camera_buffer_callback ( MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer )
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

         memcpy ( pData->im_buffer, buffer, buffer->length );
         //bytes_written = fwrite(buffer->data, 1, buffer->length, pData->file_handle);

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

MMAL_STATUS_T create_camera_component(RASPISTILL_STATE *state)
{
   MMAL_COMPONENT_T *camera = 0;
   MMAL_ES_FORMAT_T *format;
   MMAL_PORT_T *preview_port = NULL, *video_port = NULL, *still_port = NULL;
   MMAL_STATUS_T status;

   /* Create the component */
   status = mmal_component_create(MMAL_COMPONENT_DEFAULT_CAMERA, &camera);

   if (status != MMAL_SUCCESS)
   {
      vcos_log_error("Failed to create camera component");
      goto error;
   }

   status = raspicamcontrol_set_stereo_mode(camera->output[0], &state->camera_parameters.stereo_mode);
   status += raspicamcontrol_set_stereo_mode(camera->output[1], &state->camera_parameters.stereo_mode);
   status += raspicamcontrol_set_stereo_mode(camera->output[2], &state->camera_parameters.stereo_mode);

   if (status != MMAL_SUCCESS)
   {
      vcos_log_error("Could not set stereo mode : error %d", status);
      goto error;
   }

   MMAL_PARAMETER_INT32_T camera_num =
   {{MMAL_PARAMETER_CAMERA_NUM, sizeof(camera_num)}, state->common_settings.cameraNum};

   status = mmal_port_parameter_set(camera->control, &camera_num.hdr);

   if (status != MMAL_SUCCESS)
   {
      vcos_log_error("Could not select camera : error %d", status);
      goto error;
   }

   if (!camera->output_num)
   {
      status = MMAL_ENOSYS;
      vcos_log_error("Camera doesn't have output ports");
      goto error;
   }

   status = mmal_port_parameter_set_uint32(camera->control, MMAL_PARAMETER_CAMERA_CUSTOM_SENSOR_CONFIG, state->common_settings.sensor_mode);

   if (status != MMAL_SUCCESS)
   {
      vcos_log_error("Could not set sensor mode : error %d", status);
      goto error;
   }

   preview_port = camera->output[MMAL_CAMERA_PREVIEW_PORT];
   video_port = camera->output[MMAL_CAMERA_VIDEO_PORT];
   still_port = camera->output[MMAL_CAMERA_CAPTURE_PORT];

   // Enable the camera, and tell it its control callback function
   status = mmal_port_enable(camera->control, default_camera_control_callback);

   if (status != MMAL_SUCCESS)
   {
      vcos_log_error("Unable to enable control port : error %d", status);
      goto error;
   }

   //  set up the camera configuration
   {
      MMAL_PARAMETER_CAMERA_CONFIG_T cam_config =
      {
         { MMAL_PARAMETER_CAMERA_CONFIG, sizeof(cam_config) },
         .max_stills_w = state->common_settings.width,
         .max_stills_h = state->common_settings.height,
         .stills_yuv422 = 0,
         .one_shot_stills = 1,
         .max_preview_video_w = state->preview_parameters.previewWindow.width,
         .max_preview_video_h = state->preview_parameters.previewWindow.height,
         .num_preview_video_frames = 3,
         .stills_capture_circular_buffer_height = 0,
         .fast_preview_resume = 0,
         .use_stc_timestamp = MMAL_PARAM_TIMESTAMP_MODE_RESET_STC
      };

      if (state->fullResPreview)
      {
         cam_config.max_preview_video_w = state->common_settings.width;
         cam_config.max_preview_video_h = state->common_settings.height;
      }

      mmal_port_parameter_set(camera->control, &cam_config.hdr);
   }

   raspicamcontrol_set_all_parameters(camera, &state->camera_parameters);

   // Now set up the port formats

   format = preview_port->format;
   format->encoding = MMAL_ENCODING_OPAQUE;
   format->encoding_variant = MMAL_ENCODING_I420;

   if(state->camera_parameters.shutter_speed > 6000000)
   {
      MMAL_PARAMETER_FPS_RANGE_T fps_range = {{MMAL_PARAMETER_FPS_RANGE, sizeof(fps_range)},
         { 50, 1000 }, {166, 1000}
      };
      mmal_port_parameter_set(preview_port, &fps_range.hdr);
   }
   else if(state->camera_parameters.shutter_speed > 1000000)
   {
      MMAL_PARAMETER_FPS_RANGE_T fps_range = {{MMAL_PARAMETER_FPS_RANGE, sizeof(fps_range)},
         { 166, 1000 }, {999, 1000}
      };
      mmal_port_parameter_set(preview_port, &fps_range.hdr);
   }
   if (state->fullResPreview)
   {
      // In this mode we are forcing the preview to be generated from the full capture resolution.
      // This runs at a max of 15fps with the OV5647 sensor.
      format->es->video.width = VCOS_ALIGN_UP(state->common_settings.width, 32);
      format->es->video.height = VCOS_ALIGN_UP(state->common_settings.height, 16);
      format->es->video.crop.x = 0;
      format->es->video.crop.y = 0;
      format->es->video.crop.width = state->common_settings.width;
      format->es->video.crop.height = state->common_settings.height;
      format->es->video.frame_rate.num = FULL_RES_PREVIEW_FRAME_RATE_NUM;
      format->es->video.frame_rate.den = FULL_RES_PREVIEW_FRAME_RATE_DEN;
   }
   else
   {
      // Use a full FOV 4:3 mode
      format->es->video.width = VCOS_ALIGN_UP(state->preview_parameters.previewWindow.width, 32);
      format->es->video.height = VCOS_ALIGN_UP(state->preview_parameters.previewWindow.height, 16);
      format->es->video.crop.x = 0;
      format->es->video.crop.y = 0;
      format->es->video.crop.width = state->preview_parameters.previewWindow.width;
      format->es->video.crop.height = state->preview_parameters.previewWindow.height;
      format->es->video.frame_rate.num = PREVIEW_FRAME_RATE_NUM;
      format->es->video.frame_rate.den = PREVIEW_FRAME_RATE_DEN;
   }

   status = mmal_port_format_commit(preview_port);
   if (status != MMAL_SUCCESS)
   {
      vcos_log_error("camera viewfinder format couldn't be set");
      goto error;
   }

   // Set the same format on the video  port (which we don't use here)
   mmal_format_full_copy(video_port->format, format);
   status = mmal_port_format_commit(video_port);

   if (status  != MMAL_SUCCESS)
   {
      vcos_log_error("camera video format couldn't be set");
      goto error;
   }

   // Ensure there are enough buffers to avoid dropping frames
   if (video_port->buffer_num < VIDEO_OUTPUT_BUFFERS_NUM)
      video_port->buffer_num = VIDEO_OUTPUT_BUFFERS_NUM;

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
      vcos_log_error("camera still format couldn't be set");
      goto error;
   }

   /* Ensure there are enough buffers to avoid dropping frames */
   if (still_port->buffer_num < VIDEO_OUTPUT_BUFFERS_NUM)
      still_port->buffer_num = VIDEO_OUTPUT_BUFFERS_NUM;

   /* Create pool of buffer headers for the output port to consume */
   MMAL_POOL_T* pool = mmal_port_pool_create ( still_port,
                                               still_port->buffer_num,
                                               still_port->buffer_size);

    if (!pool)
    {
        vcos_log_error("Failed to create buffer header pool for encoder output port %s",
                       still_port->name);
    }

    state->encoder_pool = pool;

   /* Enable component */
   status = mmal_component_enable(camera);

   if (status != MMAL_SUCCESS)
   {
      vcos_log_error("camera component couldn't be enabled");
      goto error;
   }

   if (state->useGL)
   {
      status = raspitex_configure_preview_port(&state->raspitex_state, preview_port);
      if (status != MMAL_SUCCESS)
      {
         fprintf(stderr, "Failed to configure preview port for GL rendering");
         goto error;
      }
   }

   state->camera_component = camera;

   fprintf(stderr, "Created camera component\n");

   return status;

error:

   if (camera)
      mmal_component_destroy(camera);

   return status;
}

void destroy_camera_component(RASPISTILL_STATE *state)
{
   if (state->camera_component)
   {
      mmal_component_destroy(state->camera_component);
      state->camera_component = NULL;
   }
}

MMAL_STATUS_T create_encoder_component(RASPISTILL_STATE *state)
{
   MMAL_COMPONENT_T *encoder = 0;
   MMAL_PORT_T *encoder_input = NULL, *encoder_output = NULL;
   MMAL_STATUS_T status;
   MMAL_POOL_T *pool;

   status = mmal_component_create(MMAL_COMPONENT_DEFAULT_IMAGE_ENCODER, &encoder);

   if (status != MMAL_SUCCESS)
   {
      vcos_log_error("Unable to create JPEG encoder component");
      goto error;
   }

   if (!encoder->input_num || !encoder->output_num)
   {
      status = MMAL_ENOSYS;
      vcos_log_error("JPEG encoder doesn't have input/output ports");
      goto error;
   }

   encoder_input = encoder->input[0];
   encoder_output = encoder->output[0];

   // We want same format on input and output
   mmal_format_copy(encoder_output->format, encoder_input->format);

   // Specify out output format
   encoder_output->format->encoding = state->encoding;

   encoder_output->buffer_size = encoder_output->buffer_size_recommended;

   if (encoder_output->buffer_size < encoder_output->buffer_size_min)
      encoder_output->buffer_size = encoder_output->buffer_size_min;

   encoder_output->buffer_num = encoder_output->buffer_num_recommended;

   if (encoder_output->buffer_num < encoder_output->buffer_num_min)
      encoder_output->buffer_num = encoder_output->buffer_num_min;

   // Commit the port changes to the output port
   status = mmal_port_format_commit(encoder_output);

   if (status != MMAL_SUCCESS)
   {
      vcos_log_error("Unable to set format on video encoder output port");
      goto error;
   }

   // Set the JPEG quality level
   status = mmal_port_parameter_set_uint32(encoder_output, MMAL_PARAMETER_JPEG_Q_FACTOR, state->quality);

   if (status != MMAL_SUCCESS)
   {
      vcos_log_error("Unable to set JPEG quality");
      goto error;
   }

   // Set the JPEG restart interval
   status = mmal_port_parameter_set_uint32(encoder_output, MMAL_PARAMETER_JPEG_RESTART_INTERVAL, state->restart_interval);

   if (state->restart_interval && status != MMAL_SUCCESS)
   {
      vcos_log_error("Unable to set JPEG restart interval");
      goto error;
   }

   // Set up any required thumbnail
   {
      MMAL_PARAMETER_THUMBNAIL_CONFIG_T param_thumb = {{MMAL_PARAMETER_THUMBNAIL_CONFIGURATION, sizeof(MMAL_PARAMETER_THUMBNAIL_CONFIG_T)}, 0, 0, 0, 0};

      if ( state->thumbnailConfig.enable &&
            state->thumbnailConfig.width > 0 && state->thumbnailConfig.height > 0 )
      {
         // Have a valid thumbnail defined
         param_thumb.enable = 1;
         param_thumb.width = state->thumbnailConfig.width;
         param_thumb.height = state->thumbnailConfig.height;
         param_thumb.quality = state->thumbnailConfig.quality;
      }
      status = mmal_port_parameter_set(encoder->control, &param_thumb.hdr);
   }

   //  Enable component
   status = mmal_component_enable(encoder);

   if (status  != MMAL_SUCCESS)
   {
      vcos_log_error("Unable to enable video encoder component");
      goto error;
   }

   /* Create pool of buffer headers for the output port to consume */
   pool = mmal_port_pool_create(encoder_output, encoder_output->buffer_num, encoder_output->buffer_size);

   if (!pool)
   {
      vcos_log_error("Failed to create buffer header pool for encoder output port %s", encoder_output->name);
   }

   state->encoder_pool = pool;
   state->encoder_component = encoder;

   if (state->common_settings.verbose)
      fprintf(stderr, "Encoder component done\n");

   return status;

error:

   if (encoder)
      mmal_component_destroy(encoder);

   return status;
}

void destroy_encoder_component(RASPISTILL_STATE *state)
{
   // Get rid of any port buffers first
   if (state->encoder_pool)
   {
      mmal_port_pool_destroy(state->encoder_component->output[0], state->encoder_pool);
   }

   if (state->encoder_component)
   {
      mmal_component_destroy(state->encoder_component);
      state->encoder_component = NULL;
   }
}

MMAL_STATUS_T add_exif_tag(RASPISTILL_STATE *state, const char *exif_tag)
{
   MMAL_STATUS_T status;
   MMAL_PARAMETER_EXIF_T *exif_param = (MMAL_PARAMETER_EXIF_T*)calloc(sizeof(MMAL_PARAMETER_EXIF_T) + MAX_EXIF_PAYLOAD_LENGTH, 1);

   vcos_assert(state);
   vcos_assert(state->encoder_component);

   // Check to see if the tag is present or is indeed a key=value pair.
   if (!exif_tag || strchr(exif_tag, '=') == NULL || strlen(exif_tag) > MAX_EXIF_PAYLOAD_LENGTH-1)
      return MMAL_EINVAL;

   exif_param->hdr.id = MMAL_PARAMETER_EXIF;

   strncpy((char*)exif_param->data, exif_tag, MAX_EXIF_PAYLOAD_LENGTH-1);

   exif_param->hdr.size = sizeof(MMAL_PARAMETER_EXIF_T) + strlen((char*)exif_param->data);

   status = mmal_port_parameter_set(state->encoder_component->output[0], &exif_param->hdr);

   free(exif_param);

   return status;
}

void add_exif_tags(RASPISTILL_STATE *state)
{
   time_t rawtime;
   struct tm *timeinfo;
   char model_buf[32];
   char time_buf[32];
   char exif_buf[128];
   int i;

   snprintf(model_buf, 32, "IFD0.Model=RP_%s", state->common_settings.camera_name);
   add_exif_tag(state, model_buf);
   add_exif_tag(state, "IFD0.Make=RaspberryPi");

   time(&rawtime);
   timeinfo = localtime(&rawtime);

   snprintf(time_buf, sizeof(time_buf),
            "%04d:%02d:%02d %02d:%02d:%02d",
            timeinfo->tm_year+1900,
            timeinfo->tm_mon+1,
            timeinfo->tm_mday,
            timeinfo->tm_hour,
            timeinfo->tm_min,
            timeinfo->tm_sec);

   snprintf(exif_buf, sizeof(exif_buf), "EXIF.DateTimeDigitized=%s", time_buf);
   add_exif_tag(state, exif_buf);

   snprintf(exif_buf, sizeof(exif_buf), "EXIF.DateTimeOriginal=%s", time_buf);
   add_exif_tag(state, exif_buf);

   snprintf(exif_buf, sizeof(exif_buf), "IFD0.DateTime=%s", time_buf);
   add_exif_tag(state, exif_buf);


   // Add GPS tags
   if (state->common_settings.gps)
   {
      // clear all existing tags first
      add_exif_tag(state, "GPS.GPSDateStamp=");
      add_exif_tag(state, "GPS.GPSTimeStamp=");
      add_exif_tag(state, "GPS.GPSMeasureMode=");
      add_exif_tag(state, "GPS.GPSSatellites=");
      add_exif_tag(state, "GPS.GPSLatitude=");
      add_exif_tag(state, "GPS.GPSLatitudeRef=");
      add_exif_tag(state, "GPS.GPSLongitude=");
      add_exif_tag(state, "GPS.GPSLongitudeRef=");
      add_exif_tag(state, "GPS.GPSAltitude=");
      add_exif_tag(state, "GPS.GPSAltitudeRef=");
      add_exif_tag(state, "GPS.GPSSpeed=");
      add_exif_tag(state, "GPS.GPSSpeedRef=");
      add_exif_tag(state, "GPS.GPSTrack=");
      add_exif_tag(state, "GPS.GPSTrackRef=");
   }

   // Now send any user supplied tags

   for (i=0; i<state->numExifTags && i < MAX_USER_EXIF_TAGS; i++)
   {
      if (state->exifTags[i])
      {
         add_exif_tag(state, state->exifTags[i]);
      }
   }
}

void store_exif_tag(RASPISTILL_STATE *state, const char *exif_tag)
{
   if (state->numExifTags < MAX_USER_EXIF_TAGS)
   {
      state->exifTags[state->numExifTags] = exif_tag;
      state->numExifTags++;
   }
}

MMAL_STATUS_T create_filenames(char** finalName, char** tempName, char * pattern, int frame)
{
   *finalName = NULL;
   *tempName = NULL;
   if (0 > asprintf(finalName, pattern, frame) ||
         0 > asprintf(tempName, "%s~", *finalName))
   {
      if (*finalName != NULL)
      {
         free(*finalName);
      }
      return MMAL_ENOMEM;    // It may be some other error, but it is not worth getting it right
   }
   return MMAL_SUCCESS;
}

int wait_for_next_frame(RASPISTILL_STATE *state, int *frame)
{
   static int64_t complete_time = -1;
   int keep_running = 1;

   int64_t current_time =  get_microseconds64()/1000;

   if (complete_time == -1)
      complete_time =  current_time + state->timeout;

   // if we have run out of time, flag we need to exit
   // If timeout = 0 then always continue
   if (current_time >= complete_time && state->timeout != 0)
      keep_running = 0;

   switch (state->frameNextMethod)
   {
   case FRAME_NEXT_SINGLE :
      // simple timeout for a single capture
      vcos_sleep(state->timeout);
      return 0;

   case FRAME_NEXT_FOREVER :
   {
      *frame+=1;

      // Have a sleep so we don't hog the CPU.
      vcos_sleep(10000);

      // Run forever so never indicate end of loop
      return 1;
   }

   case FRAME_NEXT_TIMELAPSE :
   {
      static int64_t next_frame_ms = -1;

      // Always need to increment by at least one, may add a skip later
      *frame += 1;

      if (next_frame_ms == -1)
      {
         vcos_sleep(CAMERA_SETTLE_TIME);

         // Update our current time after the sleep
         current_time =  get_microseconds64()/1000;

         // Set our initial 'next frame time'
         next_frame_ms = current_time + state->timelapse;
      }
      else
      {
         int64_t this_delay_ms = next_frame_ms - current_time;

         if (this_delay_ms < 0)
         {
            // We are already past the next exposure time
            if (-this_delay_ms < state->timelapse/2)
            {
               // Less than a half frame late, take a frame and hope to catch up next time
               next_frame_ms += state->timelapse;
               vcos_log_error("Frame %d is %d ms late", *frame, (int)(-this_delay_ms));
            }
            else
            {
               int nskip = 1 + (-this_delay_ms)/state->timelapse;
               vcos_log_error("Skipping frame %d to restart at frame %d", *frame, *frame+nskip);
               *frame += nskip;
               this_delay_ms += nskip * state->timelapse;
               vcos_sleep(this_delay_ms);
               next_frame_ms += (nskip + 1) * state->timelapse;
            }
         }
         else
         {
            vcos_sleep(this_delay_ms);
            next_frame_ms += state->timelapse;
         }
      }

      return keep_running;
   }

   case FRAME_NEXT_KEYPRESS :
   {
      int ch;

      if (state->common_settings.verbose)
         fprintf(stderr, "Press Enter to capture, X then ENTER to exit\n");

      ch = getchar();
      *frame+=1;
      if (ch == 'x' || ch == 'X')
         return 0;
      else
      {
         return keep_running;
      }
   }

   case FRAME_NEXT_IMMEDIATELY :
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

      return keep_running;
   }

   case FRAME_NEXT_GPIO :
   {
      // Intended for GPIO firing of a capture
      return 0;
   }

   case FRAME_NEXT_SIGNAL :
   {
      // Need to wait for a SIGUSR1 or SIGUSR2 signal
      sigset_t waitset;
      int sig;
      int result = 0;

      sigemptyset( &waitset );
      sigaddset( &waitset, SIGUSR1 );
      sigaddset( &waitset, SIGUSR2 );

      // We are multi threaded because we use mmal, so need to use the pthread
      // variant of procmask to block until a SIGUSR1 or SIGUSR2 signal appears
      pthread_sigmask( SIG_BLOCK, &waitset, NULL );

      if (state->common_settings.verbose)
      {
         fprintf(stderr, "Waiting for SIGUSR1 to initiate capture and continue or SIGUSR2 to capture and exit\n");
      }

      result = sigwait( &waitset, &sig );

      if (result == 0)
      {
         if (sig == SIGUSR1)
         {
            if (state->common_settings.verbose)
               fprintf(stderr, "Received SIGUSR1\n");
         }
         else if (sig == SIGUSR2)
         {
            if (state->common_settings.verbose)
               fprintf(stderr, "Received SIGUSR2\n");
            keep_running = 0;
         }
      }
      else
      {
         if (state->common_settings.verbose)
            fprintf(stderr, "Bad signal received - error %d\n", errno);
      }

      *frame+=1;

      return keep_running;
   }
   } // end of switch

   // Should have returned by now, but default to timeout
   return keep_running;
}

void rename_file(RASPISTILL_STATE *state, FILE *output_file,
                        const char *final_filename, const char *use_filename, int frame)
{
   MMAL_STATUS_T status;

   fclose(output_file);
   vcos_assert(use_filename != NULL && final_filename != NULL);
   if (0 != rename(use_filename, final_filename))
   {
      vcos_log_error("Could not rename temp file to: %s; %s",
                     final_filename,strerror(errno));
   }
   if (state->linkname)
   {
      char *use_link;
      char *final_link;
      status = create_filenames(&final_link, &use_link, state->linkname, frame);

      // Create hard link if possible, symlink otherwise
      if (status != MMAL_SUCCESS
            || (0 != link(final_filename, use_link)
                &&  0 != symlink(final_filename, use_link))
            || 0 != rename(use_link, final_link))
      {
         vcos_log_error("Could not link as filename: %s; %s",
                        state->linkname,strerror(errno));
      }
      if (use_link) free(use_link);
      if (final_link) free(final_link);
   }
}

