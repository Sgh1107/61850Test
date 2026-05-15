cmake_minimum_required(VERSION 3.5.1)

project(iec61850_wrapper)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-D_SCL_SECURE_NO_WARNINGS)
endif()

set(LIBIEC61850_ROOT "${CMAKE_CURRENT_LIST_DIR}/3rd/libiec61850")
set(LIBIEC61850_LIB_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")

set(LIBIEC61850_STATIC_LIB "${LIBIEC61850_LIB_DIR}/iec61850.lib")
set(LIBHAL_STATIC_LIB "${LIBIEC61850_LIB_DIR}/hal.lib")

if(NOT EXISTS "${LIBIEC61850_STATIC_LIB}")
    message(FATAL_ERROR "Missing static library: ${LIBIEC61850_STATIC_LIB}")
endif()

if(NOT EXISTS "${LIBHAL_STATIC_LIB}")
    message(FATAL_ERROR "Missing static library: ${LIBHAL_STATIC_LIB}")
endif()

include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/inc
    ${LIBIEC61850_ROOT}
)

add_library(iec61850_wrapper SHARED
    src/iec61850_wrapper.cpp
)

target_compile_definitions(iec61850_wrapper PRIVATE IEC61850_WRAPPER_EXPORTS)

target_link_libraries(iec61850_wrapper
    ${LIBIEC61850_STATIC_LIB}
    ${LIBHAL_STATIC_LIB}
    ws2_32
    iphlpapi
)

add_executable(iec61850_wrapper_test
    src/iec61850_wrapper_test.cpp
    src/test_msg_logger.cpp
)

target_link_libraries(iec61850_wrapper_test
    iec61850_wrapper
)

set_target_properties(iec61850_wrapper PROPERTIES
    OUTPUT_NAME "iec61850_wrapper"
)

set_target_properties(iec61850_wrapper_test PROPERTIES
    OUTPUT_NAME "iec61850_wrapper_test"
)
