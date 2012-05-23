cmake_minimum_required(VERSION 2.8)
message(STATUS "Configuring handler library - namechange")
add_library(namechange ${HANDLERS_MODE} src/namechange.c)
target_link_libraries(namechange mcproxyapi)
