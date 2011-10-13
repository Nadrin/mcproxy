cmake_minimum_required(VERSION 2.8)

# =========== #
# Main Config #
# =========== # 
project(mcproxy)
set(${PROJECT_NAME}_MAJOR_VERSION 0)
set(${PROJECT_NAME}_MINOR_VERSION 2)

# Project Options
option(MCPROXY_USE_CLANG "Compile using clang, if it is available" ON)
option(MCPROXY_USE_NAMED_SEMAPHORES "Use named semaphores regardless of platform" OFF)
option(MCPROXY_HANDLERS_STATIC "Compile handlers as static libraries" OFF)
option(MCPROXY_LIBAPI_STATIC "Compile libproxyapi as a static library" OFF)
set(MCPROXY_EMBEDDED_HANDLER "" CACHE STRING "Embedded handler library for non-modular mode")

# Flags/Includes
add_definitions(-Wall -std=c99 -pedantic-errors)
include_directories(${PROJECT_SOURCE_DIR}/include)

# OS X Compatiblity
if(DEFINED APPLE)
  message(STATUS "Compiling in OS X compatibility mode.")
  set(OSX_SPECIFIC_LIBS iconv)
  set(MCPROXY_USE_NAMED_SEMAPHORES)
endif()

# Options

if(MCPROXY_USE_CLANG)
  message(STATUS "Compiling using clang - yes")
  set(CMAKE_C_COMPILER clang)
endif()

if(MCPROXY_USE_NAMED_SEMAPHORES)
    add_definitions(-DMCPROXY_USE_NAMED_SEMAPHORES)
endif()

# If a handler is being embedded, the handlers must be compiled as static libraries.
if(NOT MCPROXY_EMBED_HANDLER EQUAL "")
  set(MCPROXY_HANDLERS_STATIC ON)
endif()

# If MCPROXY_LIBAPI_STATIC is on, compile a static library, otherwise compile a shared library.
if(MCPROXY_LIBAPI_STATIC)
  set(PROXYAPI_MODE STATIC)
else()
  set(PROXYAPI_MODE SHARED)
endif()

if(MCPROXY_HANDLERS_STATIC)
  set(HANDLERS_MODE STATIC)
else()
  set(HANDLERS_MODE SHARED)
endif()

# ================== #
# Target Definitions #
# ================== # 

## libmcproxyapi
add_library(mcproxyapi ${PROXYAPI_MODE} src/system.c src/network.c src/proto.c src/msgtable.c src/proxy.c src/util.c src/mm.c src/thread.c src/log.c)
target_link_libraries(mcproxyapi dl m pthread ${OSX_SPECIFIC_LIBS})

# mcproxy
add_executable(mcproxy src/main.c src/core.c)
add_dependencies(mcproxy mcproxyapi)
target_link_libraries(mcproxy mcproxyapi ${MXPROXY_EMBEDDED_HANDLER})


# ======== #
# Handlers #
# ======== #

add_subdirectory(handlers/noop)
add_subdirectory(handlers/namechange)
add_subdirectory(handlers/serverlog)