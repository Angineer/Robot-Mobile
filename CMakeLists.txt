cmake_minimum_required ( VERSION 2.6 )
project ( robot-mobile )

# Cereal dependency
set ( CEREAL_INCLUDE_FOLDER ../cereal/include CACHE FILEPATH "Cereal include folder" )

# Mobile server
add_executable ( mobile_server ./src/mobile_server.cpp
                               ./src/MobileManager.cpp )
set_property ( TARGET mobile_server PROPERTY CXX_STANDARD 11 )
target_include_directories ( mobile_server PUBLIC ./include
                                           PUBLIC ../robot-comm/include
                                           PUBLIC ${CEREAL_INCLUDE_FOLDER} )
target_link_libraries ( mobile_server robot-comm )
