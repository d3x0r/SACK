cmake_minimum_required(VERSION 2.8)

set( Project sack.core.framebuffer )

if( NOT SACK_SDK_ROOT_PATH )
GET_FILENAME_COMPONENT(SACK_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\SACK;Install_Dir]" ABSOLUTE CACHE)
else( NOT SACK_SDK_ROOT_PATH )
set(SACK_SDK_ROOT_PATH ${SACK_SDK_ROOT_PATH} CACHE STRING "SACK SDK Root" )
endif( NOT SACK_SDK_ROOT_PATH )



macro( ainstall a b c d )
	 FILE( GLOB files ${a}/${b} )
         
         foreach( file ${files} )
  	STRING( REPLACE "${a}" "" fileout ${file} )
         	
    EXECUTE_PROCESS(COMMAND cmake -E copy_if_different ${file} ${d}${fileout} )
    endforeach( file )
endmacro( ainstall )

macro( dinstall a b c d )
	 FILE( GLOB_RECURSE files  ${a}/${b} )
         
         foreach( file ${files} )
  	STRING( REPLACE "${a}" "" fileout ${file} )
         	
    EXECUTE_PROCESS(COMMAND cmake -E copy_if_different ${file} ${d}${fileout} )
    endforeach( file )
endmacro( dinstall )

macro( install_resources target )
  FILE( GLOB_RECURSE RESOURCES RELATIVE ${SACK_SDK_ROOT_PATH}/Resources ${SACK_SDK_ROOT_PATH}/Resources/* )
  foreach( resource ${RESOURCES} )
  	STRING( REPLACE "/" "." resout ${resource} )
        EXECUTE_PROCESS(COMMAND cmake -E copy_if_different ${SACK_SDK_ROOT_PATH}/Resources/${resource} package/assets/${resout} )
  endforeach( resource )
  
endmacro( install_resources )

#EXECUTE_PROCESS(COMMAND cmake -E copy_if_different ${PROJECT_SOURCE_DIR}/${VECTLIB_SOURCES} ${CMAKE_BINARY_DIR}/src/vectlib/vectlib_double.cpp )


install_resources( package/assets )

ainstall( ${SACK_SDK_ROOT_PATH}/lib * DESTINATION package/libs/armeabi )

##########################################
###  Remove unused libraries 
FILE( GLOB files #RELATIVE ${CMAKE_BINARY_DIR} 
		${CMAKE_BINARY_DIR}/package/libs/armeabi/*++.so 
                ${CMAKE_BINARY_DIR}/package/libs/armeabi/*.proxy.*
                ${CMAKE_BINARY_DIR}/package/libs/armeabi/*.module.*
                ${CMAKE_BINARY_DIR}/package/libs/armeabi/*.puregl*
	) 
FILE( REMOVE ${files} ) 

##########################################
###  STRIP LIBRARIES
FILE( GLOB files ${CMAKE_BINARY_DIR}/package/libs/armeabi/*.so 	) 
EXECUTE_PROCESS(COMMAND ${STAGING_DIR}/bin/${ANDROID_NDK_PREBUILT_ARCH_LONG}-strip -s ${files} )


dinstall(  ${CMAKE_SOURCE_DIR}/res * DESTINATION package/res )
ainstall(  ${CMAKE_SOURCE_DIR} sql.config DESTINATION package/assets )
ainstall(  ${CMAKE_SOURCE_DIR} libEditOptions.interface.conf DESTINATION package/assets )
ainstall(  ${CMAKE_SOURCE_DIR} build.xml DESTINATION package )
ainstall(  ${CMAKE_SOURCE_DIR} AndroidManifest.xml DESTINATION package )
ainstall(  ${CMAKE_SOURCE_DIR} strings.xml DESTINATION package/res/values )


EXECUTE_PROCESS(COMMAND cmake -E remove_directory package/src )
message( "EXECUTE_PROCESS(COMMAND cmake -E remove_directory${P_SOURCE_DIR}/package/src )")

set( application ${Project} )
set( activity EditOptions )
STRING( REPLACE "." "/" app_path ${application} )
CONFIGURE_FILE( ${CMAKE_SOURCE_DIR}/../MyNativeActivity.cm package/src/org/d3x0r/${app_path}/${activity}/${activity}.java )


    add_custom_target( package
       COMMAND android.bat update project --target "android-14" --path .
       COMMAND ant.bat ${CMAKE_BUILD_TYPE}
       WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/package )


    add_custom_target( install 
    			COMMAND echo adb install command...
                     COMMAND adb install package/bin/${Project}-${CMAKE_BUILD_TYPE}.apk
 )

    add_custom_target( uninstall 
    			COMMAND echo adb uninstall command...
                     COMMAND adb uninstall org.d3x0r.${Project}
 )

