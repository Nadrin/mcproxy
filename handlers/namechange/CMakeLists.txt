CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
ADD_LIBRARY(namechange ${HANDLER_MODE} src/namechange.c)
TARGET_LINK_LIBRARIES(namechange mcproxyapi)
