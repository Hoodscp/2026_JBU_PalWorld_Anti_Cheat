cmake_minimum_required(VERSION 3.15)
project(AntiCheatProcessMonitor CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ac_procmon
    main.cpp
    ProcessMonitorModule.cpp
)

if (MSVC)
    target_compile_options(ac_procmon PRIVATE /W4 /utf-8 /permissive-)
    # UNICODE 정의: Win32 API의 W 버전을 기본으로 사용.
    target_compile_definitions(ac_procmon PRIVATE UNICODE _UNICODE)
endif()

# SHA-256(CNG)용 bcrypt. ProcessMonitorModule.cpp 에 #pragma comment 도 있음.
target_link_libraries(ac_procmon PRIVATE bcrypt)
