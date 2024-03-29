# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.12)

set(PICO_CXX_ENABLE_EXCEPTIONS 1)

#include build functions from Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Set name of project (as PROJECT_NAME) and C/C++ Standards
project(project_name C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
  add_compile_definitions(IS_DEBUG_MODE="1")
endif()

# Creates a pico-sdk subdirectory in our project for the libraries
pico_sdk_init()

# point out the CMake, where to find the executable source file
add_executable(${PROJECT_NAME}
  src/main.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_compile_options(${PROJECT_NAME} PUBLIC "-Wno-psabi")

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# create map/bin/hex/uf2 files.
pico_add_extra_outputs(${PROJECT_NAME})

pico_set_program_url(${PROJECT_NAME} "https://github.com/RaresAil/rpi-pico-esp-cpp")

# Pull in our pico_stdlib which pulls in commonly used features (gpio, timer-delay etc)
target_link_libraries(${PROJECT_NAME}
  pico_stdlib
  pico_multicore
  hardware_rtc
  hardware_i2c
)
