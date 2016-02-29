# Revision 1391, 1.2 kB  (checked in by rexbron) 
# - Try to find FUSE
# Once done this will define
#
#  FUSE_FOUND - system has FUSE
#  FUSE_INCLUDE_DIR - the FUSE include directory
#  FUSE_LIBRARIES - Link these to use FUSE
#  FUSE_DEFINITIONS - Compiler switches required for using FUSE
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if ( FUSE_INCLUDE_DIR AND FUSE_LIBRARIES )
   # in cache already
   SET(FUSE_FIND_QUIETLY TRUE)
endif ( FUSE_INCLUDE_DIR AND FUSE_LIBRARIES )

# use pkg-config to get the directories and then use these values
# in the FIND_PATH() and FIND_LIBRARY() calls
if( NOT WIN32 )
  INCLUDE(FindPkgConfig)

  pkg_check_modules(FUSE REQUIRED fuse )

endif( NOT WIN32 )

#FIND_PATH(FUSE_INCLUDE_DIR NAMES fuse.h
#  PATHS
#  ${_FUSEIncDir}
#)
#FIND_LIBRARY(FUSE_LIBRARIES NAMES fuse
#  PATHS
#  ${_FUSELinkDir}
#)

#include(FindPackageHandleStandardArgs)
#FIND_PACKAGE_HANDLE_STANDARD_ARGS(FUSE DEFAULT_MSG FUSE_INCLUDE_DIR FUSE_LIBRARIES )

# show the FUSE_INCLUDE_DIR and FUSE_LIBRARIES variables only in the advanced view
MARK_AS_ADVANCED(FUSE_INCLUDE_DIR FUSE_LIBRARIES )



