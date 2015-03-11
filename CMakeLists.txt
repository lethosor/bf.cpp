cmake_minimum_required(VERSION 2.8)

project(bf)
set(CMAKE_BUILD_TYPE Release)

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source builds are not allowed.")
endif()

if(UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wall -fPIC")
endif()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/depends/tclap/include
)

if(NOT IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/depends/tclap/include/tclap)
    message(FATAL_ERROR "tclap not found. Did you forget to run 'git submodule init' and 'git submodule udpate'?")
endif()

add_library(bf SHARED src/bf.cpp)

add_executable(bfi src/bfi.cpp)
target_link_libraries(bfi bf)
