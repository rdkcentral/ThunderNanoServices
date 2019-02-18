add_subdirectory (${PLUGIN_COMPOSITOR_IMPLEMENTATION})
add_subdirectory (Client)

if(PLUGIN_COMPOSITOR_TEST)
    add_subdirectory (Test)
endif()