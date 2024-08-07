
set( project sack_widgets ) 
INCLUDE_DIRECTORIES( include )

set(SOURCES 
   chat/scrollable_chat_list.c
   chat/scroll_chat_input_prompt.c
   chat/chat_keydefs.c
   chat/image_viewer.c
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

SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER InterShell
)

target_link_libraries( ${project} ${SACK_LIBRARIES} )
install_default_dest( ${project} )

if( INSTALL_TO_SACK_SDK )
install_sack_sdk_dest( ${project} )
endif( INSTALL_TO_SACK_SDK )

if( WIN32 )
target_link_libraries( ${project} winmm )
endif( WIN32 )

SET( WIDGET_SDK
  include/banner.h include/buttons.h include/keypad.h include/chat_control.h
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

INSTALL( FILES chat/chat-decoration.png DESTINATION ${DATA_INSTALL_PREFIX}/images )

