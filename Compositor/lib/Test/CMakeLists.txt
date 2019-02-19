find_package(GLESv2 REQUIRED)
find_package(EGL REQUIRED)
find_package(PNG REQUIRED)

add_executable(CompositorTest Test.cpp)

set_target_properties(CompositorTest PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        )     

target_link_libraries(CompositorTest
    PRIVATE
        compositorclient
        PNG::PNG       
         ${EGL_LIBRARIES}
         ${GLESV2_LIBRARIES}
        )

target_include_directories(CompositorTest
    PRIVATE
      ${EGL_INCLUDE_DIRS}
      ${GLESV2_INCLUDE_DIRS})

target_compile_definitions(CompositorTest
    PRIVATE
        ${EGL_DEFINITIONS}
        ${GLESV2_DEFINITIONS})

install(TARGETS CompositorTest DESTINATION bin)