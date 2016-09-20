if( NOT WIN32 )
	# uses pipe(), fork()
        # needs compat
	add_subdirectory(openssl)
        # nc uses sys/un.h - unix sockets?
	add_subdirectory(nc)
endif( NOT WIN32 )
