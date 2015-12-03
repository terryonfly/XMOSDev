cmake_minimum_required(VERSION 2.8.4)
project(FUELDev)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.c rokid_fuel.h rokid_fuel.c)
add_executable(FUELDev ${SOURCE_FILES})