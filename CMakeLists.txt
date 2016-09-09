cmake_minimum_required(VERSION 3.6)
project(luasqlserver)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(SOURCE_FILES xnux_luasqlserver/tdssqlserver.h)
set(SOURCE_FILES xnux_luasqlserver/tdssqlserver.cpp)

LINK_DIRECTORIES(/usr/local/freetds/lib)
add_executable(luasqlserver ${SOURCE_FILES})
TARGET_LINK_LIBRARIES(luasqlserver sybdb lua)