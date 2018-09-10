cmake_minimum_required (VERSION 2.6)
project (cheetah)
# set version number
set (VERSION_MAJOR 0)
set (VERSION_MINOR 1)
set (PORT_NUMBER 1991)
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
# add config header
configure_file (
  "${PROJECT_SOURCE_DIR}/config.h.in"
  "${PROJECT_BINARY_DIR}/config.h"
)
# include
include_directories("${PROJECT_BINARY_DIR}")
add_executable(cheetah main.cpp server.cpp socket.cpp scheduler.cpp server.h socket.h scheduler.h)
