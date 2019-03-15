set(PLUGIN_NAME InjectedBundle)
set(MODULE_NAME WPE${PLUGIN_NAME})

find_package(${NAMESPACE}Plugins REQUIRED)

add_library(${MODULE_NAME} SHARED 
    main.cpp
    WhiteListedOriginDomainsList.cpp
    Utils.cpp
    JavaScriptFunction.cpp
    NotifyWPEFramework.cpp
    Milestone.cpp
    ClassDefinition.cpp
)

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_link_libraries(${MODULE_NAME}
    PRIVATE 
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${GLIB_LIBRARIES}
        ${WPE_WEBKIT_LIBRARIES})

target_include_directories(${MODULE_NAME} 
    PRIVATE 
        ${WPE_WEBKIT_INCLUDE_DIRS}
        ${WPE_WEBKIT_INCLUDE_DIRS}/WPE
        ${GLIB_INCLUDE_DIRS})

install(TARGETS ${MODULE_NAME} 
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${NAMESPACE}/WebKitBrowser
    COMPONENT ${MODULE_NAME})
