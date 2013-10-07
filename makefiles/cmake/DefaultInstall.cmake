
if( WIN32 )
  if( __CLR__ )
    set( BINARY_OUTPUT_DIR . )
    set( SHARED_LIBRARY_OUTPUT_DIR . )
  else( __CLR__ )
    set( BINARY_OUTPUT_DIR bin )
    set( SHARED_LIBRARY_OUTPUT_DIR bin )
  endif( __CLR__ )
else( WIN32 )
   if( __LINUX64__ )
      set( BINARY_OUTPUT_DIR bin )
      set( SHARED_LIBRARY_OUTPUT_DIR lib64 )
   else( __LINUX64__ )
      set( BINARY_OUTPUT_DIR bin )
      set( SHARED_LIBRARY_OUTPUT_DIR lib )
   endif( __LINUX64__ )
endif( WIN32 )

if( __ANDROID__ )
else( __ANDROID__ )
    set( DEFAULT_WORKING_DIRECTORY ${BINARY_OUTPUT_DIR} )
endif( __ANDROID__ )



SET( HEADER_INSTALL_PREFIX include )
SET( DATA_INSTALL_PREFIX resources )

macro( install_default_dest )
   if( TARGET_BINARY_PATH )
	install( TARGETS ${ARGV}
		RUNTIME DESTINATION ${BINARY_OUTPUT_DIR}/${TARGET_BINARY_PATH}
		LIBRARY DESTINATION ${SHARED_LIBRARY_OUTPUT_DIR}/${TARGET_BINARY_PATH}
		ARCHIVE DESTINATION lib )
   else( TARGET_BINARY_PATH )
	install( TARGETS ${ARGV}
		RUNTIME DESTINATION ${BINARY_OUTPUT_DIR}
		LIBRARY DESTINATION ${SHARED_LIBRARY_OUTPUT_DIR}
		ARCHIVE DESTINATION lib )
   endif( TARGET_BINARY_PATH )
endmacro( install_default_dest )

macro( install_mode_dest )
    if( __LINUX64__ )
	install( TARGETS ${ARGV}
		RUNTIME DESTINATION ${BINARY_OUTPUT_DIR}
		LIBRARY DESTINATION ${SHARED_LIBRARY_OUTPUT_DIR}
		ARCHIVE DESTINATION lib64 )
    else( __LINUX64__ )
	install( TARGETS ${ARGV}
		RUNTIME DESTINATION ${BINARY_OUTPUT_DIR}
		LIBRARY DESTINATION ${SHARED_LIBRARY_OUTPUT_DIR}
		ARCHIVE DESTINATION lib )
    endif( __LINUX64__ )
endmacro( install_mode_dest )

macro( install_sack_sdk_dest )
	install( TARGETS ${ARGV}
		RUNTIME DESTINATION ${SACK_BASE}/${BINARY_OUTPUT_DIR} 
		LIBRARY DESTINATION ${SACK_BASE}/${SHARED_LIBRARY_OUTPUT_DIR}
		ARCHIVE DESTINATION ${SACK_BASE}/lib )
endmacro( install_sack_sdk_dest )


macro( install_default_dest_binary )
if( TARGET_BINARY_PATH )
	install( TARGETS ${ARGV}
		RUNTIME DESTINATION ${BINARY_OUTPUT_DIR}/${TARGET_BINARY_PATH} 
		LIBRARY DESTINATION ${SHARED_LIBRARY_OUTPUT_DIR}/${TARGET_BINARY_PATH} )
else( TARGET_BINARY_PATH )
	install( TARGETS ${ARGV}
		RUNTIME DESTINATION ${BINARY_OUTPUT_DIR} 
		LIBRARY DESTINATION ${SHARED_LIBRARY_OUTPUT_DIR} )
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
      install( TARGETS ${proj} RUNTIME DESTINATION ${BINARY_OUTPUT_DIR}/${project_target}
			ARCHIVE DESTINATION lib		
	)
    endif( __CLR__ )
  else( WIN32 )
    install( TARGETS ${proj} 
	RUNTIME DESTINATION ${BINARY_OUTPUT_DIR}/${project_target} 
	LIBRARY DESTINATION ${BINARY_OUTPUT_DIR}/${project_target} 
        ARCHIVE DESTINATION lib
	)
endif()

endif( __ANDROID__ )
endmacro( install_literal_product )

macro( install_default_project proj project_target )

if( WIN32 )
  install( TARGETS ${proj} RUNTIME DESTINATION ${project_target} )
else( WIN32 )
  install( TARGETS ${proj} LIBRARY DESTINATION ${BINARY_OUTPUT_DIR}/${project_target} 
  		RUNTIME DESTINATION ${BINARY_OUTPUT_DIR}/${project_target} 
	)
endif()
endmacro( install_default_project )

macro( add_library_force_source project optional_style )
  if( optional_style STREQUAL SHARED )
    if( FORCE_CXX )
      set_source_files_properties(${ARGN} PROPERTIES LANGUAGE CXX )
      set_source_files_properties( ${ARGN} PROPERTIES COMPILE_FLAGS /CLR )
    endif( FORCE_CXX )

  else( optional_style STREQUAL SHARED )
    if( FORCE_CXX )
      set_source_files_properties( ${optional_style} ${ARGN} PROPERTIES LANGUAGE CXX )
      set_source_files_properties( ${optional_style} ${ARGN} PROPERTIES COMPILE_FLAGS /CLR )
    endif( FORCE_CXX )

  endif( optional_style STREQUAL SHARED )
  add_library( ${project} ${optional_style} ${ARGN} )
endmacro( add_library_force_source )


macro( add_executable_force_source project optional_style )
  if( optional_style STREQUAL WIN32 )
    if( FORCE_CXX )
      set_source_files_properties(${ARGN} PROPERTIES LANGUAGE CXX )
      set_source_files_properties( ${ARGN} PROPERTIES COMPILE_FLAGS /CLR )
    endif( FORCE_CXX )
  else( optional_style STREQUAL WIN32 )
    if( FORCE_CXX )
      set_source_files_properties( ${optional_style} ${ARGN} PROPERTIES LANGUAGE CXX )
      set_source_files_properties( ${optional_style} ${ARGN} PROPERTIES COMPILE_FLAGS /CLR )
    endif( FORCE_CXX )
  endif( optional_style STREQUAL WIN32 )
  add_executable( ${project} ${optional_style} ${ARGN} )
endmacro( add_executable_force_source )

macro( sack_add_executable project optional_style )
  add_executable( ${project} ${optional_style} ${ARGN} )
  string( REPLACE "." "_" TARGET_LABEL ${project} )
  SET_PROPERTY( TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "TARGETNAME=\"${project}${CMAKE_EXECUTABLE_SUFFIX}\";TARGET_LABEL=${TARGET_LABEL}" )
  if( CMAKE_COMPILER_IS_GNUCC OR WATCOM )
     SET_PROPERTY( TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "NO_DEADSTART_DLLMAIN" )
  endif( CMAKE_COMPILER_IS_GNUCC OR WATCOM )
  if( ${optional_style} STREQUAL "WIN32" )
  else( ${optional_style} STREQUAL "WIN32" )
     SET_PROPERTY( TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "CONSOLE_SHELL" )
  endif( ${optional_style} STREQUAL "WIN32" )
endmacro( sack_add_executable )


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


macro( add_portable_program_ex portable targetname option1 )
if( NOT SOURCES_ROOT )
set( SACK_SOURCES_ROOT ${SACK_BASE}/src/sack )
else( NOT SOURCES_ROOT )
set( SACK_SOURCES_ROOT ${SOURCES_ROOT}/src/deadstart )
endif( NOT SOURCES_ROOT )

	if( __ANDROID__ )
       		set( ExtraDefinitions "${ExtraDefinitions};CONSOLE_SHELL" )
		if( ${option1} STREQUAL WIN32 )
			if( portable )
				set( ExtraDefinitions "${ExtraDefinitions};BUILD_PORTABLE_EXECUTABLE" )
				add_library( ${targetname}.code SHARED ${ARGN} ${SOURCES_ROOT}/src/deadstart/deadstart_core.c )
			else( portable )
				add_library( ${targetname}.code SHARED ${ARGN} )
			endif( portable )
		else( ${option1} STREQUAL WIN32 )
			if( portable )
				set( ExtraDefinitions "${ExtraDefinitions};BUILD_PORTABLE_EXECUTABLE" )
				add_library( ${targetname}.code SHARED ${option1} ${ARGN} ${SOURCES_ROOT}/src/deadstart/deadstart_core.c )
			else( portable )
				add_library( ${targetname}.code SHARED ${option1} ${ARGN} )
			endif( portable )
		endif( ${option1} STREQUAL WIN32 )
		add_library( ${targetname} SHARED ${SACK_SOURCES_ROOT}/android/android_native_app_glue ${SACK_SOURCES_ROOT}/android/default_android_main ${SACK_SOURCES_ROOT}/android/android_util )
		my_target_link_libraries( ${targetname} android log )
		string( REPLACE "." "_" TARGET_LABEL ${targetname} )
		my_target_link_libraries( ${targetname}.code ${SACK_LIBRARIES} ${SACK_PLATFORM_LIBRARIES} )
		set_target_properties( ${targetname}.code PROPERTIES COMPILE_DEFINITIONS "${ExtraDefinitions};ANDROID_SHELL;TARGETNAME=\"${targetname}.code\";TARGET_LABEL=${TARGET_LABEL}_code" )
		set_target_properties( ${targetname} PROPERTIES COMPILE_DEFINITIONS "${ExtraDefinitions};ANDROID_SHELL;TARGETNAME=\"${targetname}\";TARGET_LABEL=${TARGET_LABEL}" )
		if( MAKING_SACK_CORE )
			install_mode_dest( ${targetname}.code )
			install_mode_dest( ${targetname} )
		else( MAKING_SACK_CORE )
			install_default_dest( ${targetname}.code )
			install_default_dest( ${targetname} )
		endif( MAKING_SACK_CORE )
	else( __ANDROID__ )
		if( ${portable} )
			set( ExtraDefinitions "${ExtraDefinitions};BUILD_PORTABLE_EXECUTABLE" )
                        if( FORCE_CXX )
				if( NOT ${option1} STREQUAL WIN32 )
	                        	set_source_files_properties( ${option1} PROPERTIES LANGUAGE CXX )
	                        	set_source_files_properties( ${option1} APPEND PROPERTIES COMPILE_FLAGS /CLR )
				endif( NOT ${option1} STREQUAL WIN32 )
                        	set_source_files_properties(${SOURCES_ROOT}/src/deadstart/deadstart_core.c ${ARGN} PROPERTIES LANGUAGE CXX )
                               	set_source_files_properties(${SOURCES_ROOT}/src/deadstart/deadstart_core.c ${ARGN} PROPERTIES COMPILE_FLAGS /CLR )
                        endif( FORCE_CXX )
                        if( NO_AUTO_DEADSTART_CORE )
				add_executable( ${targetname} ${option1} ${ARGN} )
                        else( NO_AUTO_DEADSTART_CORE )                                	
				add_executable( ${targetname} ${option1} ${SOURCES_ROOT}/src/deadstart/deadstart_core.c ${ARGN} )
                        endif( NO_AUTO_DEADSTART_CORE )
		else( ${portable} )
                        if( FORCE_CXX )
				if( NOT ${option1} STREQUAL WIN32 )
	                        	set_source_files_properties( ${option1} PROPERTIES LANGUAGE CXX )
	                        	set_source_files_properties( ${option1} PROPERTIES COMPILE_FLAGS /CLR )
				endif( NOT ${option1} STREQUAL WIN32 )
                        	set_source_files_properties( ${ARGN} PROPERTIES LANGUAGE CXX )
                               	set_source_files_properties( ${ARGN} PROPERTIES COMPILE_FLAGS /CLR )
                        endif( FORCE_CXX )
			add_executable( ${targetname} ${option1} ${ARGN} )
		endif( ${portable} )
		string( REPLACE "." "_" TARGET_LABEL ${targetname} )
		if( ${option1} STREQUAL WIN32 )
			set_target_properties( ${targetname} PROPERTIES COMPILE_DEFINITIONS "${ExtraDefinitions};WINDOWS_SHELL;TARGETNAME=\"${targetname}\";TARGET_LABEL=${TARGET_LABEL}" )
		else()
			set_target_properties( ${targetname} PROPERTIES COMPILE_DEFINITIONS "${ExtraDefinitions};CONSOLE_SHELL;TARGETNAME=\"${targetname}\";TARGET_LABEL=${TARGET_LABEL}" )
		endif()
		if( MAKING_SACK_CORE )
			install_mode_dest( ${targetname} )
		else( MAKING_SACK_CORE )
			install_default_dest( ${targetname} )
		endif( MAKING_SACK_CORE )
		my_target_link_libraries( ${targetname} ${SACK_PLATFORM_LIBRARIES} )
	endif( __ANDROID__ )

endmacro( add_portable_program_ex )

macro( add_portable_program targetname option1 )
	add_portable_program_ex( 1 ${targetname} ${option1} ${ARGN} )
endmacro( add_portable_program )

macro( add_program targetname option1 )
	add_portable_program_ex( 0 ${targetname} ${option1} ${ARGN} )
	if( __ANDROID__ )
		my_target_link_libraries( ${targetname}.code ${SACK_LIBRARIES} )
	else( __ANDROID__ )
		my_target_link_libraries( ${targetname} ${SACK_LIBRARIES} )
                
	endif( __ANDROID__ )
        
endmacro( add_program )

macro( DEFINE_DEFAULT variable default )
if( NOT DEFINED ${variable} )
   #message( "variable ${variable} not defined (command line)" )
   set( ${variable} $ENV{${variable}} )
   if( "${${variable}}" STREQUAL "" )
     set( ${variable} ${default} )
   endif( "${${variable}}" STREQUAL "" )
endif( NOT DEFINED ${variable} )
endmacro( DEFINE_DEFAULT variable )
