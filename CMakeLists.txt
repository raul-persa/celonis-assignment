cmake_minimum_required(VERSION 3.6)
project(assignment)

FIND_LIBRARY(ZMQ_LIB zmq)
find_package (Threads)

set(CMAKE_CXX_STANDARD 14)

set(SOURCE_FILES Server/main.cpp DataTypes/RequestTypeEnum.hpp Server/ProxyServer.cpp Server/ProxyServer.hpp Server/Worker.cpp Server/Worker.hpp KVManager/KVManager.cpp KVManager/KVManager.hpp)
add_executable(assignment ${SOURCE_FILES})
TARGET_LINK_LIBRARIES(assignment ${ZMQ_LIB})
target_link_libraries (assignment ${CMAKE_THREAD_LIBS_INIT})