cmake_minimum_required(VERSION 3.12)

set(PICO_SDK_PATH ~/pico-sdk)
set(PICO_BOARD seeed_xiao_rp2040)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(xiao-rp2040-audio-player C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.1")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.1 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

pico_sdk_init()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )

add_executable(xiao-rp2040-audio-player
  main.c
  )

target_link_libraries(xiao-rp2040-audio-player pico_stdlib hardware_dma hardware_flash hardware_pwm)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(xiao-rp2040-audio-player)
