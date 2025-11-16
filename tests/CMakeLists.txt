add_executable(pio_clocked_input clocked_input.c)
pico_generate_pio_header(pio_clocked_input ${CMAKE_CURRENT_LIST_DIR}/clocked_input.pio)
target_link_libraries(pio_clocked_input PRIVATE
	pico_stdlib
	hardware_pio
	hardware_spi
	hardware_dma
)
