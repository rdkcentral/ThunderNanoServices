list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(PLUGIN_NAME DropbearServer)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(LibDropbear REQUIRED)

if ( NOT LIBDROPBEAR_FOUND )
	message(FATAL_ERROR "Package Dropbear required, but not found!")
endif( NOT LIBDROPBEAR_FOUND) 


add_library(${MODULE_NAME} SHARED
    DropbearServer.cpp
    DropbearServerJsonRpc.cpp
    Module.cpp)

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_iREQUIRED YES)

target_include_directories(${MODULE_NAME}
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/"
	${LIBDROPBEAR_INCLUDE_DIRS})

target_link_libraries(${MODULE_NAME}
    PRIVATE
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
	${LIBDROPBEAR_LIBRARIES} -L"$ENV{STAGING_DIR}/usr/lib" -ldropbear -lz -lutil -lpthread -lcrypt)

install(TARGETS ${MODULE_NAME} 
    DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})
