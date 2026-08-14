# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.0)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(Hello_Pico C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_LIST_DIR}/src/*.c"
        "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/*.S"
)
# flash 尚未移植，先不编进去
list(FILTER APP_SOURCES EXCLUDE REGEX "[\\/]flash[\\/]")
# HAGL HAL 由 hagl_hal 目标编入，避免 GLOB 再编一次
list(FILTER APP_SOURCES EXCLUDE REGEX "[\\/]hagl_hal[\\/]")

add_executable(Hello_Pico Hello_Pico.c ${APP_SOURCES})

pico_set_program_name(Hello_Pico "Hello_Pico")
pico_set_program_version(Hello_Pico "0.1")

# Generate PIO header
pico_generate_pio_header(Hello_Pico ${CMAKE_CURRENT_LIST_DIR}/blink.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(Hello_Pico 0)
pico_enable_stdio_usb(Hello_Pico 1)

# Add the standard library to the build
target_link_libraries(Hello_Pico
        pico_stdlib
        pico_async_context_poll)

set(APP_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/src")
target_include_directories(Hello_Pico PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${APP_INCLUDE_DIRS}
)

# HAGL 官方 CMake 会 link hagl_hal，必须先建好这个 target。
add_library(hagl_hal INTERFACE)
target_sources(hagl_hal INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/src/service/hagl_hal/hagl_hal.c
)
target_include_directories(hagl_hal INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/src/service/hagl_hal
        ${CMAKE_CURRENT_LIST_DIR}/src
)
target_link_libraries(hagl_hal INTERFACE pico_stdlib)
add_subdirectory(third_party/hagl)

# Add any user requested libraries
target_link_libraries(Hello_Pico 
        hardware_spi
        hardware_i2c
        hardware_dma
        hardware_pio
        hardware_interp
        hardware_timer
        hardware_watchdog
        hardware_clocks
        hagl
        )

pico_add_extra_outputs(Hello_Pico)

