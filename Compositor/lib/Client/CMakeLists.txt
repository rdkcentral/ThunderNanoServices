set(TARGET cmpositorclient)

message("Setup ${TARGET} v${VERSION}...")

set (PUBLIC_HEADERS
     Client.h 
    )

install(
	FILES ${PUBLIC_HEADERS}
        DESTINATION include/${NAMESPACE}/compositor
)

if (WPEFRAMEWORK_PLUGIN_COMPOSITOR_IMPLEMENTATION)
  set(WPEFRAMEWORK_PLUGIN_COMPOSITOR_CLIENT_LIBS libcompositorclient.so)
  add_subdirectory (${WPEFRAMEWORK_PLUGIN_COMPOSITOR_IMPLEMENTATION})
endif ()
