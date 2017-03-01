cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "-std=c++1y -pthread -g")

set(HDR_LIST ./src/logger.h)

set(SRC_LIST ./src/logger.cpp
             ./src/main.cpp)

add_executable(app ${SRC_LIST} ${HDR_LIST})
