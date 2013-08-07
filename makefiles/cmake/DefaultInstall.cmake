
SET( HEADER_INSTALL_PREFIX include )
SET( DATA_INSTALL_PREFIX resources )

macro( install_default_dest )
if( TARGET_BINARY_PATH )
if( WIN32 )
	# On Windows platforms, the dynamic libs should
	# go in the same dir as the executables.
if( __CLR__ )
	install( TARGETS ${ARGV}
	        RUNTIME DESTINATION ./${TARGET_BINARY_PATH}
        	LIBRARY DESTINATION ./${TARGET_BINARY_PATH}
	        ARCHIVE DESTINATION lib )
else( __CLR__ )
	install( TARGETS ${ARGV}
	        RUNTIME DESTINATION bin/${TARGET_BINARY_PATH}
        	LIBRARY DESTINATION bin/${TARGET_BINARY_PATH}
	        ARCHIVE DESTINATION lib )
endif( __CLR__ )
else( WIN32 )
	install( TARGETS ${ARGV}
	        RUNTIME DESTINATION bin/${TARGET_BINARY_PATH}
        	LIBRARY DESTINATION lib/${TARGET_BINARY_PATH}
	        ARCHIVE DESTINATION lib )
endif( WIN32 )
else( TARGET_BINARY_PATH )
if( WIN32 )
	# On Windows platforms, the dynamic libs should
	# go in the same dir as the executables.
if( __CLR__ )
	install( TARGETS ${ARGV}
	        RUNTIME DESTINATION .
        	LIBRARY DESTINATION .
	        ARCHIVE DESTINATION lib )
else( __CLR__ )
	install( TARGETS ${ARGV}
	        RUNTIME DESTINATION bin
        	LIBRARY DESTINATION bin
	        ARCHIVE DESTINATION lib )
endif( __CLR__ )
else( WIN32 )
	install( TARGETS ${ARGV}
	        RUNTIME DESTINATION bin
        	LIBRARY DESTINATION lib
	        ARCHIVE DESTINATION lib )
endif( WIN32 )
endif( TARGET_BINARY_PATH )
endmacro( install_default_dest )

macro( install_mode_dest )
if( __CLR__ )
	# On Windows platforms, the dynamic libs should
	# go in the same dir as the executables.
	install( TARGETS ${ARGV}
	        RUNTIME DESTINATION .
        	LIBRARY DESTINATION .
	        ARCHIVE DESTINATION lib )
else( __CLR__ )
if( WIN32 )
	# On Windows platforms, the dynamic libs should
	# go in the same dir as the executables.
	install( TARGETS ${ARGV}
	        RUNTIME DESTINATION bin
        	LIBRARY DESTINATION bin
	        ARCHIVE DESTINATION lib )
elseif( __LINUX64__ )
	install( TARGETS ${ARGV}
	        RUNTIME DESTINATION bin
        	LIBRARY DESTINATION lib64
	        ARCHIVE DESTINATION lib64 )
elseif( __LINUX__ )
	install( TARGETS ${ARGV}
	        RUNTIME DESTINATION bin
        	LIBRARY DESTINATION lib
	        ARCHIVE DESTINATION lib )
endif( WIN32 )
endif( __CLR__ )
endmacro( install_mode_dest )

macro( install_sack_sdk_dest )
if( WIN32 )
if( __CLR__ )
	# On Windows platforms, the dynamic libs should
	# go in the same dir as the executables.
	install( TARGETS ${ARGV}
	        RUNTIME DESTINATION ${SACK_BASE}
        	LIBRARY DESTINATION ${SACK_BASE}
	        ARCHIVE DESTINATION ${SACK_BASE}/lib )
else( __CLR__ )
	install( TARGETS ${ARGV}
	        RUNTIME DESTINATION ${SACK_BASE}/bin
        	LIBRARY DESTINATION ${SACK_BASE}/bin
	        ARCHIVE DESTINATION ${SACK_BASE}/lib )
endif( __CLR__ )
else( WIN32 )
	install( TARGETS ${ARGV}
	        RUNTIME DESTINATION ${SACK_BASE}/bin 
        	LIBRARY DESTINATION ${SACK_BASE}/lib
	        ARCHIVE DESTINATION ${SACK_BASE}/lib )
endif( WIN32 )
endmacro( install_sack_sdk_dest )


macro( install_default_dest_binary )
if( TARGET_BINARY_PATH )
if( WIN32 )
	# On Windows platforms, the dynamic libs should
	# go in the same dir as the executables.
if( __CLR__ )
	install( TARGETS ${ARGV}
	        RUNTIME DESTINATION .
        	LIBRARY DESTINATION .)
else( __CLR__ )
	install( TARGETS ${ARGV}
	        RUNTIME DESTINATION bin/${TARGET_BINARY_PATH} 
        	LIBRARY DESTINATION bin/${TARGET_BINARY_PATH} )
endif( __CLR__ )
else( WIN32 )
	install( TARGETS ${ARGV}
	        RUNTIME DESTINATION bin 
        	LIBRARY DESTINATION lib )
endif( WIN32 )
else( TARGET_BINARY_PATH )
if( WIN32 )
	# On Windows platforms, the dynamic libs should
	# go in the same dir as the executables.
if( __CLR__ )
	install( TARGETS ${ARGV}
	        RUNTIME DESTINATION .
        	LIBRARY DESTINATION . )
else( __CLR__ )
	install( TARGETS ${ARGV}
	        RUNTIME DESTINATION bin 
        	LIBRARY DESTINATION bin )
endif( __CLR__ )
else( WIN32 )
	install( TARGETS ${ARGV}
	        RUNTIME DESTINATION bin 
        	LIBRARY DESTINATION lib )
endif( WIN32 )
endif( TARGET_BINARY_PATH )
endmacro( install_default_dest_binary )

if( OFF )
include( GetPrerequisites )

macro( install_product proj )
  get_prerequisites( ${${proj}_BINARY_DIR}/${proj} filelist 1 0 "" "" )
  #exclude_system recurse dirs
  message( "pre-req: [${filelist}] ${proj} (${${proj}_BINARY_DIR}/${PROJECT}${CMAKE_EXECUTABLE_SUFFIX})" )
endmacro( install_product )
endif( OFF )

macro( install_literal_product proj project_target )
if( __ANDROID__ )
  install( TARGETS ${proj} RUNTIME DESTINATION lib/${project_target} 
  		LIBRARY DESTINATION lib/${project_target} 
  	)
else( __ANDROID__ )
if( WIN32 )
if( __CLR__ )
  install( TARGETS ${proj} RUNTIME DESTINATION ./${project_target} 
  	)
else( __CLR__ )
  install( TARGETS ${proj} RUNTIME DESTINATION bin/${project_target} 
  	)
endif( __CLR__ )
else( WIN32 )
  install( TARGETS ${proj} 
	RUNTIME DESTINATION bin/${project_target} 
	LIBRARY DESTINATION bin/${project_target} 
  	)
endif()

endif( __ANDROID__ )
endmacro( install_literal_product )

macro( install_default_project proj project_target )

if( WIN32 )
  install( TARGETS ${proj} RUNTIME DESTINATION ${project_target} )
else( WIN32 )
  install( TARGETS ${proj} LIBRARY DESTINATION bin/${project_target} 
  	)
endif()
endmacro( install_default_project )

macro( add_library_force_source project optional_style )
  if( optional_style STREQUAL SHARED )
    if( FORCE_CXX )
      set_source_files_properties(${ARGN} PROPERTIES LANGUAGE CXX )
    endif( FORCE_CXX )

  else( optional_style STREQUAL SHARED )
    if( FORCE_CXX )
      set_source_files_properties( ${optional_style} ${ARGN} PROPERTIES LANGUAGE CXX )
    endif( FORCE_CXX )

  endif( optional_style STREQUAL SHARED )
  add_library( ${project} ${optional_style} ${ARGN} )
endmacro( add_library_force_source )


macro( add_executable_force_source project optional_style )
  if( optional_style STREQUAL WIN32 )
    if( FORCE_CXX )
      set_source_files_properties(${ARGN} PROPERTIES LANGUAGE CXX )
    endif( FORCE_CXX )
  else( optional_style STREQUAL WIN32 )
    if( FORCE_CXX )
      set_source_files_properties( ${optional_style} ${ARGN} PROPERTIES LANGUAGE CXX )
    endif( FORCE_CXX )
  endif( optional_style STREQUAL WIN32 )
  add_executable( ${project} ${optional_style} ${ARGN} )
endmacro( add_executable_force_source )

macro(my_target_link_libraries target )
    if(CMAKE_COMPILER_IS_GNUCC AND __ANDROID__ )
       foreach( target_lib ${ARGN} )
          if( TARGET ${target_lib} )
             get_property( lib_path TARGET ${target_lib} PROPERTY LOCATION)
             get_property( existing_outname TARGET ${target_lib} PROPERTY OUTPUT_NAME )
             if( NOT existing_outname )
	             set( existing_outname ${target_lib} )
             endif( NOT existing_outname )
             if( ${lib_path} MATCHES "(.*)/([^/]*)$" )
                get_target_property(existing_link_flags ${target} LINK_FLAGS)
                if(existing_link_flags)
                
                    set(new_link_flags "${existing_link_flags} -L ${CMAKE_MATCH_1} -l ${existing_outname}")
                else()
                    set(new_link_flags "-L ${CMAKE_MATCH_1} -l ${existing_outname}")
                endif()
                set_target_properties( ${target} PROPERTIES LINK_FLAGS ${new_link_flags})
					 add_dependencies( ${target} ${target_lib} )
             endif( ${lib_path} MATCHES "(.*)/([^/]*)$" )
          else()
             target_link_libraries( ${target} ${target_lib} )
          endif( TARGET ${target_lib} )
       endforeach( target_lib ${ARGN} )
    else()
    	target_link_libraries( ${target} ${ARGN} )
    endif()
endmacro()


macro( add_program targetname option1 )
if( NOT SOURCES_ROOT )
set( SACK_SOURCES_ROOT ${SACK_BASE}/src/sack )
else( NOT SOURCES_ROOT )
set( SACK_SOURCES_ROOT ${SOURCES_ROOT}/src/deadstart )
endif( NOT SOURCES_ROOT )

	if( __ANDROID__ )
		if( ${option1} STREQUAL WIN32 )
			add_library( ${targetname}.code SHARED ${ARGN} )
                        add_library( ${targetname} SHARED ${SACK_SOURCES_ROOT}/android/android_native_app_glue ${SACK_SOURCES_ROOT}/android/default_android_main ${SACK_SOURCES_ROOT}/android/android_util )
		else( ${option1} STREQUAL WIN32 )
			add_library( ${targetname}.code SHARED ${option1} ${ARGN} )
                        add_library( ${targetname} SHARED ${SACK_SOURCES_ROOT}/android/android_native_app_glue ${SACK_SOURCES_ROOT}/android/default_android_main ${SACK_SOURCES_ROOT}/android/android_util )
		endif( ${option1} STREQUAL WIN32 )
		my_target_link_libraries( ${targetname} android log )
		string( REPLACE "." "_" TARGET_LABEL ${targetname} )
		my_target_link_libraries( ${targetname}.code ${SACK_LIBRARIES}  )
       		set_target_properties( ${targetname}.code PROPERTIES COMPILE_DEFINITIONS "WINDOWS_SHELL;TARGETNAME=\"${targetname}.code\";TARGET_LABEL=${TARGET_LABEL}_code" )
       		set_target_properties( ${targetname} PROPERTIES COMPILE_DEFINITIONS "WINDOWS_SHELL;TARGETNAME=\"${targetname}\";TARGET_LABEL=${TARGET_LABEL}" )
		if( MAKING_SACK_CORE )
			install_mode_dest( ${targetname}.code )
			install_mode_dest( ${targetname} )
		else( MAKING_SACK_CORE )
			install_default_dest( ${targetname}.code )
			install_default_dest( ${targetname} )
		endif( MAKING_SACK_CORE )
	else( __ANDROID__ )
		add_executable( ${targetname} ${option1} ${ARGN} )
		string( REPLACE "." "_" TARGET_LABEL ${targetname} )
		if( ${option1} STREQUAL WIN32 )
			set_target_properties( ${targetname} PROPERTIES COMPILE_DEFINITIONS "WINDOWS_SHELL;TARGETNAME=\"${targetname}\";TARGET_LABEL=${TARGET_LABEL}" )
		else()
			set_target_properties( ${targetname} PROPERTIES COMPILE_DEFINITIONS "CONSOLE_SHELL;TARGETNAME=\"${targetname}\";TARGET_LABEL=${TARGET_LABEL}" )
		endif()
		if( MAKING_SACK_CORE )
			install_mode_dest( ${targetname} )
		else( MAKING_SACK_CORE )
			install_default_dest( ${targetname} )
		endif( MAKING_SACK_CORE )
		my_target_link_libraries( ${targetname} ${SACK_LIBRARIES}  )
	endif( __ANDROID__ )

endmacro( add_program )


macro( DEFINE_DEFAULT variable default )
if( NOT DEFINED ${variable} )
   message( "variable ${variable} not defined (command line)" )
   set( ${variable} $ENV{${variable}} )
   if( "${${variable}}" STREQUAL "" )
     set( ${variable} ${default} )
   endif( "${${variable}}" STREQUAL "" )
endif( NOT DEFINED ${variable} )
endmacro( DEFINE_DEFAULT variable )
