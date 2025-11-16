cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(pico_sdk_import.cmake)

set(PROJECT_NAME pico_i80_data_sniffer)
project(${PROJECT_NAME} C CXX ASM)

pico_sdk_init()

add_executable(${PROJECT_NAME})
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/i80_input.pio)
target_sources(${PROJECT_NAME} PRIVATE main.c analyzer.c)
target_link_libraries(${PROJECT_NAME} PRIVATE
	pico_stdlib
	hardware_pio
	hardware_dma
)

add_subdirectory(tests)

# pico_enable_stdio_usb(${PROJECT_NAME} 1)
# pico_enable_stdio_uart(${PROJECT_NAME} 1)

# pico_add_extra_outputs(${PROJECT_NAME})
