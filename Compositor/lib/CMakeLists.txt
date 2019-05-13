add_subdirectory (${PLUGIN_COMPOSITOR_IMPLEMENTATION})

if(PLUGIN_COMPOSITOR_TEST)
    add_subdirectory (Test)
endif()
