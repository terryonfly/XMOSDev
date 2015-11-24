cmake_minimum_required(VERSION 2.8.4)
project(LEDDev)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.c rokid_led.h rokid_led.c)
add_executable(LEDDev ${SOURCE_FILES})