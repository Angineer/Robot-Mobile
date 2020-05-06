#include "RaspiCam.h"
#include "RaspiGPS.h" // Defines booleans

#include <sysexits.h>

// We use some GNU extensions (asprintf, basename)
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

RASPISTILL_STATE* createCam()
{
    // Our main data storage vessel..
    RASPISTILL_STATE* state = (RASPISTILL_STATE*) malloc (sizeof(RASPISTILL_STATE));

    MMAL_STATUS_T status = MMAL_SUCCESS;
    MMAL_PORT_T *camera_preview_port = NULL;
    MMAL_PORT_T *camera_still_port = NULL;
    MMAL_PORT_T *preview_input_port = NULL;
    MMAL_PORT_T *encoder_input_port = NULL;

    bcm_host_init();

    // Register our application with the logging system
    vcos_log_register("RaspiStill", VCOS_LOG_CATEGORY);

    // Set signal handlers. We won't be using the USR signals.
    signal(SIGINT, default_signal_handler);
    signal(SIGUSR1, SIG_IGN);
    signal(SIGUSR2, SIG_IGN);

    default_status(state);
 
    // Setup for sensor specific parameters
    get_sensor_defaults(state->common_settings.cameraNum, state->common_settings.camera_name,
                        &state->common_settings.width, &state->common_settings.height);

    print_app_details(stderr);

    // Set up the MMAL camera component
    if ((status = create_camera_component(state)) != MMAL_SUCCESS)
    {
       vcos_log_error("%s: Failed to create camera component", __func__);
       free ( state );
       return NULL;
    }

   return state;
}

void destroyCam ( RASPISTILL_STATE* state ) {
    MMAL_STATUS_T status = MMAL_SUCCESS;

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

void capture ( RASPISTILL_STATE* state, void* im_buffer ) {
    MMAL_STATUS_T status = MMAL_SUCCESS;
    PORT_USERDATA callback_data;
    VCOS_STATUS_T vcos_status;

    MMAL_PORT_T* camera_still_port = state->camera_component->output[MMAL_CAMERA_CAPTURE_PORT];

    // Set up our userdata - this is passed though to the callback where we need the information.
    callback_data.im_buffer = im_buffer;
    callback_data.pstate = state;
    vcos_status = vcos_semaphore_create(&callback_data.complete_semaphore, "RaspiStill-sem", 0);

    vcos_assert(vcos_status == VCOS_SUCCESS);

    // Capture a single image
    int frame = state->frameStart - 1;

    // Disable stuff we aren't going to use
    mmal_port_parameter_set_boolean(
        camera_still_port, MMAL_PARAMETER_ENABLE_RAW_CAPTURE, 0);

    // There is a possibility that shutter needs to be set each loop.
    if ( mmal_status_to_int (
            mmal_port_parameter_set_uint32 (
                state->camera_component->control,
                MMAL_PARAMETER_SHUTTER_SPEED,
                state->camera_parameters.shutter_speed ) ) != MMAL_SUCCESS ) {
       vcos_log_error("Unable to set shutter speed");
    }

    // Enable the camera output port and tell it its callback function
    fprintf(stderr, "Enabling camera output port\n");

    camera_still_port->userdata = (struct MMAL_PORT_USERDATA_T *)&callback_data;
    status = mmal_port_enable(camera_still_port, camera_buffer_callback);

    // Send all the buffers to the camera output port
    int num = mmal_queue_length(state->encoder_pool->queue);

    for (int q=0; q<num; q++)
    {
       MMAL_BUFFER_HEADER_T *buffer = mmal_queue_get(state->encoder_pool->queue);

       if (!buffer)
          vcos_log_error("Unable to get a required buffer %d from pool queue", q);

       if (mmal_port_send_buffer(camera_still_port, buffer)!= MMAL_SUCCESS)
          vcos_log_error("Unable to send a buffer to camera output port (%d)", q);
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

    // Clean up
    status = mmal_port_disable(camera_still_port);
    vcos_semaphore_delete(&callback_data.complete_semaphore);
}

