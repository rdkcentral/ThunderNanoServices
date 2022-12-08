project(Buffer)
cmake_minimum_required(VERSION 3.15)

if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/${COMPOSITOR_BUFFER_IMPLEMENTATION}")
    message(FATAL_ERROR "Missing a buffer implementation for ${COMPOSITOR_BUFFER_IMPLEMENTATION}")
endif()

add_subdirectory("${COMPOSITOR_BUFFER_IMPLEMENTATION}")

add_subdirectory(test)