### linux/unix unixODBC + freetds connect sqlserver lib for lua 

```
this is a linux / unix lib
i will write code and can be used , please  wait for a meoment 
```

cmake_minimum_required(VERSION 3.6)
project(luasqlserver)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(SOURCE_FILES main.cpp)

LINK_DIRECTORIES(/usr/local/freetds/lib)
add_executable(luasqlserver ${SOURCE_FILES})
TARGET_LINK_LIBRARIES(luasqlserver sybdb)

