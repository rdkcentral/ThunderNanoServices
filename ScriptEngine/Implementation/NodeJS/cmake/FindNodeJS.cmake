find_path (NODEJS_INCLUDE_DIRS NAME node.h PATHS "usr/include/" PATH_SUFFIXES "node")
find_library(NODEJS_LIBRARIES NAME libnode.so.115 PATH_SUFFIXES lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NodeJS DEFAULT_MSG NODEJS_INCLUDE_DIRS NODEJS_LIBRARIES)
mark_as_advanced(NODEJS_INCLUDE_DIRS NODEJS_LIBRARIES)

