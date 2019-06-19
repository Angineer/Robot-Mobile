cmake_minimum_required ( VERSION 2.6 )
project ( robot-mobile )

# Inventory server
add_executable ( bluetooth_server ./src/bluetooth_server.cpp )
set_property ( TARGET bluetooth_server PROPERTY CXX_STANDARD 11 )
target_include_directories ( bluetooth_server PUBLIC $ENV{HOME}/robot-comm/include )
target_link_libraries ( bluetooth_server robot-comm )
