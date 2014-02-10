
add_program( scan_ports ${FIRST_GCC_PROGRAM_SOURCE} 
scanner 
${LAST_GCC_PROGRAM_SOURCE}
)
SET_TARGET_PROPERTIES(scan_ports PROPERTIES FOLDER "Network Examples" )


if( NOT __ARM__ )

add_program( scan_addresses ${FIRST_GCC_PROGRAM_SOURCE} 
      pinger 
${LAST_GCC_PROGRAM_SOURCE}
)
SET_TARGET_PROPERTIES(scan_addresses PROPERTIES  FOLDER "Network Examples")


endif( NOT __ARM__ )

PROJECT( nameof )
add_program( nameof ${FIRST_GCC_PROGRAM_SOURCE} 
      nameof 
${LAST_GCC_PROGRAM_SOURCE}
)
SET_TARGET_PROPERTIES(nameof PROPERTIES		FOLDER "Network Examples" )

add_program( ipof ${FIRST_GCC_PROGRAM_SOURCE} 
    ipof 
  ${LAST_GCC_PROGRAM_SOURCE}
)
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES FOLDER "Network Examples" )



add_subdirectory( portable )

