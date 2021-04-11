cmake_minimum_required(VERSION 3.10)

# set the project name and version
project(fifo-ipc-latency VERSION 1.0)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(fifo-ipc-latency src/test_latency.cpp)

message(NOTICE "Cmake build type: ${CMAKE_BUILD_TYPE}")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	message(NOTICE "Debug mode: enabling -O0")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Release")
	message(NOTICE "Release mode: enabling -O3 -march=native -DNDEBUG")
	target_compile_options(fifo-ipc-latency PUBLIC -march=native -DNATIVE)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -save-temps -g")

target_include_directories(fifo-ipc-latency PUBLIC
   src/
)

target_link_libraries(fifo-ipc-latency pthread)
