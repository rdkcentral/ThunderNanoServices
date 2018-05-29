include (list_to_string)

function(show_target_properties target)
    message(STATUS "Properties for ${target}")

    MESSAGE(STATUS "CMAKE_CXX_FLAGS:                    " ${CMAKE_CXX_FLAGS})
    MESSAGE(STATUS "CMAKE_CXX_FLAGS_DEBUG:              " ${CMAKE_CXX_FLAGS_DEBUG} )
    MESSAGE(STATUS "CMAKE_CXX_FLAGS_RELEASE:            " ${CMAKE_CXX_FLAGS_RELEASE} )
    MESSAGE(STATUS "CMAKE_CXX_FLAGS_MINSIZEREL:         " ${CMAKE_CXX_FLAGS_MINSIZEREL} )
    MESSAGE(STATUS "CMAKE_CXX_FLAGS_RELWITHDEBINFO:     " ${CMAKE_CXX_FLAGS_RELWITHDEBINFO} )

    get_target_property(INCLUDES ${target} INCLUDE_DIRECTORIES)
    message(STATUS "Includes:                           " ${INCLUDES})

    get_target_property(DEFINES ${target} COMPILE_DEFINITIONS)
    list_to_string(DEFINES STR)
    message(STATUS "Target defines:                     " ${STR})

    get_target_property(OPTIONS ${target} COMPILE_OPTIONS)
    list_to_string(OPTIONS STR)
    message(STATUS "Target options:                     " ${STR})

    get_target_property(INCLUDES ${target} INCLUDE_DIRECTORIES)
    list_to_string(INCLUDES STR)
    message(STATUS "Target includes:                    " ${STR})

    get_target_property(LIBRARIES ${target} LINK_LIBRARIES)
    list_to_string(LIBRARIES STR)
    message(STATUS "Target link libraries:              " ${STR})

    get_target_property(LIBRARIES ${target} LINK_FLAGS)
    list_to_string(LIBRARIES STR)
    message(STATUS "Target link options:                " ${STR})

    get_target_property(DEFINES_EXPORTS ${target} INTERFACE_COMPILE_DEFINITIONS)
    list_to_string(DEFINES_EXPORTS STR)
    message(STATUS "Target exported defines:            " ${STR})

    get_target_property(OPTIONS_EXPORTS ${target} INTERFACE_COMPILE_OPTIONS)
    list_to_string(OPTIONS_EXPORTS STR)
    message(STATUS "Target exported options:            " ${STR})

    get_target_property(INCLUDES_EXPORTS ${target} INTERFACE_INCLUDE_DIRECTORIES)
    list_to_string(INCLUDES_EXPORTS STR)
    message(STATUS "Target exported includes:           " ${STR})

    get_target_property(LIBRARIES_EXPORTS ${target} INTERFACE_LINK_LIBRARIES)
    list_to_string(LIBRARIES_EXPORTS STR)
    message(STATUS "Target exported link libraries:     " ${STR})

    get_test_property(IMPORT_DEPENDENCIES ${target} IMPORTED_LINK_DEPENDENT_LIBRARIES)
    list_to_string(LIBRARIES_EXPORTS STR)
    message(STATUS "Target imported dependencies:       " ${STR})

    get_test_property(IMPORT_LIBRARIES ${target} IMPORTED_LINK_INTERFACE_LIBRARIES)
    list_to_string(LIBRARIES_EXPORTS STR)
    message(STATUS "Target imported link libraries:     " ${STR})

    get_target_property(ARCHIVE_LOCATION ${target} ARCHIVE_OUTPUT_DIRECTORY)
    message(STATUS "Target static library location:     " ${ARCHIVE_LOCATION})

    get_target_property(LIBRARY_LOCATION ${target} LIBRARY_OUTPUT_DIRECTORY)
    message(STATUS "Target dynamic library location:    " ${LIBRARY_LOCATION})

    get_target_property(RUNTIME_LOCATION ${target} RUNTIME_OUTPUT_DIRECTORY)
    message(STATUS "Target binary location:             " ${RUNTIME_LOCATION})
endfunction()
