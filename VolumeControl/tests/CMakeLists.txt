set(TARGET VolumeControlIntegrationTests)

find_package(GTest REQUIRED)
find_package(thunder_test_support REQUIRED CONFIG)

add_executable(${TARGET}
    VolumeControlTest.cpp
    Module.cpp
)

target_compile_definitions(${TARGET}
    PRIVATE
        MODULE_NAME=VolumeControlIntegrationTests
        VOLUMECONTROL_TEST_CALLSIGN="VolumeControl"
        VOLUMECONTROL_TEST_CLASSNAME="VolumeControl"
        VOLUMECONTROL_TEST_LOCATOR="${CMAKE_SHARED_LIBRARY_PREFIX}${MODULE_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}"
        VOLUMECONTROL_TEST_PLUGIN_PATH="${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/${STORAGE_DIRECTORY}/plugins"
)

if(BUILD_REFERENCE)
    target_compile_definitions(${TARGET} PRIVATE BUILD_REFERENCE=${BUILD_REFERENCE})
endif()

target_link_libraries(${TARGET}
    PRIVATE
        thunder_test_support::thunder_test_support
        GTest::gtest
        GTest::gtest_main
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
)

add_test(NAME ${TARGET} COMMAND ${TARGET})