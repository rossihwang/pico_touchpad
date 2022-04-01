cmake_minimum_required(VERSION 3.12)
include(pico_sdk_import.cmake)

project(gt911_i2c C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)

if(PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
  message(
    FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0(or later) required. Your version is ${PICO_SDK_VERSION_STRING}"
  )
endif()

# Initialize the SDK
pico_sdk_init()

# add_compile_options( -Wall -Wno-format # int != int32_t as far as the compiler
# is concerned because gcc # has int32_t as long int -Wno-unused-function # we
# have some for the docs that aren't called -Wno-maybe-uninitialized)
include_directories(include)

add_executable(gt911_interrupt_test src/dma_uart.cpp src/gt911_i2c.cpp src/gt911_interrupt_test.cpp)
target_link_libraries(gt911_interrupt_test pico_stdlib hardware_gpio hardware_clocks
                      hardware_dma hardware_uart hardware_irq hardware_i2c)

add_executable(gt911_poll_test src/dma_uart.cpp src/gt911_i2c.cpp src/gt911_poll_test.cpp)
target_link_libraries(gt911_poll_test pico_stdlib hardware_gpio hardware_clocks hardware_dma hardware_uart hardware_irq hardware_i2c)

add_executable(mouse_test src/mouse_test.cpp src/dma_uart.cpp src/gt911_i2c.cpp src/usb/usb_descriptors.c)
target_include_directories(mouse_test PRIVATE include/usb)
target_link_libraries(mouse_test pico_stdlib tinyusb_device tinyusb_board hardware_gpio hardware_clocks
hardware_dma hardware_uart hardware_irq hardware_i2c)

pico_add_extra_outputs(gt911_interrupt_test)
pico_add_extra_outputs(mouse_test)