# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2020 Metrological
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

find_package(Python3 COMPONENTS Interpreter)

if(NOT Python3_Interpreter_FOUND)
message(FATAL_ERROR "python3 was not found...")
endif()

file(GLOB SHADER_FILES *.glsl)

set(OUTPUT_SHADER_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/shaders")

add_custom_target(generated_shaders)

foreach(SHADER ${SHADER_FILES})
    message(STATUS "Converting ${SHADER} to a source file: ${OUTPUT_SHADER_DIR}/${TARGET}-shader.h")
    get_filename_component(TARGET ${SHADER} NAME_WE)

    add_custom_command (
        TARGET generated_shaders
        PRE_BUILD
        VERBATIM
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        COMMENT "Generating shader: ${TARGET}-shader.h"
        COMMAND mkdir -p "${OUTPUT_SHADER_DIR}"
        COMMAND ${Python3_EXECUTABLE} convert.py --input ${SHADER} --output ${OUTPUT_SHADER_DIR}/${TARGET}-shader.h --symbol ${TARGET}_shader 
    )
endforeach(SHADER)

add_library(GL_SHADERS INTERFACE)
add_dependencies(GL_SHADERS generated_shaders)
target_include_directories(GL_SHADERS INTERFACE ${OUTPUT_SHADER_DIR})

add_library(gl::shaders ALIAS GL_SHADERS)
