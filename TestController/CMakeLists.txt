set(PLUGIN_NAME TestController)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

 message("Setting up ${PLUGIN_NAME}")

 find_package(${NAMESPACE}Plugins REQUIRED)

 add_library(${MODULE_NAME} SHARED
        Module.cpp
        TestController.cpp
        TestControllerImp.cpp
        Core/TestAdministrator.cpp
        Examples/Test1.cpp
        Examples/Test2.cpp
        Examples/Test3.cpp
        Examples/Test4.cpp
)

 set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

 target_link_libraries(${MODULE_NAME}
    PRIVATE
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins)

 install(TARGETS ${MODULE_NAME}
    DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

 write_config(${PLUGIN_NAME})