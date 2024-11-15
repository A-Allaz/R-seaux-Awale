# Specify the minimum version of CMake required
cmake_minimum_required(VERSION 3.10)

# Set the project name
project(AwaleGame VERSION 1.0)

# Set the C standard to C11
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

## Add the source directory
#add_subdirectory(src)

# Define the executable target for the main application
add_executable(awale_server src/server.c cJSON/cJSON.c
        src/game.h
        src/request.h)
add_executable(awale_client src/client.c cJSON/cJSON.c
        src/request.h)

# Include the directory containing headers
target_include_directories(awale_server PRIVATE src cJSON)
target_include_directories(awale_client PRIVATE src cJSON)