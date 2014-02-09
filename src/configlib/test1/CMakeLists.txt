
add_executable(test_configscript ${FIRST_GCC_PROGRAM_SOURCE} 
		test.c 
                ${LAST_GCC_PROGRAM_SOURCE} 
                )
target_link_libraries( test_configscript ${SACK_LIBRARIES} )                
SET_TARGET_PROPERTIES( test_configscript PROPERTIES
		COMPILE_FLAGS  "-DTARGET_LABEL=test_configscript" 
                  FOLDER tests
)
install_default_dest( test_configscript )
