# Copyright © 2021 Andrea Baretta

# This file is part of FifoIPCBenchmark.

# FifoIPCBenchmark is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# FifoIPCBenchmark is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with FifoIPCBenchmark. If not, see <https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.10)

# set the project name and version
project(fifo-ipc-latency VERSION 1.0)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(fifo-ipc-latency src/test_latency.cpp)
add_executable(fifo-ipc-throughput src/test_throughput.cpp)

message(NOTICE "Cmake build type: ${CMAKE_BUILD_TYPE}")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	message(NOTICE "Debug mode: enabling -O0")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -march=native -DNATIVE")
	target_compile_options(fifo-ipc-latency PUBLIC -O0 -march=native -DNATIVE)
	target_compile_options(fifo-ipc-throughput PUBLIC -O0 -march=native -DNATIVE)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Release")
	message(NOTICE "Release mode: enabling -O3 -march=native -DNDEBUG")
	target_compile_options(fifo-ipc-latency PUBLIC -march=native -DNATIVE)
	target_compile_options(fifo-ipc-throughput PUBLIC -march=native -DNATIVE)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -save-temps -g")

target_include_directories(fifo-ipc-latency PUBLIC
   src/
)

target_include_directories(fifo-ipc-throughput PUBLIC
   src/
)

target_link_libraries(fifo-ipc-latency pthread)
target_link_libraries(fifo-ipc-throughput pthread)
