cmake_minimum_required(VERSION 2.8)



add_subdirectory( concore )


project( psicon.nex )


set(SOURCES 
${FIRST_GCC_LIBRARY_SOURCE}
	psiconlink psicon 
${LAST_GCC_LIBRARY_SOURCE} )


add_library( ${PROJECT_NAME} SHARED ${SOURCES} )

string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_DEFINITIONS "TARGETNAME=\"${PROJECT_NAME}\""
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} -DPSICON -DGRAPHCON -D__DEKWARE_PLUGIN__"
                  PREFIX ""
                  SUFFIX ""
                  FOLDER plugins/devices
)

target_link_libraries( ${PROJECT_NAME}  ${SACK_LIBRARIES} concore )

INSTALL( TARGETS ${PROJECT_NAME} 
	RUNTIME DESTINATION bin/plugins
	LIBRARY DESTINATION bin/plugins
)


if( WIN32 )

project( wincon.nex )

set(SOURCES 
${FIRST_GCC_LIBRARY_SOURCE}
	psiconlink wincon regaccess
${LAST_GCC_LIBRARY_SOURCE} )


add_library( ${PROJECT_NAME} SHARED ${SOURCES} )

string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_DEFINITIONS "TARGETNAME=\"${PROJECT_NAME}\""
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} -DVIDLIB -DWINCON -DGRAPHCON -D__DEKWARE_PLUGIN__"
                  PREFIX ""
                  SUFFIX ""
                  FOLDER plugins/devices
)

target_link_libraries( ${PROJECT_NAME}  ${SACK_LIBRARIES} concore )

INSTALL( TARGETS ${PROJECT_NAME} 
	RUNTIME DESTINATION bin/plugins
	LIBRARY DESTINATION bin/plugins
)

endif( WIN32 )
