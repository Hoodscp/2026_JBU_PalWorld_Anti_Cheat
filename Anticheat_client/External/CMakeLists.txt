cmake_minimum_required(VERSION 3.20)
project(AntiCheatExternalSuite CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 통합 러너: 외부 감시 탐지 모듈 4종을 단일 큐/스케줄러로 묶은 실행파일.
add_executable(ac_external_monitor
    Runner/main.cpp
    Module_Injection_Scanner/InjectionScannerModule.cpp
    Module_Hook_Scanner/HookScannerModule.cpp
    Module_Integrity_Scanner/IntegrityScannerModule.cpp
    # 기존 프로세스 감시 모듈(다른 디렉터리)도 통합.
    ${CMAKE_CURRENT_SOURCE_DIR}/../Module_Process_Watcher/ProcessMonitorModule.cpp
)

target_include_directories(ac_external_monitor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../Common                  # AcCommon.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../Module_Process_Watcher  # IDetectionModule.h, ProcessMonitorModule.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Module_Injection_Scanner
    ${CMAKE_CURRENT_SOURCE_DIR}/Module_Hook_Scanner
    ${CMAKE_CURRENT_SOURCE_DIR}/Module_Integrity_Scanner
)

if(MSVC)
    target_compile_options(ac_external_monitor PRIVATE /W4 /utf-8 /permissive-)
    # Win32 API 의 W 버전을 기본으로.
    target_compile_definitions(ac_external_monitor PRIVATE UNICODE _UNICODE)
endif()

# wintrust: Authenticode 서명 검증(InjectionScanner)
# bcrypt  : SHA-256(ProcessMonitor)
target_link_libraries(ac_external_monitor PRIVATE wintrust bcrypt)
