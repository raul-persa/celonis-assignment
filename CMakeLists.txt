cmake_minimum_required(VERSION 3.6)
project(assignment)

find_library(ZMQ_LIB zmq)
find_package (Threads)

find_package( Boost REQUIRED COMPONENTS thread )
include_directories( ${Boost_INCLUDE_DIRS} )

set(CMAKE_CXX_STANDARD 14)

set(SOURCE_FILES Server/main.cpp DataTypes/RequestTypeEnum.hpp Server/ProxyServer.cpp Server/ProxyServer.hpp Server/Worker.cpp Server/Worker.hpp KVManager/KVManager.cpp KVManager/KVManager.hpp Buffer/BufferManager.hpp DataTypes/PageData.hpp Policy/FIFOEvictionPolicy.hpp Policy/SimpleLinuxIoPolicy.hpp)
add_executable(assignment ${SOURCE_FILES})
target_link_libraries(assignment ${ZMQ_LIB} Boost::thread)
target_link_libraries (assignment ${CMAKE_THREAD_LIBS_INIT})