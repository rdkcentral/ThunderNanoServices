set(PLUGIN_NAME TestUtility)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

message("Setting up ${PLUGIN_NAME}")

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Interfaces REQUIRED)

add_library(${MODULE_NAME} SHARED
        Module.cpp
        TestUtility.cpp
        TestUtilityImp.cpp
        TestUtilityJsonRpc.cpp
        CommandCore/TestCommandController.cpp
        Commands/Malloc.cpp
        Commands/Free.cpp
        Commands/Statm.cpp
        Commands/Crash.cpp
        Commands/CrashNTimes.cpp
)

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_link_libraries(${MODULE_NAME}
    PRIVATE
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Interfaces::${NAMESPACE}Interfaces)

install(TARGETS ${MODULE_NAME}
    DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})
