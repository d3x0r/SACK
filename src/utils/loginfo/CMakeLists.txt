
set(  PROJECT loginfo.module )

set(BASE_SOURCES
${FIRST_GCC_LIBRARY_SOURCE} 
    loginfo.c
${LAST_GCC_LIBRARY_SOURCE}
)

add_library_force_source( ${PROJECT} SHARED ${BASE_SOURCES} )

string( REPLACE "." "_" TARGET_LABEL ${PROJECT} )
SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
	COMPILE_DEFINITIONS  "TARGET_LABEL=${TARGET_LABEL}" 
                  FOLDER utils
)
if( NOT __ANDROID__ )
  SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  SUFFIX ""
                  PREFIX ""
  )
endif( NOT __ANDROID__ )
my_target_link_libraries( ${PROJECT} ${SACK_LIBRARIES} )
install_mode_dest( ${PROJECT} )

