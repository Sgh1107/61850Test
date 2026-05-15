# Install script for directory: C:/Users/win1064/Desktop/libiec61850-1.5.3

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files/libiec61850")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xDevelopmentx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libiec61850" TYPE FILE FILES
    "C:/Users/win1064/Desktop/libiec61850-1.5.3/hal/inc/hal_base.h"
    "C:/Users/win1064/Desktop/libiec61850-1.5.3/hal/inc/hal_time.h"
    "C:/Users/win1064/Desktop/libiec61850-1.5.3/hal/inc/hal_thread.h"
    "C:/Users/win1064/Desktop/libiec61850-1.5.3/hal/inc/hal_filesystem.h"
    "C:/Users/win1064/Desktop/libiec61850-1.5.3/hal/inc/hal_ethernet.h"
    "C:/Users/win1064/Desktop/libiec61850-1.5.3/hal/inc/hal_socket.h"
    "C:/Users/win1064/Desktop/libiec61850-1.5.3/hal/inc/tls_config.h"
    "C:/Users/win1064/Desktop/libiec61850-1.5.3/src/common/inc/libiec61850_common_api.h"
    "C:/Users/win1064/Desktop/libiec61850-1.5.3/src/common/inc/linked_list.h"
    "C:/Users/win1064/Desktop/libiec61850-1.5.3/src/iec61850/inc/iec61850_client.h"
    "C:/Users/win1064/Desktop/libiec61850-1.5.3/src/iec61850/inc/iec61850_common.h"
    "C:/Users/win1064/Desktop/libiec61850-1.5.3/src/iec61850/inc/iec61850_server.h"
    "C:/Users/win1064/Desktop/libiec61850-1.5.3/src/iec61850/inc/iec61850_model.h"
    "C:/Users/win1064/Desktop/libiec61850-1.5.3/src/iec61850/inc/iec61850_cdc.h"
    "C:/Users/win1064/Desktop/libiec61850-1.5.3/src/iec61850/inc/iec61850_dynamic_model.h"
    "C:/Users/win1064/Desktop/libiec61850-1.5.3/src/iec61850/inc/iec61850_config_file_parser.h"
    "C:/Users/win1064/Desktop/libiec61850-1.5.3/src/mms/inc/mms_value.h"
    "C:/Users/win1064/Desktop/libiec61850-1.5.3/src/mms/inc/mms_common.h"
    "C:/Users/win1064/Desktop/libiec61850-1.5.3/src/mms/inc/mms_types.h"
    "C:/Users/win1064/Desktop/libiec61850-1.5.3/src/mms/inc/mms_type_spec.h"
    "C:/Users/win1064/Desktop/libiec61850-1.5.3/src/mms/inc/mms_client_connection.h"
    "C:/Users/win1064/Desktop/libiec61850-1.5.3/src/mms/inc/mms_server.h"
    "C:/Users/win1064/Desktop/libiec61850-1.5.3/src/mms/inc/iso_connection_parameters.h"
    "C:/Users/win1064/Desktop/libiec61850-1.5.3/src/logging/logging_api.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xDevelopmentx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libiec61850" TYPE FILE FILES "C:/Users/win1064/Desktop/libiec61850-1.5.3/build_static/config/stack_config.h")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE PROGRAM FILES
    "E:/programming/VS2013/VC/redist/x64/Microsoft.VC120.CRT/msvcp120.dll"
    "E:/programming/VS2013/VC/redist/x64/Microsoft.VC120.CRT/msvcr120.dll"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE DIRECTORY FILES "")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("C:/Users/win1064/Desktop/libiec61850-1.5.3/build_static/hal/cmake_install.cmake")
  include("C:/Users/win1064/Desktop/libiec61850-1.5.3/build_static/src/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "C:/Users/win1064/Desktop/libiec61850-1.5.3/build_static/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
