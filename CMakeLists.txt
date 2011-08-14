CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(MCPROXY)
SET(CMAKE_C_FLAGS "-g -O1 -fPIC -pipe -Wall")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
INCLUDE_DIRECTORIES(${MCPROXY_SOURCE_DIR}/include)

ADD_EXECUTABLE(mcproxy src/main.c src/core.c src/network.c src/msgtable.c)
ADD_LIBRARY(proxyapi STATIC src/proto.c src/proxy.c src/util.c src/mm.c src/log.c)

IF(DEFINED APPLE)
  SET(OSX_LIBRARIES iconv)
ENDIF(DEFINED APPLE)
TARGET_LINK_LIBRARIES(mcproxy proxyapi dl m pthread ${OSX_LIBRARIES})

ADD_SUBDIRECTORY(handlers/noop)
ADD_SUBDIRECTORY(handlers/serverlog)
