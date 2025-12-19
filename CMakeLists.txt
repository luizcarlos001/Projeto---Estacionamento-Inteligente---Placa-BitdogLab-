# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
# ==========================================================
# Projeto Raspberry Pi Pico W - VL53L0X + SSD1306 Modular
# ==========================================================

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ----------------------------------------------------------
# Inicialização do ambiente Pico SDK
# ----------------------------------------------------------
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()

set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()

set(PICO_BOARD pico_w CACHE STRING "Board type")

include(pico_sdk_import.cmake)
project(displayfuncionando C CXX ASM)
pico_sdk_init()

# ----------------------------------------------------------
# Fontes principais
# ----------------------------------------------------------
add_executable(displayfuncionando
    main.c
    src/display.c
    src/sensor.c
    inc/ssd1306_i2c.c
    inc/vl53l0x.c
    src/sensor_ultrasonico.c
    src/wifi_ap.c
    src/http_server.c

    dhcpserver/dhcpserver.c  # Incluindo DHCP
    dnsserver/dnsserver.c    # Incluindo DNS

    src/parking_state.c
)

# ----------------------------------------------------------
# Nome e versão
# ----------------------------------------------------------
pico_set_program_name(displayfuncionando "displayfuncionando")
pico_set_program_version(displayfuncionando "0.1")

# ----------------------------------------------------------
# STDIO
# ----------------------------------------------------------
pico_enable_stdio_usb(displayfuncionando 1)
pico_enable_stdio_uart(displayfuncionando 0)

# ----------------------------------------------------------
# Bibliotecas
# ----------------------------------------------------------
target_link_libraries(displayfuncionando
    pico_stdlib
    pico_cyw43_arch_lwip_poll
    hardware_i2c
    hardware_pwm
)

# ----------------------------------------------------------
# Includes
# ----------------------------------------------------------
target_include_directories(displayfuncionando PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/inc
    ${CMAKE_CURRENT_LIST_DIR}/dhcpserver  # Incluindo os diretórios
    ${CMAKE_CURRENT_LIST_DIR}/dnsserver
)

# ----------------------------------------------------------
# Saídas (UF2, BIN, ELF)
# ----------------------------------------------------------
pico_add_extra_outputs(displayfuncionando)
