set(TARGET ${NAMESPACE}SimpleCOMRPCInterface)

add_library(${TARGET} SHARED
        ProxyStub_SimpleCOMRPCInterface.cpp
        Module.cpp
        )


target_link_libraries(${TARGET}
		PRIVATE
		  ${NAMESPACE}WebSocket::${NAMESPACE}WebSocket
		  ${NAMESPACE}COM::${NAMESPACE}COM
            )

target_link_libraries(${TARGET}
        PRIVATE
          CompileSettingsDebug::CompileSettingsDebug
        )

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        VERSION ${VERSION}
        SOVERSION ${VERSION_MAJOR}
        )

install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib/${NAMESPACE_LIB}/proxystubs COMPONENT libs      # static lib
        LIBRARY DESTINATION lib/${NAMESPACE_LIB}/proxystubs COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin/${NAMESPACE_LIB}/proxystubs COMPONENT libs    # for mac
        PUBLIC_HEADER DESTINATION include/${NAMESPACE}/proxystubs COMPONENT devel   # headers for mac (note the different component -> different package)
        INCLUDES DESTINATION include/${NAMESPACE}/proxystubs      # headers
)

InstallCMakeConfig(TARGETS ${TARGET})
