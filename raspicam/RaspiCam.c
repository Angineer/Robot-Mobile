#include "RaspiCam.h"
#include "RaspiGPS.h" // Defines booleans

#include <sysexits.h>

// We use some GNU extensions (asprintf, basename)
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

RASPISTILL_STATE* createCam ( int argc, const char** argv )
{
    // Our main data storage vessel..
    RASPISTILL_STATE* state = (RASPISTILL_STATE*) malloc (sizeof(RASPISTILL_STATE));
    //int exit_code = EX_OK;

    MMAL_STATUS_T status = MMAL_SUCCESS;
    MMAL_PORT_T *camera_preview_port = NULL;
    MMAL_PORT_T *camera_video_port = NULL;
    MMAL_PORT_T *camera_still_port = NULL;
    MMAL_PORT_T *preview_input_port = NULL;
    MMAL_PORT_T *encoder_input_port = NULL;
    MMAL_PORT_T *encoder_output_port = NULL;

    bcm_host_init();

    // Register our application with the logging system
    vcos_log_register("RaspiStill", VCOS_LOG_CATEGORY);

    signal(SIGINT, default_signal_handler);

    // Disable USR1 and USR2 for the moment - may be reenabled if go in to signal capture mode
    signal(SIGUSR1, SIG_IGN);
    signal(SIGUSR2, SIG_IGN);

    //set_app_name(argv[0]);

    // Do we have any parameters
    if (argc == 1)
    {
       display_valid_parameters(basename(argv[0]), &application_help_message);
       exit(EX_USAGE);
    }
 
    default_status(state);
 
    // Parse the command line and put options in to our status structure
    if (parse_cmdline(argc, argv, state))
    {
       exit(EX_USAGE);
    }

    // Run forever by default
    if (state->timeout == -1)
       state->timeout = 0;

    // Setup for sensor specific parameters
    get_sensor_defaults(state->common_settings.cameraNum, state->common_settings.camera_name,
                        &state->common_settings.width, &state->common_settings.height);

    if (state->common_settings.verbose)
    {
       print_app_details(stderr);
       dump_status(state);
    }

    //if (state->common_settings.gps)
    //{
       //if (raspi_gps_setup(state->common_settings.verbose))
          //state->common_settings.gps = false;
    //}

    if (state->useGL)
       raspitex_init(&state->raspitex_state);

    // OK, we have a nice set of parameters. Now set up our components
    // We have three components. Camera, Preview and encoder.
    // Camera and encoder are different in stills/video, but preview
    // is the same so handed off to a separate module

    if ((status = create_camera_component(state)) != MMAL_SUCCESS)
    {
       vcos_log_error("%s: Failed to create camera component", __func__);
       //exit_code = EX_SOFTWARE;
       return NULL;
    }
    else if ((!state->useGL) && (status = raspipreview_create(&state->preview_parameters)) != MMAL_SUCCESS)
    {
       vcos_log_error("%s: Failed to create preview component", __func__);
       destroy_camera_component(state);
       //exit_code = EX_SOFTWARE;
       return NULL;
    }
    else if ((status = create_encoder_component(state)) != MMAL_SUCCESS)
    {
       vcos_log_error("%s: Failed to create encode component", __func__);
       raspipreview_destroy(&state->preview_parameters);
       destroy_camera_component(state);
       //exit_code = EX_SOFTWARE;
       return NULL;
    }
   
    if (state->common_settings.verbose)
        fprintf(stderr, "Starting component connection stage\n");

    if (! state->useGL)
    {
        if (state->common_settings.verbose)
            fprintf(stderr, "Connecting camera preview port to video render.\n");

        // Note we are lucky that the preview and null sink components use the same input port
        // so we can simple do this without conditionals
        preview_input_port  = state->preview_parameters.preview_component->input[0];

        // Connect camera to preview (which might be a null_sink if no preview required)
        status = connect_ports(camera_preview_port, preview_input_port, &state->preview_connection);
    }

    if (status != MMAL_SUCCESS)
    {
        mmal_status_to_int(status);
        vcos_log_error("%s: Failed to connect camera to preview", __func__);
        // TODO
    }

    if (state->common_settings.verbose)
        fprintf(stderr, "Connecting camera stills port to encoder input port\n");

    // Now connect the camera to the encoder
    status = connect_ports(camera_still_port, encoder_input_port, &state->encoder_connection);

    if (status != MMAL_SUCCESS)
    {
        vcos_log_error("%s: Failed to connect camera video port to encoder input", __func__);
        // TODO
    }

   return state;
}

void destroyCam ( RASPISTILL_STATE* state ) {
    MMAL_STATUS_T status = MMAL_SUCCESS;
    MMAL_PORT_T *camera_video_port = NULL;
    MMAL_PORT_T *encoder_output_port = NULL;

    mmal_status_to_int(status);

    if (state->common_settings.verbose)
        fprintf(stderr, "Closing down\n");

    if (state->useGL)
    {
        raspitex_stop(&state->raspitex_state);
        raspitex_destroy(&state->raspitex_state);
    }

    // Disable all our ports that are not handled by connections
    check_disable_port(state->camera_component->output[MMAL_CAMERA_VIDEO_PORT]);
    check_disable_port(state->encoder_component->output[0]);

    if (state->preview_connection)
        mmal_connection_destroy(state->preview_connection);

    if (state->encoder_connection)
        mmal_connection_destroy(state->encoder_connection);

    /* Disable components */
    if (state->encoder_component)
        mmal_component_disable(state->encoder_component);

    if (state->preview_parameters.preview_component)
        mmal_component_disable(state->preview_parameters.preview_component);

    if (state->camera_component)
        mmal_component_disable(state->camera_component);

    destroy_encoder_component(state);
    raspipreview_destroy(&state->preview_parameters);
    destroy_camera_component(state);

    if (state->common_settings.verbose)
        fprintf(stderr, "Close down completed, all components disconnected, disabled and destroyed\n\n");

    //if (state->common_settings.gps)
        //raspi_gps_shutdown(state->common_settings.verbose);

    if (status != MMAL_SUCCESS)
        raspicamcontrol_check_configuration(128);

    free ( state );
}

void capture ( RASPISTILL_STATE* state ) {
    MMAL_STATUS_T status = MMAL_SUCCESS;
    MMAL_PORT_T *camera_still_port = NULL;
    MMAL_PORT_T *encoder_output_port = NULL;

    PORT_USERDATA callback_data;
    VCOS_STATUS_T vcos_status;

    camera_still_port   = state->camera_component->output[MMAL_CAMERA_CAPTURE_PORT];
    encoder_output_port = state->encoder_component->output[0];

    // Set up our userdata - this is passed though to the callback where we need the information.
    // Null until we open our filename
    callback_data.file_handle = NULL;
    callback_data.pstate = state;
    vcos_status = vcos_semaphore_create(&callback_data.complete_semaphore, "RaspiStill-sem", 0);

    vcos_assert(vcos_status == VCOS_SUCCESS);

    /* If GL preview is requested then start the GL threads */
    if (state->useGL && (raspitex_start(&state->raspitex_state) != 0))
        goto error;

    /**********************************/
    // Capture a single image
    int frame, keep_looping = 1;
    FILE *output_file = NULL;

    frame = state->frameStart - 1;

    // Keep it simple for the moment
    output_file = stdout;
    callback_data.file_handle = output_file;

    if (output_file)
    {
        int num, q;

        // Disable EXIF
        {
           mmal_port_parameter_set_boolean(
              state->encoder_component->output[0], MMAL_PARAMETER_EXIF_DISABLE, 1);
        }

        // Same with raw, apparently need to set it for each capture, whilst port
        // is not enabled
        if (state->wantRAW)
        {
           if (mmal_port_parameter_set_boolean(camera_still_port, MMAL_PARAMETER_ENABLE_RAW_CAPTURE, 1) != MMAL_SUCCESS)
           {
              vcos_log_error("RAW was requested, but failed to enable");
           }
        }

        // There is a possibility that shutter needs to be set each loop.
        if (mmal_status_to_int(mmal_port_parameter_set_uint32(state->camera_component->control, MMAL_PARAMETER_SHUTTER_SPEED, state->camera_parameters.shutter_speed)) != MMAL_SUCCESS)
           vcos_log_error("Unable to set shutter speed");

        // Enable the encoder output port and tell it its callback function
        fprintf(stderr, "Enabling encoder output port\n");

        encoder_output_port->userdata = (struct MMAL_PORT_USERDATA_T *)&callback_data;
        status = mmal_port_enable(encoder_output_port, encoder_buffer_callback);

        // Send all the buffers to the encoder output port
        num = mmal_queue_length(state->encoder_pool->queue);

        for (q=0; q<num; q++)
        {
           MMAL_BUFFER_HEADER_T *buffer = mmal_queue_get(state->encoder_pool->queue);

           if (!buffer)
              vcos_log_error("Unable to get a required buffer %d from pool queue", q);

           if (mmal_port_send_buffer(encoder_output_port, buffer)!= MMAL_SUCCESS)
              vcos_log_error("Unable to send a buffer to encoder output port (%d)", q);
        }

        fprintf(stderr, "Starting capture %d\n", frame);

        // Perform the capture
        if (mmal_port_parameter_set_boolean(camera_still_port, MMAL_PARAMETER_CAPTURE, 1) != MMAL_SUCCESS)
        {
           vcos_log_error("%s: Failed to start capture", __func__);
        }
        else
        {
           // Wait for capture to complete
           // For some reason using vcos_semaphore_wait_timeout sometimes returns immediately with bad parameter error
           // even though it appears to be all correct, so reverting to untimed one until figure out why its erratic
           vcos_semaphore_wait(&callback_data.complete_semaphore);
           if (state->common_settings.verbose)
              fprintf(stderr, "Finished capture %d\n", frame);
        }

        // Ensure we don't die if get callback with no open file
        callback_data.file_handle = NULL;

        fflush(output_file);

        // Disable encoder output port
        status = mmal_port_disable(encoder_output_port);
    }

    vcos_semaphore_delete(&callback_data.complete_semaphore);
    /**********************************/
error:
     vcos_log_error ( "Foo" );
     // TODO
}

