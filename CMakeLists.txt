# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

set(PICO_BOARD pico_w CACHE STRING "Board type")

include(pico_sdk_import.cmake)

project(Saude C CXX ASM)

pico_sdk_init()

# Arquivos do projeto (ADICIONEI lib/mpu6050.c)
add_executable(Saude
    Saude.c
    lib/mpu6050.c
    lib/ds18b20.c
)

pico_set_program_name(Saude "Saude")
pico_set_program_version(Saude "0.1")


# stdio
pico_enable_stdio_uart(Saude 1)
pico_enable_stdio_usb(Saude 1)

# Includes
target_include_directories(Saude PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/lib
    ${CMAKE_CURRENT_LIST_DIR}/lib/headers
)

# Bibliotecas
target_link_libraries(Saude
    pico_stdlib
    hardware_i2c
    hardware_gpio
    hardware_adc
    hardware_pwm
    pico_time
    pico_cyw43_arch_lwip_threadsafe_background
)

pico_add_extra_outputs(Saude)