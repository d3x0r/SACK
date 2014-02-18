
# because sources are common to monolithic build mode
# they are already defined in a variable calld VIDLIB_SOURCES (defined in CMakeSources.lst)

if( FORCE_CXX )
    set_source_files_properties( ${IMAGE_SOURCES}  PROPERTIES LANGUAGE CXX )
endif( FORCE_CXX )

add_library( bag.image SHARED 
  ${FIRST_GCC_LIBRARY_SOURCE} 
  ${IMAGE_SOURCES} 
  ${LAST_GCC_LIBRARY_SOURCE} 
)

SET_TARGET_PROPERTIES(bag.image PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=bag_image ${ExtraFlags} ${ImageExtraFlags} -DMAKE_RCOORD_SINGLE" 
                  FOLDER "Image Library" 
                  LINK_FLAGS "${extra_link_flags}"
)
if( __ANDROID__ )
    SET_PROPERTY( TARGET bag.image APPEND PROPERTY COMIPLE_FLAGS "-DUSE_OPENGL_COMPAT_COLORS" )
endif( __ANDROID__ )

my_target_link_libraries( bag.image bag ${EXTERNALS} ${PLATFORM_LIBRARIES} )
my_target_link_libraries( bag.image bag.psi )

install_mode_dest( bag.image )

if( NOT NEED_PNG ) 
  my_target_link_libraries( bag.image png )
endif() 
if( NOT NEED_JPEG ) 
  my_target_link_libraries( bag.image jpeg )
endif() 

if( NOT NEED_FREETYPE )
  my_target_link_libraries( bag.image  freetype  )
  include_directories( ${FREETYPE_INCLUDE_DIRS} )
endif()

