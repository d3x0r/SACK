cmake_minimum_required(VERSION 2.8)

set( project keypad.isp )

set( SOURCES
${FIRST_GCC_LIBRARY_SOURCE} 
keypad
${LAST_GCC_LIBRARY_SOURCE}
)


add_library_force_source(${project} SHARED ${SOURCES} )



string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} ${LibraryExtraFlags}"
                  COMPILE_DEFINITIONS "TARGETNAME=\"${project}\""
                  FOLDER "InterShell/plugins"
)
if( NOT __ANDROID__ )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  SUFFIX ""
                  PREFIX ""
)
endif( NOT __ANDROID__ )
my_target_link_libraries( ${project} ${SACK_LIBRARIES} sack_widgets )
install_plugin( ${project} ) 
INSTALL( 
  FILES
    keypad.h
  DESTINATION
    include/SACK/InterShell/widgets/keypad
)

add_subdirectory( frames )

