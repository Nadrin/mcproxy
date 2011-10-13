cmake_minimum_required(VERSION 2.8)
add_library(namechange ${HANDLER_MODE} src/namechange.c)
target_link_libraries(namechange mcproxyapi)
