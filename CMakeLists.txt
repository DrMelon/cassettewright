cmake_minimum_required(VERSION 3.13)
project(cassettewright)

set(SOURCE_FILES 
        src/cassettewright.c
        argparse/argparse.c) 

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(cassettewright ${SOURCE_FILES})
