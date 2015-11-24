cmake_minimum_required(VERSION 2.8.4)
project(XMOSDev)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.c rokid_xmos.c rokid_xmos.h uart.c uart.h demo.c)
add_executable(XMOSDev ${SOURCE_FILES})
