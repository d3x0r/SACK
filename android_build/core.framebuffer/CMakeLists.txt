cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0037 OLD)



set( Project core.framebuffer )
PROJECT( ${Project} NONE )

if( NOT SACK_SDK_ROOT_PATH )
GET_FILENAME_COMPONENT(SACK_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\SACK;Install_Dir]" ABSOLUTE CACHE)
else( NOT SACK_SDK_ROOT_PATH )
set(SACK_SDK_ROOT_PATH ${SACK_SDK_ROOT_PATH} CACHE STRING "SACK SDK Root" )
endif( NOT SACK_SDK_ROOT_PATH )

add_custom_target( update
       WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)


macro( ainstall a b c d )
	 FILE( GLOB files ${a}/${b} )
         
         foreach( file ${files} )
         	STRING( REPLACE "${a}" "" fileout ${file} )
                
	  	if( NOT IS_DIRECTORY ${file} )
			add_custom_command( TARGET update PRE_BUILD  COMMAND cmake -E copy_if_different ${file} ${d}${fileout} )
                        list(APPEND ${c} ${d}${fileout} ) 
	  	endif( NOT IS_DIRECTORY ${file} )
	endforeach( file )
endmacro( ainstall )

macro( dinstall a b c d )
	 FILE( GLOB_RECURSE files  ${a}/${b} )
         
         foreach( file ${files} )
  	STRING( REPLACE "${a}" "" fileout ${file} )
         	
    add_custom_command( TARGET update PRE_BUILD  COMMAND cmake -E copy_if_different ${file} ${d}${fileout} )
    endforeach( file )
endmacro( dinstall )

macro( install_resources target )
  FILE( GLOB_RECURSE RESOURCES RELATIVE ${SACK_SDK_ROOT_PATH}/Resources ${SACK_SDK_ROOT_PATH}/Resources/* )
  foreach( resource ${RESOURCES} )
  	STRING( REPLACE "/" "." resout ${resource} )
        add_custom_command( TARGET update PRE_BUILD  COMMAND cmake -E copy_if_different ${SACK_SDK_ROOT_PATH}/Resources/${resource} package/assets/${resout} )
  endforeach( resource )
        add_custom_command( TARGET update PRE_BUILD  COMMAND cmake -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/../../data/MyriadPro.ttf package/assets/fonts~MyriadPro.ttf )
  
endmacro( install_resources )



install_resources( package/assets )


ainstall( ${SACK_SDK_ROOT_PATH}/lib * INSTALL_LIST package/libs/armeabi )

##########################################
###  Remove unused libraries 

        
#       message( "LIST IS NOW:${INSTALL_LIST}" )
foreach( installed_file ${INSTALL_LIST} )
	set( COPY 1 )                
	foreach( mask   \\+\\+\\.  proxy  module puregl libsw libav libopenal )
        	if( installed_file MATCHES ${mask} )
                   ##message( "file ${installed_file} matches ${mask}" )
                	set( COPY 0 )
        	endif( installed_file MATCHES ${mask} )
	endforeach( mask )                
        if( COPY )
        	LIST( APPEND FILTERED_INSTALL_LIST ${installed_file} )
        else( COPY )
        	LIST( APPEND DELETION_LIST ${installed_file} )
        endif( COPY )
                
endforeach( installed_file )
    #message( "LIST IS NOW:${FILTERED_INSTALL_LIST}" )
    #message( "DEL_LIST IS NOW:${DELETION_LIST}" )
if( files )
	add_custom_command( TARGET update PRE_BUILD COMMAND rm ${DELETION_LIST} ) 
endif( files )

##########################################
###  STRIP LIBRARIES
#FILE( GLOB files ${CMAKE_BINARY_DIR}/package/libs/armeabi/*.so 	) 

add_custom_command( TARGET update PRE_BUILD  COMMAND ${STAGING_DIR}/bin/${ANDROID_NDK_PREBUILT_ARCH_LONG}-strip -s ${FILTERED_INSTALL_LIST} )



dinstall(  ${CMAKE_SOURCE_DIR}/res * DESTINATION package/res )
ainstall(  ${CMAKE_SOURCE_DIR} sql.config DESTINATION package/assets )
ainstall(  ${CMAKE_SOURCE_DIR} libEditOptions.interface.conf DESTINATION package/assets )
ainstall(  ${CMAKE_SOURCE_DIR} build.xml DESTINATION package )
ainstall(  ${CMAKE_SOURCE_DIR} AndroidManifest.xml DESTINATION package )
ainstall(  ${CMAKE_SOURCE_DIR} strings.xml DESTINATION package/res/values )


add_custom_command( TARGET update PRE_BUILD  COMMAND cmake -E remove_directory package/src )

set( application ${Project} )
set( activity EditOptions )
STRING( REPLACE "." "/" app_path ${application} )
FILE( WRITE ${CMAKE_BINARY_DIR}/SupidConfigure.cmake "configure_file( ${CMAKE_SOURCE_DIR}/../MyNativeActivity.cm package/src/org/d3x0r/${app_path}/${activity}/${activity}.java )\n" )
FILE( APPEND ${CMAKE_BINARY_DIR}/SupidConfigure.cmake "EXECUTE_PROCESS(COMMAND cmake -E copy_if_different ${CMAKE_SOURCE_DIR}/../NativeStaticLib.java package/src/org/d3x0r/sack/NativeStaticLib.java )\n" )
EXECUTE_PROCESS(COMMAND cmake -E copy_if_different ${CMAKE_SOURCE_DIR}/../NativeStaticLib.java package/src/org/d3x0r/sack/NativeStaticLib.java )

add_custom_command( TARGET update PRE_BUILD COMMAND  cmake -Dapplication=${Project} -Dactivity=${activity} -P ${CMAKE_BINARY_DIR}/SupidConfigure.cmake )


if( CMAKE_BUILD_TYPE STREQUAL "release" )
  set( MORE_COMMANDS 
       COMMAND jarsigner -verbose -sigalg SHA1withRSA -digestalg SHA1 -keystore ${CMAKE_CURRENT_SOURCE_DIR}/../dekware/my-release-key.keystore bin/${Project}-${CMAKE_BUILD_TYPE}-unsigned.apk alias_name
       COMMAND rm -f bin/${Project}-${CMAKE_BUILD_TYPE}.apk
       COMMAND zipalign -v 4 bin/${Project}-${CMAKE_BUILD_TYPE}-unsigned.apk bin/${Project}-${CMAKE_BUILD_TYPE}.apk
     )
endif( CMAKE_BUILD_TYPE STREQUAL "release" )

    add_custom_target( package
       COMMAND android.bat update project --target "android-14" --path .
       COMMAND ant.bat ${CMAKE_BUILD_TYPE}
       ${MORE_COMMANDS}
       WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/package )


    add_custom_target( install 
    			COMMAND echo adb install command...
                     COMMAND adb install package/bin/${Project}-${CMAKE_BUILD_TYPE}.apk
 )

    add_custom_target( uninstall 
    			COMMAND echo adb uninstall command...
                     COMMAND adb uninstall org.d3x0r.sack.${Project}
 )

