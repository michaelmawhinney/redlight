cmake_minimum_required(VERSION 3.20)
project(RedLight VERSION 0.4.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(RedLight WIN32 src/main.cpp resources/app.rc)

target_link_libraries(RedLight PRIVATE user32 gdi32 Shell32)

set_target_properties(RedLight PROPERTIES
    LINK_FLAGS "/SUBSYSTEM:WINDOWS"
)
