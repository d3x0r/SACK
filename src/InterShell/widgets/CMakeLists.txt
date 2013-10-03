
set( project sack_widgets ) 

set(SOURCES 
   banner/banner.c 
   keypad/buttons.c 
   keypad/keypad_control.c 
   keypad/accum.c 
)

add_definitions( 
	-DBANNER_SOURCE  -DKEYPAD_SOURCE
)

add_library_force_source( ${project} SHARED 
   ${FIRST_GCC_LIBRARY_SOURCE}
      ${SOURCES} 
   ${LAST_GCC_LIBRARY_SOURCE}
)

string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  COMPILE_DEFINITIONS "TARGETNAME=\"${CMAKE_SHARED_LIBRARY_PREFIX}${project}${CMAKE_SHARED_LIBRARY_SUFFIX}\""
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} ${LibraryExtraFlags}"
                  FOLDER InterShell
)

target_link_libraries( ${project} ${SACK_LIBRARIES})

install_default_dest( ${project} )

if( INSTALL_TO_SACK_SDK )
install_sack_sdk_dest( ${project} )
endif( INSTALL_TO_SACK_SDK )

if( WIN32 )
target_link_libraries( ${project} winmm )
endif( WIN32 )

SET( WIDGET_SDK
  include/banner.h include/buttons.h include/keypad.h
)

INSTALL( 
  FILES
    ${WIDGET_SDK}
  DESTINATION
    include/SACK/InterShell/widgets
)

if( INSTALL_TO_SACK_SDK )
INSTALL( 
  FILES
    ${WIDGET_SDK}
  DESTINATION
    ${SACK_BASE}/include/SACK/widgets )
endif( INSTALL_TO_SACK_SDK )

