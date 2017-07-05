cmake_minimum_required (VERSION 2.6)
project (Robot)

# file(GLOB SOURCE "./src/*.cpp")

set(CEREAL_INCLUDE_FOLDER "../cereal/include" CACHE FILEPATH "Cereal include folder")
include_directories(${CEREAL_INCLUDE_FOLDER})

add_executable(inventory_server ./src/communication.cpp
                                ./src/inventory_manager.cpp 
                                ./src/inventory_server.cpp)
add_executable(test_client ./src/communication.cpp
                            ./src/test_client.cpp)

set_property(TARGET inventory_server PROPERTY CXX_STANDARD 11)
set_property(TARGET test_client PROPERTY CXX_STANDARD 11)
