cmake_minimum_required (VERSION 2.6)
project (Robot)

file(GLOB SOURCE "./src/*.cpp")

add_executable(run ${SOURCE})

set_property(TARGET run PROPERTY CXX_STANDARD 11)
