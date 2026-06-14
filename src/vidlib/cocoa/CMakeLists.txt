
if( APPLE )
# macOS (Cocoa) framebuffer/window video backend.  Sources are plain C; all
# AppKit access is via the Objective-C runtime, so no .m / Objective-C sources.

add_library( bag.video.cocoa SHARED
  ${FIRST_GCC_LIBRARY_SOURCE}
  vidlib.cocoa.c
  ${LAST_GCC_LIBRARY_SOURCE}
)

SET_TARGET_PROPERTIES( bag.video.cocoa PROPERTIES
                  COMPILE_FLAGS  "-DRENDER_LIBRARY_SOURCE"
                  COMPILE_DEFINITIONS "TARGET_LABEL=bag_video_cocoa;TARGETNAME=\"${CMAKE_SHARED_LIBRARY_PREFIX}bag.video.cocoa${CMAKE_SHARED_LIBRARY_SUFFIX}\";${ExtraRenderFlags}"
                  FOLDER "Video Renderer"
                  LINK_FLAGS "${extra_link_flags}"
)

# Cocoa pulls in AppKit/Foundation; QuartzCore provides CALayer; CoreGraphics
# provides CGImage.  libobjc is linked automatically.
find_library( COCOA_LIBRARY Cocoa )
find_library( QUARTZCORE_LIBRARY QuartzCore )
find_library( COREGRAPHICS_LIBRARY CoreGraphics )

my_target_link_libraries( bag.video.cocoa bag ${EXTERNALS}
        ${COCOA_LIBRARY} ${QUARTZCORE_LIBRARY} ${COREGRAPHICS_LIBRARY}
        ${PLATFORM_LIBRARIES} )

install_mode_dest( bag.video.cocoa )

endif( APPLE )
