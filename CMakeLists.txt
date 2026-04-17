cmake_minimum_required(VERSION 3.20)
project(FilestashCfapi LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT WIN32)
    message(FATAL_ERROR "This project targets Windows only")
endif()

add_definitions(-DWINVER=0x0A00 -D_WIN32_WINNT=0x0A00)
add_definitions(-DUNICODE -D_UNICODE)

add_executable(filestash-cfapi
    src/main.cpp
    src/CloudProvider.cpp
    src/CloudProvider.h
    src/StateMachine.cpp
    src/StateMachine.h
    src/StateStore.cpp
    src/StateStore.h
    src/FilestashClient.cpp
    src/FilestashClient.h
)

target_include_directories(filestash-cfapi
    PRIVATE
    src
)

target_link_libraries(filestash-cfapi
    PRIVATE
    cldapi.lib
    ole32.lib
    winhttp.lib
)
