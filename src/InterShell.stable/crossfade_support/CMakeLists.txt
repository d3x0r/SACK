cmake_minimum_required(VERSION 2.8)


project( crossfade_vid_mount.isp )

set(BASE_SOURCES
${FIRST_GCC_LIBRARY_SOURCE} 
    crossfade_vid_mount.c
${LAST_GCC_LIBRARY_SOURCE}
)

add_library_force_source(${PROJECT_NAME} SHARED ${BASE_SOURCES} )
string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
	COMPILE_DEFINITIONS  "TARGET_LABEL=${TARGET_LABEL};TARGETNAME=\"${PROJECT_NAME}\"" 
 	PREFIX ""
        SUFFIX ""
        FOLDER utils/crossfade
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} winmm )
install_literal_product( ${PROJECT_NAME} plugins )
