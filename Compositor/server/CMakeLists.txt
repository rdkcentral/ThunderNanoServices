include_directories(${LIBNXSERVER_INCLUDE_DIRS})

set(BUILD_SHARED_LIBS ON)

find_library(
   WPEFRAMEWORKCORE_LIBRARY
   NAMES WPEFrameworkCore
   HINTS ${PC_WPEFRAMEWORK_LIBDIR}
)

find_library(
   WPEFRAMEWORKPLUGINS_LIBRARY
   NAMES WPEFrameworkPlugins
   HINTS ${PC_WPEFRAMEWORK_LIBDIR}
)

find_library(
   WPEFRAMEWORKPROTOCOLS_LIBRARY
   NAMES WPEFrameworkProtocols
   HINTS ${PC_WPEFRAMEWORK_LIBDIR}
)

# Build platformserver
add_executable(platformserver platformserver.cpp)
target_link_libraries(platformserver
                      ${WPEFRAMEWORK_CORE_LIBRARIES}
                      ${WPEFRAMEWORKPLUGINS_LIBRARY} 
                      ${WPEFRAMEWORKCORE_LIBRARY} 
                      ${WPEFRAMEWORKPROTOCOLS_LIBRARY}
                     )

install(TARGETS platformserver DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
