# Slimmed down version of raspistill

SET(COMPILE_DEFINITIONS -Werror)

# Set --no-as-needed to stop the linker discarding mmal_vc_client
# as it can't see that the constructor registers a load of functionality
# with the MMAL core.
SET( CMAKE_EXE_LINKER_FLAGS "-Wl,--no-as-needed" )

include_directories ( ${PROJECT_SOURCE_DIR} )
include_directories ( /opt/vc/include )

# Find the commit hash of the build and pass to the compiler
execute_process(
  COMMAND git log -1 --abbrev=12 --format=%h
  OUTPUT_VARIABLE GIT_COMMIT_ID
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
add_definitions("-DGIT_COMMIT_ID=\"${GIT_COMMIT_ID}\"")

# Determine if we are tainted
execute_process(
  COMMAND bash "-c" "git ls-files -m | wc -l"
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_TAINTED
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
add_definitions("-DGIT_TAINTED=${GIT_TAINTED}")

set (COMMON_SOURCES
   RaspiCamControl.c
   RaspiCLI.c
   RaspiPreview.c
   RaspiCommonSettings.c
   RaspiHelpers.c
   RaspiGPS.c
   libgps_loader.c)

set (EGL_SOURCES RaspiTexStub.c)

add_executable(raspistill ${COMMON_SOURCES} RaspiStill.c ${EGL_SOURCES} )

set ( MMAL_LIBS /opt/vc/lib/libmmal_core.so
                /opt/vc/lib/libmmal_util.so
                /opt/vc/lib/libmmal_vc_client.so )
set ( OTHER_LIBS /opt/vc/lib/libbcm_host.so
                 /opt/vc/lib/libvcos.so )
#set (MMAL_LIBS mmal_core mmal_util mmal_vc_client)
target_link_libraries(raspistill ${MMAL_LIBS} ${OTHER_LIBS} ${EGL_LIBS} m dl pthread)
