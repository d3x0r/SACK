
set( project Dekware.core )


set(SOURCES 
${FIRST_GCC_LIBRARY_SOURCE}
nexus.c commands.c sentcmds.c entcmds.c  datacmds.c varcmds.c syscmds.c input.c plugins.c
interface.c 
${LAST_GCC_LIBRARY_SOURCE} )


add_library( ${project} SHARED ${SOURCES} )

string( REGEX REPLACE "(.*)\\.(.*)" "\\1_\\2" TARGET_LABEL ${project} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  DEFINE_SYMBOL CORE_SOURCE
                  COMPILE_DEFINITIONS "COMPILED_INSTALL_PREFIX=\"${COMPILED_INSTALL_PREFIX}\";TARGETNAME=\"${project}\""
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}"
)
if( NOT __ANDROID__ )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  PREFIX ""
                  SUFFIX ""
)
endif( NOT __ANDROID__ )

target_link_libraries( ${project}  ${SACK_LIBRARIES} )

install_literal_product( ${project} applicationCore ON )

SET( DEKWARE_SDK )


INSTALL( 
  FILES
    ${DEKWARE_SDK}
  DESTINATION
    include/dekware
)



