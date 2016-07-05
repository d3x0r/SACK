cmake_minimum_required(VERSION 2.8)


PROJECT( ballpicker )

if( NOT SACK_SDK_ROOT_PATH )
GET_FILENAME_COMPONENT(SACK_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\SACK;Install_Dir]" ABSOLUTE CACHE)
else( NOT SACK_SDK_ROOT_PATH )
set(SACK_SDK_ROOT_PATH ${SACK_SDK_ROOT_PATH} CACHE STRING "SACK SDK Root" )
endif( NOT SACK_SDK_ROOT_PATH )

include( ${SACK_SDK_ROOT_PATH}/CMakePackage )
INSTALL_SACK( . )
include_directories( ${SACK_INCLUDE_DIR} )
LINK_DIRECTORIES( ${SACK_LIBRARY_DIR} )

SET( SOURCES 
${FIRST_GCC_PROGRAM_SOURCE} 
    ball_picker.c ball_shuffler.c cardset_loader.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_executable( ${PROJECT_NAME} ${SOURCES} )
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION . )


add_program( cardset_compare cardset_compare.c )
target_link_libraries( cardset_compare ${SACK_LIBRARIES} )
install( TARGETS cardset_compare RUNTIME DESTINATION . )

add_program( make_cardset_compare makebat.c )
target_link_libraries( make_cardset_compare ${SACK_LIBRARIES} )
install( TARGETS make_cardset_compare RUNTIME DESTINATION . )

add_program( print_card print_card.c )
target_link_libraries( print_card ${SACK_LIBRARIES} )
install( TARGETS print_card RUNTIME DESTINATION . )

add_program( count_card_stats count_card_stats.c )
target_link_libraries( count_card_stats ${SACK_LIBRARIES} )
install( TARGETS count_card_stats RUNTIME DESTINATION . )

add_program( find_card find_card.c )
target_link_libraries( find_card ${SACK_LIBRARIES} )
install( TARGETS find_card RUNTIME DESTINATION . )


