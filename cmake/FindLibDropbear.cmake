# - Try to find LibDropbear
# Once done this will define
#  LIBDROPBEAR_FOUND - System has LibDropbear
#  LIBDROPBEAR_INCLUDE_DIRS - The LibDropbear include directories
#  LIBDROPBEAR_LIBRARIES - The libraries needed to use LibDropbear
#

message("<FindLibDropbear.cmake>")

find_package(PkgConfig)
pkg_check_modules(LIBDROPBEAR REQUIRED LibDropbear)


if(PC_LIBDROPBEAR_FOUND)
    set(LIBDROPBEAR_INCLUDE_DIRS ${PC_LIBDROPBEAR_INCLUDE_DIRS})
    set(LIBDROPBEAR_LIBRARIES ${PC_LIBDROPBEAR_LIBRARIES})
    set(LIBDROPBEAR_LIBRARY_DIRS ${PC_LIBDROPBEAR_LIBRARY_DIRS})
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBDROPBEAR DEFAULT_MSG LIBDROPBEAR_INCLUDE_DIRS LIBDROPBEAR_LIBRARIES)

mark_as_advanced(
    LIBDROPBEAR_FOUND
    LIBDROPBEAR_INCLUDE_DIRS
    LIBDROPBEAR_LIBRARIES
    LIBDROPBEAR_LIBRARY_DIRS)

message("</FindLibDropbear.cmake>")
