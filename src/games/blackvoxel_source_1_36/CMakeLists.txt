
set_property(GLOBAL PROPERTY USE_FOLDERS On)
set( CMAKE_INSTALL_MESSAGE "LAZY" )

cmake_minimum_required(VERSION 2.8)

PROJECT( blackvoxel )

set( SDL_SDK_ROOT_PATH M:/SDL2-2.0.3/build/output )
set(SDL_SDK_ROOT_PATH ${SDL_SDK_ROOT_PATH} CACHE STRING "SDL SDK Root" )

if( NOT SACK_SDK_ROOT_PATH )
GET_FILENAME_COMPONENT(SACK_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\SACK;Install_Dir]" ABSOLUTE CACHE)
else( NOT SACK_SDK_ROOT_PATH )
set(SACK_SDK_ROOT_PATH ${SACK_SDK_ROOT_PATH} CACHE STRING "SACK SDK Root" )
endif( NOT SACK_SDK_ROOT_PATH )

include( ${SACK_SDK_ROOT_PATH}/CMakePackage )
INSTALL_SACK( bin )
include_directories( ${SACK_INCLUDE_DIR} )
LINK_DIRECTORIES( ${SACK_LIBRARY_DIR} )

if( 0 )
if( NOT SACK_BINARY_SDK_ROOT_PATH )
GET_FILENAME_COMPONENT(SACK_BINARY_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\Sack Binaries;Install_Dir]" ABSOLUTE CACHE)
else( NOT SACK_BINARY_SDK_ROOT_PATH )
set(SACK_BINARY_SDK_ROOT_PATH ${SACK_BINARY_SDK_ROOT_PATH} CACHE STRING "INTERSHELL SDK Root" )
endif( NOT SACK_BINARY_SDK_ROOT_PATH )

include( ${SACK_BINARY_SDK_ROOT_PATH}/CMakePackage )

if( NOT INTERSHELL_SDK_ROOT_PATH )
GET_FILENAME_COMPONENT(INTERSHELL_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\INTERSHELL;Install_Dir]" ABSOLUTE CACHE)
else( NOT INTERSHELL_SDK_ROOT_PATH )
set(INTERSHELL_SDK_ROOT_PATH ${INTERSHELL_SDK_ROOT_PATH} CACHE STRING "INTERSHELL SDK Root" )
endif( NOT INTERSHELL_SDK_ROOT_PATH )

include( ${INTERSHELL_SDK_ROOT_PATH}/CMakePackage )
INSTALL_INTERSHELL( . )
include_directories( ${INTERSHELL_INCLUDE_DIR} )
LINK_DIRECTORIES( ${INTERSHELL_LIBRARY_DIR} )

if( NOT DEKWARE_SDK_ROOT_PATH )
GET_FILENAME_COMPONENT(DEKWARE_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\DEKWARE;Install_Dir]" ABSOLUTE CACHE)
else( NOT DEKWARE_SDK_ROOT_PATH )
set(DEKWARE_SDK_ROOT_PATH ${DEKWARE_SDK_ROOT_PATH} CACHE STRING "DEKWARE SDK Root" )
endif( NOT DEKWARE_SDK_ROOT_PATH )

include( ${DEKWARE_SDK_ROOT_PATH}/CMakePackage )
INSTALL_DEKWARE( . . )
include_directories( ${DEKWARE_INCLUDE_DIR} )
LINK_DIRECTORIES( ${DEKWARE_LIBRARY_DIR} )

endif( 0 )


set( SOURCES 
src/bmploader.cpp
src/main.cpp
src/Z3DBlit.cpp
src/ZActorPhysics.cpp
src/ZActor_Player.cpp
src/ZEventManager.cpp
src/ZGame.cpp
src/ZGameStat.cpp
src/ZGameWindow_Advertising.cpp
src/ZGameWindow_DisplayInfos.cpp
src/ZGameWindow_Inventory.cpp
src/ZGameWindow_Programmable.cpp
src/ZGameWindow_ProgressBar.cpp
src/ZGameWindow_Sequencer.cpp
src/ZGameWindow_Storage.cpp
src/ZGameWindow_UserTextureTransformer.cpp
src/ZGameWindow_VoxelTypeBar.cpp
src/ZGame_Events.cpp
src/ZGui.cpp
src/ZGui_CheckBox.cpp
src/ZGui_CyclingChoiceBox.cpp
src/ZGui_FontFrame.cpp
src/ZGui_InventoryBox.cpp
src/ZGui_KeyChooser.cpp
src/ZGui_NumericChoiceBox.cpp
src/ZGui_PanelTextButton.cpp
src/ZGui_ProgressBar.cpp
src/ZGui_TileFrame.cpp
src/ZGui_VoxelViewBox.cpp
src/ZHighPerfTimer.cpp
src/ZInclusionProbabilizer.cpp
src/ZInventory.cpp
src/ZLog.cpp
src/ZOs_Specific_Various.cpp
src/ZRender_Basic.cpp
src/ZScreen_ChooseOption.cpp
src/ZScreen_Loading.cpp
src/ZScreen_Main.cpp
src/ZScreen_Message.cpp
src/ZScreen_Options_Display.cpp
src/ZScreen_Options_Gameplay.cpp
src/ZScreen_Options_Keymap.cpp
src/ZScreen_Options_Sound.cpp
src/ZScreen_Saving.cpp
src/ZScreen_SlotSelection.cpp
src/ZScripting_Squirrel3.cpp
src/ZSectorSphere.cpp
src/ZSectorStreamLoader.cpp
src/ZSettings_Hardware.cpp
src/ZSound.cpp
src/ZTextureManager.cpp
src/ZTileSets.cpp
src/ZTools.cpp
src/ZTool_Constructor.cpp
src/ZVoxelExtension_FabMachine.cpp
src/ZVoxelExtension_MiningRobot_xr1.cpp
src/ZvoxelExtension_Programmable.cpp
src/ZVoxelExtension_Sequencer.cpp
src/ZVoxelExtension_Storage.cpp
src/ZVoxelExtension_UserTextureTransformer.cpp
src/ZVoxelGfx_Tree.cpp
src/ZVoxelGfx_Tree_Giant.cpp
src/ZVoxelGfx_Tree_Straight.cpp
src/ZVoxelProcessor.cpp
src/ZVoxelReactor.cpp
src/ZVoxelSector.cpp
src/ZVoxelType.cpp
src/ZVoxelTypeManager.cpp
src/ZVoxelType_BlastFurnace.cpp
src/ZVoxelType_Concrete.cpp
src/ZVoxelType_Egmy_T1.cpp
src/ZVoxelType_FabMachine.cpp
src/ZVoxelType_FusionElement.cpp
src/ZVoxelType_IndicatorLight.cpp
src/ZVoxelType_LightTransmitter.cpp
src/ZVoxelType_MiningRobot_xr1.cpp
src/ZVoxelType_Mover.cpp
src/ZVoxelType_Picker.cpp
src/ZVoxelType_PlaneZ1.cpp
src/ZVoxelType_Programmable.cpp
src/ZVoxelType_Sequencer.cpp
src/ZVoxelType_SimpleButton.cpp
src/ZVoxelType_Storage.cpp
src/ZVoxelType_UserTextureTransformer.cpp
src/ZVoxelType_YellowGas.cpp
src/ZWorld.cpp
src/ZWorldConvert.cpp
src/ZWorldGenesis.cpp
src/ZWorldInfo.cpp
src/ZWorld_Stat.cpp
src/sc_Squirrel3/sqstdlib/sqstdaux.cpp
src/sc_Squirrel3/sqstdlib/sqstdblob.cpp
src/sc_Squirrel3/sqstdlib/sqstdio.cpp
src/sc_Squirrel3/sqstdlib/sqstdmath.cpp
src/sc_Squirrel3/sqstdlib/sqstdrex.cpp
src/sc_Squirrel3/sqstdlib/sqstdstream.cpp
src/sc_Squirrel3/sqstdlib/sqstdstring.cpp
src/sc_Squirrel3/sqstdlib/sqstdsystem.cpp
src/sc_Squirrel3/squirrel/sqapi.cpp
src/sc_Squirrel3/squirrel/sqbaselib.cpp
src/sc_Squirrel3/squirrel/sqclass.cpp
src/sc_Squirrel3/squirrel/sqcompiler.cpp
src/sc_Squirrel3/squirrel/sqdebug.cpp
src/sc_Squirrel3/squirrel/sqfuncstate.cpp
src/sc_Squirrel3/squirrel/sqlexer.cpp
src/sc_Squirrel3/squirrel/sqmem.cpp
src/sc_Squirrel3/squirrel/sqobject.cpp
src/sc_Squirrel3/squirrel/sqstate.cpp
src/sc_Squirrel3/squirrel/sqtable.cpp
src/sc_Squirrel3/squirrel/sqvm.cpp
src/z/ZType_ZMatrix.cpp
src/z/ZBitmapImage.cpp
src/z/ZFastRandom.cpp
src/z/ZGenericCanva.cpp
src/z/ZGenericCanva_2.cpp
src/z/ZLinearInterpolator.cpp
src/z/ZMemPool.cpp
src/z/ZMemPool_Optimized.cpp
src/z/ZObject_Array.cpp
src/z/ZObject_Lists.cpp
src/z/ZSimpleFile.cpp
src/z/ZStream_File.cpp
src/z/ZStream_SpecialRamStream.cpp
src/z/ZString.cpp
src/z/ZBitmapImage.cpp
src/z/ZFastRandom.cpp
src/z/ZGenericCanva.cpp
src/z/ZGenericCanva_2.cpp
src/z/ZLinearInterpolator.cpp
src/z/ZMemPool.cpp
src/z/ZMemPool_Optimized.cpp
src/z/ZObject_Array.cpp
src/z/ZObject_Lists.cpp
src/z/ZSimpleFile.cpp
src/z/ZStream_File.cpp
src/z/ZStream_SpecialRamStream.cpp
src/z/ZString.cpp
)

OPTION( COMPILEOPTION_ALLOWSAVEPLAYERSTATE "Allow Save Player State" ON )
if( COMPILEOPTION_ALLOWSAVEPLAYERSTATE )
add_definitions( -DCOMPILEOPTION_ALLOWSAVEPLAYERSTATE=1 )
endif( COMPILEOPTION_ALLOWSAVEPLAYERSTATE )

include_directories( ${SDL_SDK_ROOT_PATH}/include )
include_directories( src/sc_Squirrel3/include )
LINK_DIRECTORIES( ${SDL_SDK_ROOT_PATH}/lib )
add_executable( ${PROJECT_NAME} WIN32 ${SOURCES} )
target_link_libraries( ${PROJECT_NAME} M:/SDL2-2.0.3/build/output/lib/SDL2.lib squirrel )

install_default_dest( ${PROJECT_NAME} )

target_link_libraries( ${PROJECT_NAME} ${SACK_PLATFORM_LIBRARIES} ${SACK_LIBRARIES} glew imm32 version glu32 )

add_subdirectory( src/sc_Squirrel3 )
