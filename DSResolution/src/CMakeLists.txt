# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2020 RDK Management
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

set(DSRESOLUTION_TEST_LIB_ARTIFACT
    ds-hal
    )

set(DSHAL_SOURCES
    DSHAL.cpp
)

set(DSHAL_INPUT
    ${DSHAL_SOURCES}
)

set(DSHAL_CONFIG
    rdk_devicesettings_hal.conf
    DummyHALConfig.sh
)

set (DSHAL_INCLUDE
    dsHost.h
    dsTypes.h
    dsVideoPort.h
)

add_library(${DSRESOLUTION_TEST_LIB_ARTIFACT} SHARED ${DSHAL_INPUT})
target_include_directories(${DSRESOLUTION_TEST_LIB_ARTIFACT} PUBLIC ${CMAKE_CURRENT_LIST_DIR})

install(
    TARGETS ${DSRESOLUTION_TEST_LIB_ARTIFACT}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    COMPONENT ${DSRESOLUTION_TEST_LIB_ARTIFACT}
)

install(
    FILES ${DSHAL_INCLUDE}
    DESTINATION include/
)

install(
    FILES ${DSHAL_CONFIG}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/local/etc/WPEFramework/
)
