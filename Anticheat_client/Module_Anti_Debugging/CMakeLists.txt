cmake_minimum_required(VERSION 3.20)

project(Module_Anti_Debugging LANGUAGES CXX)

add_executable(anti_debug_monitor
    src/anti_debug_monitor.cpp
)

target_compile_features(anti_debug_monitor PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(anti_debug_monitor PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(anti_debug_monitor PRIVATE -Wall -Wextra -Wpedantic)
endif()
