cmake_minimum_required(VERSION 3.5.1)

# automagically detect if we should cross-compile
if(DEFINED ENV{TOOLCHAIN})
    set(CMAKE_C_COMPILER        $ENV{TOOLCHAIN}gcc)
    set(CMAKE_CXX_COMPILER      $ENV{TOOLCHAIN}g++)
    set(CMAKE_AR                "$ENV{TOOLCHAIN}ar" CACHE FILEPATH "CW archiver" FORCE)
endif()

project(hal)

set(LIBHAL_VERSION_MAJOR "2")
set(LIBHAL_VERSION_MINOR "1")
set(LIBHAL_VERSION_PATCH "0")

include(CheckLibraryExists)
check_library_exists(rt clock_gettime "time.h" CONFIG_SYSTEM_HAS_CLOCK_GETTIME)

include(TestBigEndian)
test_big_endian(PLATFORM_IS_BIGENDIAN)

if(WIN32)
    if(CONFIG_FORCE_DISABLE_WPCAP)
        message(STATUS "CONFIG_FORCE_DISABLE_WPCAP=ON -> ethernet HAL layer disabled on Windows")
        set(WITH_WPCAP 0)
    elseif(CONFIG_INCLUDE_GOOSE_SUPPORT OR CONFIG_INCLUDE_SAMPLED_VALUES_SUPPORT)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/winpcap/Lib/wpcap.lib")
            message(STATUS "Found winpcap -> compile ethernet HAL layer (required for GOOSE/SV support)")
            set(WITH_WPCAP 1)
            include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../third_party/winpcap/Include")
        else()
            message(WARNING "GOOSE/SV requested but winpcap not found -> ethernet HAL layer disabled")
            set(WITH_WPCAP 0)
        endif()
    else()
        message(STATUS "GOOSE/SV support disabled -> ethernet HAL layer disabled on Windows")
        set(WITH_WPCAP 0)
    endif()
endif()

include_directories(${CMAKE_CURRENT_LIST_DIR}/inc)

set(libhal_linux_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/socket/linux/socket_linux.c
    ${CMAKE_CURRENT_LIST_DIR}/ethernet/linux/ethernet_linux.c
    ${CMAKE_CURRENT_LIST_DIR}/thread/linux/thread_linux.c
    ${CMAKE_CURRENT_LIST_DIR}/filesystem/linux/file_provider_linux.c
    ${CMAKE_CURRENT_LIST_DIR}/time/unix/time.c
    ${CMAKE_CURRENT_LIST_DIR}/serial/linux/serial_port_linux.c
    ${CMAKE_CURRENT_LIST_DIR}/memory/lib_memory.c
)

set(libhal_windows_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/socket/win32/socket_win32.c
    ${CMAKE_CURRENT_LIST_DIR}/thread/win32/thread_win32.c
    ${CMAKE_CURRENT_LIST_DIR}/filesystem/win32/file_provider_win32.c
    ${CMAKE_CURRENT_LIST_DIR}/time/win32/time.c
    ${CMAKE_CURRENT_LIST_DIR}/serial/win32/serial_port_win32.c
    ${CMAKE_CURRENT_LIST_DIR}/memory/lib_memory.c
)

if(WITH_WPCAP)
    list(APPEND libhal_windows_SRCS
        ${CMAKE_CURRENT_LIST_DIR}/ethernet/win32/ethernet_win32.c
    )
endif()

set(libhal_bsd_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/socket/bsd/socket_bsd.c
    ${CMAKE_CURRENT_LIST_DIR}/ethernet/bsd/ethernet_bsd.c
    ${CMAKE_CURRENT_LIST_DIR}/thread/bsd/thread_bsd.c
    ${CMAKE_CURRENT_LIST_DIR}/filesystem/linux/file_provider_linux.c
    ${CMAKE_CURRENT_LIST_DIR}/time/unix/time.c
    ${CMAKE_CURRENT_LIST_DIR}/memory/lib_memory.c
)

set(libhal_macos_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/socket/bsd/socket_bsd.c
    ${CMAKE_CURRENT_LIST_DIR}/ethernet/bsd/ethernet_bsd.c
    ${CMAKE_CURRENT_LIST_DIR}/thread/macos/thread_macos.c
    ${CMAKE_CURRENT_LIST_DIR}/filesystem/linux/file_provider_linux.c
    ${CMAKE_CURRENT_LIST_DIR}/time/unix/time.c
    ${CMAKE_CURRENT_LIST_DIR}/memory/lib_memory.c
)

if(WIN32)
    set(libhal_SRCS ${libhal_windows_SRCS})

    if(MSVC)
        set_source_files_properties(${libhal_SRCS} PROPERTIES LANGUAGE CXX)
    endif()
elseif(UNIX)
    if(APPLE)
        set(libhal_SRCS ${libhal_macos_SRCS})
    elseif(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
        set(libhal_SRCS ${libhal_bsd_SRCS})
    else()
        set(libhal_SRCS ${libhal_linux_SRCS})
    endif()
endif()

if(WITH_MBEDTLS)
    message(STATUS "Found mbedtls -> can compile HAL with TLS support")
    include_directories(
        ${CMAKE_CURRENT_LIST_DIR}/tls/mbedtls
        ${MBEDTLS_INCLUDE_DIR}
    )

    if(CONFIG_USE_EXTERNAL_MBEDTLS_DYNLIB)
        link_directories(${CONFIG_EXTERNAL_MBEDTLS_DYNLIB_PATH})
    else()
        file(GLOB tls_SRCS ${CMAKE_CURRENT_LIST_DIR}/../third_party/mbedtls/mbedtls-2.28/library/*.c)
    endif()

    add_definitions(-DMBEDTLS_CONFIG_FILE="mbedtls_config.h")

    list(APPEND libhal_SRCS ${CMAKE_CURRENT_LIST_DIR}/tls/mbedtls/tls_mbedtls.c)

    if(MSVC)
        set_source_files_properties(${libhal_SRCS} PROPERTIES LANGUAGE CXX)
    endif()

    list(APPEND libhal_SRCS ${tls_SRCS})
endif()

add_library(hal STATIC ${libhal_SRCS})

if(UNIX)
    if(CONFIG_SYSTEM_HAS_CLOCK_GETTIME)
        target_link_libraries(hal -lpthread -lrt)
    else()
        target_link_libraries(hal -lpthread)
    endif()
endif()

if(CONFIG_USE_EXTERNAL_MBEDTLS_DYNLIB)
    target_link_libraries(hal mbedcrypto mbedx509 mbedtls)
endif()

if(MINGW)
    target_link_libraries(hal ws2_32 iphlpapi)
endif()

if(WITH_WPCAP)
    target_link_libraries(hal
        ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/winpcap/Lib/wpcap.lib
        ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/winpcap/Lib/packet.lib
    )
endif()

set(BINDIR "bin")
set(LIBDIR "lib")
if(UNIX)
    include(GNUInstallDirs)
    set(LIBDIR ${CMAKE_INSTALL_LIBDIR})
    set(BINDIR ${CMAKE_INSTALL_BINDIR})
endif()

install(TARGETS hal
    RUNTIME DESTINATION ${BINDIR} COMPONENT Applications
    ARCHIVE DESTINATION ${LIBDIR} COMPONENT Libraries
    LIBRARY DESTINATION ${LIBDIR} COMPONENT Libraries
)
