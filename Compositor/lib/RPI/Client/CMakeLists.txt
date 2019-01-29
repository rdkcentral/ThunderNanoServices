set(TARGET compositorclient)

message("Setup ${TARGET} v${VERSION}...")

find_package(${NAMESPACE}Core REQUIRED)
find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}VirtualInput REQUIRED)

add_library(${TARGET} SHARED Implementation.cpp)

target_link_libraries(${TARGET}
    PUBLIC
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}VirtualInput::${NAMESPACE}VirtualInput
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
)

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        VERSION ${VERSION}
        SOVERSION ${VERSION_MAJOR}
)

install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
)

InstallPackageConfig(
        TARGETS ${TARGET}
        DESCRIPTION "A compositor implementation wrapper for RPI and key handling.")

InstallCMakeConfig(TARGETS ${TARGET})
