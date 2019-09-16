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
