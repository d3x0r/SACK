
if( UNIX )
# because sources are common to monolithic build mode
# they are already defined in a variable calld VIDLIB_SOURCES (defined in CMakeSources.lst)



add_library( bag.video.wayland SHARED
  ${FIRST_GCC_LIBRARY_SOURCE}
  vidlib.wayland.c
	${CMAKE_BINARY_DIR}/xdg-shell-protocol.c
	${CMAKE_BINARY_DIR}/xdg-shell-client-protocol.h
  ${LAST_GCC_LIBRARY_SOURCE}
)

INCLUDE_DIRECTORIES( ${CMAKE_BINARY_DIR} )
  add_custom_command( OUTPUT ${CMAKE_BINARY_DIR}/xdg-shell-client-protocol.h
   			COMMAND wayland-scanner client-header /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml ${CMAKE_BINARY_DIR}/xdg-shell-client-protocol.h
  )
  add_custom_command( OUTPUT ${CMAKE_BINARY_DIR}/xdg-shell-protocol.c
   			 COMMAND wayland-scanner private-code /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml ${CMAKE_BINARY_DIR}/xdg-shell-protocol.c
  )


SET_TARGET_PROPERTIES(bag.video.wayland PROPERTIES
                  COMPILE_FLAGS  "-DRENDER_LIBRARY_SOURCE"
                  COMPILE_DEFINITIONS "TARGET_LABEL=bag_video_wayland;TARGETNAME=\"${CMAKE_SHARED_LIBRARY_PREFIX}bag.video.wayland${CMAKE_SHARED_LIBRARY_SUFFIX}\";${ExtraRenderFlags}"
                  FOLDER "Video Renderer"
                  LINK_FLAGS "${extra_link_flags}"
)

my_target_link_libraries( bag.video.wayland bag ${EXTERNALS} wayland-client xkbcommon ${PLATFORM_LIBRARIES})



install_mode_dest( bag.video.wayland )

endif( UNIX )
