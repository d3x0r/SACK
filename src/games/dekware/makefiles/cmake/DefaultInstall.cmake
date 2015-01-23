
SET( HEADER_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/include )
SET( DATA_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/resources )

macro( install_default_dest )
if( WIN32 )
	# On Windows platforms, the dynamic libs should
	# go in the same dir as the executables.
	install( TARGETS ${ARGV}
	        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin 
        	LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
	        ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib )
else( WIN32 )
	install( TARGETS ${ARGV}
	        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin 
        	LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
	        ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib )
endif( WIN32 )
endmacro( install_default_dest )

macro( install_default_dest_binary )
if( WIN32 )
	# On Windows platforms, the dynamic libs should
	# go in the same dir as the executables.
	install( TARGETS ${ARGV}
	        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin 
        	LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/bin )
else( WIN32 )
	install( TARGETS ${ARGV}
	        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin 
        	LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib )
endif( WIN32 )
endmacro( install_default_dest_binary )

