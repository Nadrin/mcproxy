cmake_minimum_required(VERSION 2.8)

# =========== #
# Main Config #
# =========== # 
project(mcproxy)
set(${PROJECT_NAME}_MAJOR_VERSION 0)
set(${PROJECT_NAME}_MINOR_VERSION 3)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

# Find pkg-config
find_package(PkgConfig)

# Project Options
option(MCPROXY_HANDLERS_STATIC "Compile handlers as static libraries" OFF)
option(MCPROXY_LIBAPI_STATIC "Compile libproxyapi as a static library" OFF)
set(MCPROXY_EMBEDDED_HANDLER "" CACHE STRING "Embedded handler library for non-modular mode")

# Flags/Includes
add_definitions("-Werror -Wall -Wextra 
                 -Wpointer-arith 
                 -Wcast-qual 
                 -Wstrict-prototypes
                 -Wmissing-prototypes 
                 -Wno-unused-parameter 
                 -std=c99 -fstrict-aliasing -pedantic")
include_directories(${PROJECT_SOURCE_DIR}/include)

# Require SUSv3 (POSIX.2001 + XSI)
add_definitions("-D_XOPEN_SOURCE=600")
add_definitions("-D_BSD_SOURCE")

# OS X Compatibility
if(DEFINED APPLE)
  message(STATUS "Compiling in OS X compatibility mode - yes")
  set(OSX_SPECIFIC_LIBS iconv)
else()
  message(STATUS "Compiling in OS X compatibility mode - no")
endif()

# If a handler is being embedded, the handlers must be compiled as static libraries.
if(NOT MCPROXY_EMBEDDED_HANDLER STREQUAL "")
  if(NOT MCPROXY_HANDLERS_STATIC)
    message(STATUS "Handlers must be compiled statically to be embedded: compiling handlers as static archives")
  endif()
  message(STATUS "Embedding handler - ${MCPROXY_EMBEDDED_HANDLER}")
  set(MCPROXY_HANDLERS_STATIC ON)
else()
  message(STATUS "Embedding handler - (none)")
endif()

# If MCPROXY_LIBAPI_STATIC is on, compile a static library, otherwise compile a shared library.
if(MCPROXY_LIBAPI_STATIC)
  message(STATUS "Compiling libmcproxy as - static")
  set(PROXYAPI_MODE STATIC)
else()
  message(STATUS "Compiling libmcproxy as - shared")
  set(PROXYAPI_MODE SHARED)
endif()

if(MCPROXY_HANDLERS_STATIC)
  message(STATUS "Compiling handlers as - static")
  set(HANDLERS_MODE STATIC)
else()
  message(STATUS "Compiling handlers as - shared")
  set(HANDLERS_MODE SHARED)
endif()

# =========== #
# API LIbrary #
# =========== # 

## libmcproxyapi
add_library(mcproxyapi ${PROXYAPI_MODE} src/system.c src/network.c src/proto.c src/msgtable.c src/proxy.c src/util.c src/mm.c src/thread.c src/log.c)
target_link_libraries(mcproxyapi dl m pthread ${OSX_SPECIFIC_LIBS})

# ======== #
# Handlers #
# ======== #

add_subdirectory(handlers/noop)
add_subdirectory(handlers/namechange)
add_subdirectory(handlers/serverlog)

# ========== #
# Executable #
# ========== #

add_executable(mcproxy src/main.c src/core.c)
add_dependencies(mcproxy mcproxyapi ${MCPROXY_EMBEDDED_HANDLER})
target_link_libraries(mcproxy mcproxyapi ${MCPROXY_EMBEDDED_HANDLER})
