
cmake_minimum_required(VERSION 2.8)
project( sack_widgets ) 

set(SOURCES 
   banner/banner.c 
   keypad/buttons.c 
   keypad/keypad_control.c 
   keypad/accum.c 
)

add_definitions( 
	-DBANNER_SOURCE  -DKEYPAD_SOURCE
)

add_library_force_source( ${PROJECT_NAME} SHARED 
   ${FIRST_GCC_LIBRARY_SOURCE}
      ${SOURCES} 
   ${LAST_GCC_LIBRARY_SOURCE}
)

string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_DEFINITIONS "TARGETNAME=\"${CMAKE_SHARED_LIBRARY_PREFIX}${PROJECT_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}\""
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} ${LibraryExtraFlags}"
                  FOLDER InterShell
)

target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES})

install_default_dest( ${PROJECT_NAME} )

if( INSTALL_TO_SACK_SDK )
install_sack_sdk_dest( ${PROJECT_NAME} )
endif( INSTALL_TO_SACK_SDK )

if( WIN32 )
target_link_libraries( ${PROJECT_NAME} winmm )
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

