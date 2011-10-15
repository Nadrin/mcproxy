cmake_minimum_required(VERSION 2.8)
add_library(noop ${HANDLERS_MODE} src/noop.c)
target_link_libraries(noop mcproxyapi)
