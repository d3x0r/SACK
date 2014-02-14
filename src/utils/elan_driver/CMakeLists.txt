project( elan_driver )

include_directories( original )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    elan_driver.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_executable(${PROJECT_NAME} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
COMPILE_FLAGS  "-DTARGET_LABEL=${PROJECT_NAME}" 
                  FOLDER utils/elan
)

target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} elan_driver_library )

install_default_dest( ${PROJECT_NAME} )



project( elan_driver_library )

set( BASE_SOURCES
	${FIRST_GCC_LIBRARY_SOURCE} 
	    original/usbpulse100drvr.cpp
	${LAST_GCC_LIBRARY_SOURCE}
)

add_library(${PROJECT_NAME} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
COMPILE_FLAGS  "-DTARGET_LABEL=${PROJECT_NAME}" 
                  FOLDER utils/elan
)

target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )

install_default_dest( ${PROJECT_NAME} )



