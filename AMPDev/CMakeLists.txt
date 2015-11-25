cmake_minimum_required(VERSION 2.8.4)
project(AMPDev)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.c)
add_executable(AMPDev ${SOURCE_FILES} rokid_amp.h rokid_amp.c)