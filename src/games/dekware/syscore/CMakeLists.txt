

cmake_minimum_required(VERSION 2.8)

project( dekware.core )


set(SOURCES 
${FIRST_GCC_LIBRARY_SOURCE}
nexus commands sentcmds entcmds  datacmds varcmds syscmds input plugins
interface 
${LAST_GCC_LIBRARY_SOURCE} )


add_library( ${PROJECT_NAME} SHARED ${SOURCES} )

string( REGEX REPLACE "(.*)\\.(.*)" "\\1_\\2" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  DEFINE_SYMBOL CORE_SOURCE
                  PREFIX ""
                  SUFFIX ""
                  COMPILE_DEFINITIONS "TARGETNAME=\"${PROJECT_NAME}\""
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}"
)


target_link_libraries( ${PROJECT_NAME}  ${SACK_LIBRARIES} )

INSTALL( TARGETS ${PROJECT_NAME} 
	RUNTIME DESTINATION bin 
	LIBRARY DESTINATION bin 
)

SET( DEKWARE_SDK )


INSTALL( 
  FILES
    ${DEKWARE_SDK}
  DESTINATION
    include/dekware
)



