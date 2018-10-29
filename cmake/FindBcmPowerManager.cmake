find_path(NXSERVER_INCLUDE_DIR nexus_platform_client.h
        PATHS usr/include/
        PATH_SUFFIXES refsw
        )

find_library(PM_LIBRARY NAMES libpmlib.a)
set (BCM_PM_LIBRARIES ${PM_LIBRARY} CACHE PATH "path to pmlib library")
set (BCM_PM_INCLUDE_DIRS ${NXSERVER_INCLUDE_DIR} CACHE PATH "Path to header")
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(BCM_PM DEFAULT_MSG BCM_PM_INCLUDE_DIRS BCM_PM_LIBRARIES)

mark_as_advanced(BCM_PM_INCLUDE_DIRS BCM_PM_LIBRARIES)
