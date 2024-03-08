
add_program( sack_ping ${FIRST_GCC_PROGRAM_SOURCE} 
	ping.c
${LAST_GCC_PROGRAM_SOURCE}
)
SET_TARGET_PROPERTIES(sack_ping PROPERTIES FOLDER "Network Examples" )

