cmake_minimum_required(VERSION 3.15)
project(Pal_Injector VERSION 1.0.0)

# Build Console Executable
add_executable(${PROJECT_NAME} main.cpp)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)

# Link essential Windows libraries
target_link_libraries(${PROJECT_NAME} PRIVATE kernel32 user32 advapi32)
