cmake_minimum_required ( VERSION 2.6 )
project ( robot-mobile )

# Need threads
find_package ( Threads )

# Also build custom version of raspistill
add_subdirectory ( raspicam )

# Cereal
set ( CEREAL_INCLUDE_FOLDER ../cereal/include CACHE FILEPATH "Cereal include folder" )

# AprilTags
set ( APRILTAG_INCLUDE_FOLDER /usr/local/include/apriltag CACHE FILEPATH "AprilTag include folder" )

# Mobile server
add_executable ( mobile_server ./src/mobile_server.cpp
                               ./src/CameraFetcher.cpp
                               ./src/ImageBuffer.cpp
                               ./src/ImageChecker.cpp
                               ./src/MobileConfiguration.cpp
                               ./src/MobileManager.cpp )
set_property ( TARGET mobile_server PROPERTY CXX_STANDARD 14 )
target_include_directories ( mobile_server PUBLIC ./include
                                           PUBLIC ./raspicam
                                           PUBLIC ../robot-comm/core
                                           PUBLIC ../robot-comm/msgs
                                           PUBLIC ${CEREAL_INCLUDE_FOLDER}
                                           PUBLIC ${APRILTAG_INCLUDE_FOLDER} )
target_link_libraries ( mobile_server raspistill robot-comm ${CMAKE_THREAD_LIBS_INIT} apriltag )
