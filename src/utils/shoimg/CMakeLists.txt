cmake_minimum_required(VERSION 2.8)



include_directories( ${SACK_ROOT_}/sdk/include )


set( VLC_INTERFACE_ONLY ON )
add_subdirectory( ../../InterShell/vlc_hook __/__/InterShell/vlc_hook )
include_directories( ../../InterShell/vlc_hook )

include_directories( ../../libvlc/1.1.6/sdk/include )

##############################################
set( project crossfade )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    crossfade.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_program(${project} WIN32 ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
COMPILE_FLAGS  "-DTARGET_LABEL=${project}" 
                  FOLDER utils/crossfade
)
target_link_libraries( ${project} ${SACK_LIBRARIES})
install_default_dest( ${project} )


##############################################
set( project crossfade_vid )

include_directories( ../../InterShell/vlc_hook )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    crossfade_vid.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_program(${project} WIN32 ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
COMPILE_FLAGS  "-DTARGET_LABEL=${project}" 
                  FOLDER utils/crossfade
)
target_link_libraries( ${project} ${SACK_LIBRARIES} vlc_interface)
install_default_dest( ${project} )


##############################################
set( project crossfade_vid_playlist )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    crossfade_vid_playlist.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_program(${project} WIN32 ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER utils/crossfade
)
target_link_libraries( ${project} ${SACK_LIBRARIES} vlc_interface)
install_default_dest( ${project} )

##############################################
set( project crossfade_vid_trigger )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    crossfade_vid_trigger.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_program(${project} WIN32 ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER utils/crossfade
)
target_link_libraries( ${project} ${SACK_LIBRARIES} winmm )
install_default_dest( ${project} )

##############################################

set( project bounce )


set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    bounce.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_program(${project} WIN32 ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER utils
)
target_link_libraries( ${project} ${SACK_LIBRARIES} )
install_default_dest( ${project} )

##############################################

set( project bounce2 )


set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    bounce2.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_program(${project} WIN32 ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER utils
)
target_link_libraries( ${project} ${SACK_LIBRARIES} )
install_default_dest( ${project} )

##############################################

set( project crossfade_vid_playlist_game )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    crossfade_vid_playlist_game.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_program(${project} WIN32 ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER utils/crossfade
)
target_link_libraries( ${project} ${SACK_LIBRARIES} vlc_interface)
install_default_dest( ${project} )

