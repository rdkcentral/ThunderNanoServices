set(PLUGIN_NAME Spark)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(Spark REQUIRED)
find_package(Freetype2 REQUIRED)

add_library(${MODULE_NAME} SHARED 
    Spark.cpp
    SparkImplementation.cpp
    SparkJsonRpc.cpp
    Module.cpp)

if(PLUGIN_SPARK_NAME)
    target_compile_definitions(${MODULE_NAME} 
        PRIVATE 
            FRIENDLY_NAME="${PLUGIN_SPARK_NAME}")
endif()

if (PLATFORM_LINUX)
    target_compile_definitions(${MODULE_NAME}
        PRIVATE
            RT_PLATFORM_LINUX=ON)
endif()

if (PLATFORM_WPEFRAMEWORK)
    target_compile_definitions(${MODULE_NAME}
        PRIVATE
            PX_PLATFORM_WPEFRAMEWORK=ON)
endif()

if (PLATFORM_WAYLAND_EGL)
    target_compile_definitions(${MODULE_NAME}
        PRIVATE
            PX_PLATFORM_WAYLAND_EGL=ON)
endif()

target_compile_definitions(${MODULE_NAME}
    PRIVATE
        ANIMATION_ROTATE_XYZ=ON
        PX_SCENE_VERSION="dev"
        ENABLE_PERMISSIONS_CHECK=ON
        ENABLE_RT_NODE=ON
        ENABLE_HTTP_CACHE=ON
        RUNINMAIN=ON)

target_include_directories(${MODULE_NAME} 
    PRIVATE 
        ${FREETYPE2_INCLUDE_DIRS}) 

target_link_libraries(${MODULE_NAME} 
    PRIVATE 
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        Spark::Spark)

# Library installation section
install(
    TARGETS ${MODULE_NAME}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGENAME}/plugins)

set(PLUGIN_SPARK_AUTOSTART false CACHE STRING "Automatically start Spark plugin")
set(PLUGIN_SPARK_STARTURL "browser.js" CACHE STRING "Initial URL for Spark plugin")

write_config(${PLUGIN_NAME})
