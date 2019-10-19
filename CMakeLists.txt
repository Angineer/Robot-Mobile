cmake_minimum_required ( VERSION 2.6 )
project ( robot-mobile )

# Inventory server
add_executable ( mobile_server ./src/mobile_server.cpp
                               ./src/MobileManager.cpp )
set_property ( TARGET mobile_server PROPERTY CXX_STANDARD 11 )
target_include_directories ( mobile_server PUBLIC ./include
                                           PUBLIC $ENV{HOME}/robot-comm/include )
target_link_libraries ( mobile_server robot-comm )
