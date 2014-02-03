
add_subdirectory( plugins )

set( headers 
 deprecated.h
 libvlc.h
 libvlc_events.h
 libvlc_media.h
 libvlc_media_discoverer.h
 libvlc_media_library.h
 libvlc_media_list.h
 libvlc_media_list_player.h
 libvlc_media_player.h
 libvlc_structures.h
 libvlc_version.h
 libvlc_vlm.h
 vlc.h
)

INSTALL( FILES ${headers} DESTINATION include/SACK/vlc )

