cmake_minimum_required(VERSION 2.8)
message(STATUS "Configuring handler library - serverlog")

pkg_check_modules(GLIB REQUIRED glib-2.0)
include_directories(${GLIB_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_library(serverlog ${HANDLERS_MODE} src/init.c src/gamestate.c src/login.c src/track.c src/transaction.c src/inventory.c src/action.c src/chat.c src/settings.c)
target_link_libraries(serverlog mcproxyapi m pthread ${GLIB_LIBRARIES})
