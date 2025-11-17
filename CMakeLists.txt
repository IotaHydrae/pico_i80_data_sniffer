cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -Wl,--print-memory-usage")
SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -Wl,--print-memory-usage")

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

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 1)

pico_add_extra_outputs(${PROJECT_NAME})

add_custom_target(
	print-memory-usage ALL
	COMMAND arm-none-eabi-size -G ${CMAKE_PROJECT_NAME}.elf
	DEPENDS ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.elf
	COMMENT "Print target size info"
	DEPENDS ${PROJECT_NAME}
)

add_custom_target(
	copy-gdbinit ALL
	COMMAND cp ${CMAKE_CURRENT_LIST_DIR}/.gdbinit ${CMAKE_BINARY_DIR}/
	DEPENDS ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.elf
	COMMENT "Copy .gdbinit to build directory"
	DEPENDS ${PROJECT_NAME}
)

# add a firmware flash target
if(${PICO_BOARD} STREQUAL "pico" OR ${PICO_PLATFORM} STREQUAL "rp2040")
	add_custom_target(
		flash
		COMMAND pyocd load --target rp2040 ${CMAKE_PROJECT_NAME}.elf
		DEPENDS ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.elf
		COMMENT "Flashing firmware using pyocd..."
		DEPENDS ${PROJECT_NAME}
	)
	add_custom_target(
		reset
		COMMAND pyocd reset --target rp2040
		COMMENT "Resetting target using pyocd..."
	)
elseif(${PICO_BOARD} STREQUAL  "pico2" OR ${PICO_PLATFORM} STREQUAL  "rp2350")
	add_custom_target(
		flash
		COMMAND openocd -f interface/cmsis-dap.cfg -c "adapter speed 10000"
		-f target/rp2350.cfg -s tcl -c "program ${CMAKE_PROJECT_NAME}.elf verify reset exit"
		DEPENDS ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.elf
		COMMENT "Flashing firmware using CMSIS-DAP Debugger..."
		DEPENDS ${PROJECT_NAME}
	)
endif()
