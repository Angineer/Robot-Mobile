cmake_minimum_required ( VERSION 2.6 )
project ( robot-mobile )

# Need threads
find_package ( Threads )

# Cereal
set ( CEREAL_INCLUDE_FOLDER ../cereal/include CACHE FILEPATH "Cereal include folder" )

# AprilTags
set ( APRILTAG_INCLUDE_FOLDER ${CMAKE_INSTALL_INCLUDEDIR}/apriltag CACHE FILEPATH "AprilTag include folder" )

# Mobile server
add_executable ( mobile_server ./src/mobile_server.cpp
                               ./src/CameraChecker.cpp
                               ./src/MobileManager.cpp )
set_property ( TARGET mobile_server PROPERTY CXX_STANDARD 14 )
target_include_directories ( mobile_server PUBLIC ./include
                                           PUBLIC ../robot-comm/include
                                           PUBLIC ${CEREAL_INCLUDE_FOLDER}
                                           PUBLIC ${APRILTAG_INCLUDE_FOLDER} )
target_link_libraries ( mobile_server robot-comm ${CMAKE_THREAD_LIBS_INIT} apriltag )
