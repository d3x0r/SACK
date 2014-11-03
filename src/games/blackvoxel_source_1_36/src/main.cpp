/*
 * This file is part of Blackvoxel.
 *
 * Copyright 2010-2014 Laurent Thiebaut & Olivia Merle
 *
 * Blackvoxel is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Blackvoxel is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
    #include <stdio.h>
    #include <stdlib.h>
    #include <time.h>
    #include <GL/glew.h>
// #include <GL/gl.h>
   // #include <GL/glext.h>
   // #include <GL/glut.h>
    #include <SDL2/SDL.h>
    #include "z/ZTypes.h"
    #include "bmploader.h"

    #include "math.h"

    #include "ZCamera.h"
    #include "ZWorld.h"

#ifndef Z_ZVOXELTYPE_H
#  include "ZVoxelType.h"
#endif

    #include "ZRender_Interface.h"
    #include "ZRender_Basic.h"
    #include "ZRender_Smooth.h"
    #include "ZActorPhysics.h"
    #include "ZActor_Player.h"

#ifndef Z_ZSTRING_H
#  include "z/ZString.h"
#endif

#ifndef Z_ZSOUND_H
#  include "ZSound.h"
#endif

#ifndef Z_ZSECTORSTREAMLOADER_H
#  include "ZSectorStreamLoader.h"
#endif

#ifndef Z_ZVOXELPROCESSOR_H
#  include "ZVoxelProcessor.h"
#endif

#ifndef Z_ZTEXTUREMANAGER_H
#  include "ZTextureManager.h"
#endif

#ifndef Z_ZGUI_H
#  include "ZGui.h"
#endif

#ifndef Z_GUI_FONTFRAME_H
#  include "ZGui_FontFrame.h"
#endif

#ifndef Z_ZTILESETS_H
#  include "ZTileSets.h"
#endif

#ifndef Z_EVENTDISPATCH_H
#  include "ZEventManager.h"
#endif

#ifndef Z_ZGAME_H
#  include "ZGame.h"
#endif

#ifndef Z_ZSCREEN_MAIN_H
#  include "ZScreen_Main.h"
#endif

#ifndef Z_ZSCREEN_SLOTSELECTION_H
#  include "ZScreen_SlotSelection.h"
#endif

#ifndef Z_ZSCREEN_LOADING_H
#  include "ZScreen_Loading.h"
#endif

#ifndef Z_ZSCREEN_SAVING_H
#  include "ZScreen_Saving.h"
#endif

#ifndef Z_ZSCREEN_OPTIONS_H
#  include "ZScreen_Options_Display.h"
#endif

#ifndef Z_ZSCREEN_CHOOSEOPTION_H
#  include "ZScreen_ChooseOption.h"
#endif

#ifndef Z_ZSCREEN_OPTIONS_SOUND_H
#  include "ZScreen_Options_Sound.h"
#endif

#ifndef Z_ZSCREEN_OPTIONS_MOUSE_H
#  include "ZScreen_Options_Gameplay.h"
#endif

#ifndef Z_ZSCREEN_OPTIONS_KEYMAP_H
#  include "ZScreen_Options_Keymap.h"
#endif

#ifndef Z_ZSCREEN_MESSAGE_H
#  include "ZScreen_Message.h"
#endif

#ifndef Z_ZWORLDCONVERT_H
#  include "ZWorldConvert.h"
#endif

#ifndef Z_ZHIGHPERFTIMER_H
#  include "ZHighPerfTimer.h"
#endif

#ifndef Z_ZFASTRANDOM_H
#  include "z/ZFastRandom.h"
#endif

#ifndef A_COMPILESETTINGS_H
#  include "ACompileSettings.h"
#endif

#ifndef Z_ZRANDOM_MAKEGOODNUMBERS_H
#  include "ZRandom_MakeGoodNumbers.h"
#endif

#ifndef Z_ZGENERICCANVA_H
#  include "z/ZGenericCanva.h"
#endif

#ifndef Z_ZOS_SPECIFIC_VARIOUS_H
#  include "ZOs_Specific_Various.h"
#endif

#ifndef Z_ZTEST_PARTS_H
#  include "Z0Test_Parts.h"
#endif

ZGame * Ge;
ZScreen_Main *pSM;
bool *pSM_Continue;
bool *pStartGame;
bool *pGameContinue;

double FrameTime;



static PTRSZVAL OnInit3d( "BlackVoxel" )(PMatrix projection, PTRANSFORM camera, RCOORD *identity_depth, RCOORD *aspect )
{
	return 1;
}

static void OnFirstDraw3d( "BlackVoxel" )( PTRSZVAL psvInit )
{
	int result;
	//result = Init_GraphicMode(InitLog.Sec(1030));        if (!result) return;//(false);
	{
		_32 w, h;
		GetDisplaySize( &w, &h );
		Ge->HardwareResolution.x
			= Ge->ScreenResolution.x = w;
		Ge->HardwareResolution.y 
			= Ge->ScreenResolution.y = h;
		Ge->Initialized_GraphicMode = true;
		Ge->Init_Glew( Ge->InitLog.Sec(1040) );
	    result = Ge->Init_GuiManager(Ge->InitLog.Sec(1090));         if (!result) return;
		result = Ge->Init_TileSetsAndFonts(Ge->InitLog.Sec(1100));   if (!result) return;
    
	}
	result = Ge->Init_Renderer(Ge->InitLog.Sec(1110));           
	//if (!result) return(false);
}

static void OnBeginDraw3d("BlackVoxel")( PTRSZVAL psvInit, PTRANSFORM camera )
{
	int n;
	if( Ge->Basic_Renderer->Camera )
	for( n = 0; n < 16; n++ )
		((float*)camera)[n] = Ge->Basic_Renderer->Camera->orientation.m[0][n];
	//Set Ge->Basic_Renderer->Camera->orientation.quat();
}

			ZScreen_ChooseOption Screen_ChooseOption;
          ZScreen_SlotSelection Screen_SlotSelection;
		          ZScreen_Loading Screen_Loading;
 ZScreen_Options_Display Screen_Options_Display;
 ZScreen_Options_Sound Screen_Options_Sound;    
 ZScreen_Options_Game Screen_Options_Mouse;     
ZScreen_Options_Keymap Screen_Options_Keymap;   

static void OnDraw3d( "BlackVoxel" )( PTRSZVAL psvInit )
{
	ULong Result;
	switch( Ge->page_up )
	{
	case 0:
        Result = pSM->ProcessScreen(Ge);
        switch(Result)
        {
          case ZScreen_Main::CHOICE_QUIT:     // Quit the game

                                              (*pGameContinue) = false;
                                              (*pStartGame) = false;
                                              (*pSM_Continue) = false;
                                              break;

          case ZScreen_Main::CHOICE_OPTIONS:  // Option Section
												Ge->page_up = PAGE_SETTINGS;
                                              break;

          case ZScreen_Main::CHOICE_PLAYGAME: // Play the game
                                              (*pSM_Continue) = false;
											  Ge->page_up = PAGE_SELECT_UNIVERSE;
								  				

                                              break;
        }
		break;
	case PAGE_SETTINGS:
		{
			Screen_ChooseOption.ProcessScreen(Ge);
			switch( Screen_ChooseOption.ResultCode )
			{
			case ZScreen_ChooseOption::CHOICE_QUIT:  { Ge->page_up = PAGE_MAIN_MENU; break; }
			case ZScreen_ChooseOption::CHOICE_DISPLAY:  { Ge->page_up = PAGE_SETTINGS_DISPLAY; break; }
				case ZScreen_ChooseOption::CHOICE_SOUND:    { Ge->page_up = PAGE_SETTINGS_SOUND;   break; }
				case ZScreen_ChooseOption::CHOICE_MOUSE:    {Ge->page_up = PAGE_SETTINGS_MOUSE;  break; }
				case ZScreen_ChooseOption::CHOICE_KEYMAP:  {Ge->page_up = PAGE_SETTINGS_KEYMAP;  break; }
			}
		}
		break;
	case PAGE_SETTINGS_DISPLAY:
		 { Screen_Options_Display.ProcessScreen(Ge); break; }
	case PAGE_SETTINGS_SOUND:
		 { Screen_Options_Sound.ProcessScreen(Ge);   break; }
	case PAGE_SETTINGS_MOUSE:
		 {Screen_Options_Mouse.ProcessScreen(Ge);   break; }
	case PAGE_SETTINGS_KEYMAP:
		{  Screen_Options_Keymap.ProcessScreen(Ge);  break; }
	case PAGE_GAME_WORLD_1:
		if( Ge->prior_page_up != Ge->page_up )
		{
			Ge->prior_page_up = Ge->page_up;
			Ge->GuiManager.RemoveAllFrames();
		}
		break;
	case PAGE_SELECT_UNIVERSE:

          Ge->UniverseNum = Screen_SlotSelection.ProcessScreen(Ge);
		  if( Ge->UniverseNum )
			  Ge->page_up = PAGE_LOADING_SCREEN;
		break;
	case PAGE_LOADING_SCREEN:

          Screen_Loading.ProcessScreen(Ge);
           (*pStartGame) = true;

		break;
	}


	if( Ge->Game_Run  )
	{
		// Rendering
		if( Ge && Ge->Basic_Renderer )
		{
            Ge->GameWindow_Advertising->Advertising_Actions((double)FrameTime);
            Ge->ToolManager->ProcessAndDisplay();
				Ge->Basic_Renderer->Render( true );
		//if( Ge && Ge->Gui )
				Ge->GuiManager.Render();
		}
	}
}

static LOGICAL OnKey3d( "BlackVoxel" )( PTRSZVAL psvInit, _32 key )
{
	bool used = false;
		  ZListItem * Item;
			if( ( KEY_CODE( key ) >= 'A' && KEY_CODE( key )<= 'Z' ) )
				key |= 0x20;
			else if( KEY_CODE(key) >= KEY_F1 && KEY_CODE(key) <= KEY_F12 )
			{
				key += (  SDL_SCANCODE_F1 - KEY_F1);
			}
			if ((Item = Ge->EventManager.ConsumerList.GetFirst()))
			do
			{
				if( key & KEY_PRESSED )
					used = ((ZEventConsumer *)Item->Object)->KeyDown( KEY_CODE( key ) );
				else
					used = ((ZEventConsumer *)Item->Object)->KeyUp( KEY_CODE( key ) );

			} while((Item = Ge->EventManager.ConsumerList.GetNext(Item)));
			return used;
}

static LOGICAL OnMouse3d( "BlackVoxel" )( PTRSZVAL psvInit, PRAY mouse_ray, S_32 MouseX, S_32 MouseY, _32 b ) 
{
	//Ge->EventManager.ConsumerList
	{
		  ZListItem * Item;

		  static int in_mouse;
			static S_32 _x, _y;
			static _32 _b;
			int delx, dely;
			if( in_mouse )
				return 0;
			in_mouse = 1;
			delx = _x - MouseX;
			dely = _y - MouseY;
			_x = MouseX;
			_y = MouseY;
			if ((Item = Ge->EventManager.ConsumerList.GetFirst()))
			do
			{
				if( delx || dely )
				{
					((ZEventConsumer *)Item->Object)->MouseMove(delx, dely, MouseX,MouseY);
				}

				if( b & MK_SCROLL_LEFT )
							((ZEventConsumer *)Item->Object)->MouseButtonClick( 7, MouseX, MouseY);
				if( b & MK_SCROLL_RIGHT )
							((ZEventConsumer *)Item->Object)->MouseButtonClick( 6, MouseX, MouseY);
				if( b & MK_SCROLL_UP )
							((ZEventConsumer *)Item->Object)->MouseButtonClick( 4, MouseX, MouseY);
				if( b & MK_SCROLL_DOWN )
							((ZEventConsumer *)Item->Object)->MouseButtonClick( 5, MouseX, MouseY);


				if( ( b & MK_LBUTTON ) && !( _b & MK_LBUTTON ) )
					((ZEventConsumer *)Item->Object)->MouseButtonClick( 1, MouseX, MouseY );
				if( ( b & MK_RBUTTON ) && !( _b & MK_RBUTTON ) )
					((ZEventConsumer *)Item->Object)->MouseButtonClick( 3, MouseX, MouseY );
				if( ( b & MK_MBUTTON ) && !( _b & MK_MBUTTON ) )
					((ZEventConsumer *)Item->Object)->MouseButtonClick( 2, MouseX, MouseY );

				if( !( b & MK_LBUTTON ) && ( _b & MK_LBUTTON ) )
					((ZEventConsumer *)Item->Object)->MouseButtonRelease( 1, MouseX, MouseY );
				if( !( b & MK_RBUTTON ) && ( _b & MK_RBUTTON ) )
					((ZEventConsumer *)Item->Object)->MouseButtonRelease( 3, MouseX, MouseY );
				if( !( b & MK_MBUTTON ) && ( _b & MK_MBUTTON ) )
					((ZEventConsumer *)Item->Object)->MouseButtonRelease( 2, MouseX, MouseY );

			} while((Item = Ge->EventManager.ConsumerList.GetNext(Item)));
			if( Ge->Mouse_relative )
			{
				SetMousePosition( NULL, _x = Ge->ScreenResolution.x/2, _y = Ge->ScreenResolution.y/2 );
			}
			else
			{
				_x = MouseX;
				_y = MouseY;
			}
			_b = b;
			in_mouse = 0;
	}


	return 0;
}


SaneWinMain( argc, argv )
//int main(int argc, char *argv[])
{
  ULong Result;
  bool StartGame;
  pStartGame = &StartGame;

  #ifdef ZENV_OS_WINDOWS
    Windows_DisplayConsole();
  #endif

  // Start

    printf ("Starting BlackVoxel...\n");

  // Test Code

  #if DEVELOPPEMENT_ON == 1
    //ZTest_Parts TestParts;
    //if (!TestParts.RunTestCode()) exit(0);
  #endif

  // Game main object

    ZGame GameEnv;

    Ge = &GameEnv;

	bool use_external_render = true;
  // Main Game Object Initialisation

    if (!GameEnv.Init()) return(-10);

  // Windows Terminal Fix
  RestoreDisplayEx( NULL DBG_SRC );
  // Debug output for manufacturing compositions

    #if COMPILEOPTION_FABDATABASEOUTPUT == 1
      GameEnv.VoxelTypeManager.OutFabInfos();
      GameEnv.VoxelTypeManager.FindFabConflics();
    #endif

  // Game Menu Loop
	  bool GameContinue;
	  pGameContinue = &GameContinue;

    for ( bool GameContinue = true; GameContinue ; )
    {
      StartGame = false;


      // ***************************************** Main Title Screen ****************************************************

      //SDL_ShowCursor(SDL_ENABLE);
	  GameEnv.Mouse_relative = false;
      SDL_WM_GrabInput(SDL_GRAB_OFF);

      ZScreen_Main Screen_Main;
	  bool ScreenTitle_Continue = true;
	  pSM = &Screen_Main;
	  pSM_Continue = &ScreenTitle_Continue;

	  for ( ScreenTitle_Continue = true; !GameEnv.UniverseNum || !StartGame || ScreenTitle_Continue ; )
      {
        // Display Main Screen and wait user choice;
		  if( use_external_render )
		  {
			  MSG msg;
				if( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE ) )
					DispatchMessage(&msg);
		  }
		  else
		  {
			Result = Screen_Main.ProcessScreen(&GameEnv);

			switch(Result)
			{
			  case ZScreen_Main::CHOICE_QUIT:     // Quit the game

												  GameContinue = false;
												  StartGame = false;
												  ScreenTitle_Continue = false;
												  break;

			  case ZScreen_Main::CHOICE_OPTIONS:  // Option Section
												  {
													ZScreen_ChooseOption Screen_ChooseOption;
													do
													{
													  Screen_ChooseOption.ProcessScreen(&GameEnv);
													  switch( Screen_ChooseOption.ResultCode )
													  {
														case ZScreen_ChooseOption::CHOICE_DISPLAY:  { ZScreen_Options_Display Screen_Options_Display; Screen_Options_Display.ProcessScreen(&GameEnv); break; }
														case ZScreen_ChooseOption::CHOICE_SOUND:    { ZScreen_Options_Sound Screen_Options_Sound;     Screen_Options_Sound.ProcessScreen(&GameEnv);   break; }
														case ZScreen_ChooseOption::CHOICE_MOUSE:    { ZScreen_Options_Game Screen_Options_Mouse;     Screen_Options_Mouse.ProcessScreen(&GameEnv);   break; }
														case ZScreen_ChooseOption::CHOICE_KEYMAP:  { ZScreen_Options_Keymap Screen_Options_Keymap;    Screen_Options_Keymap.ProcessScreen(&GameEnv);  break; }
													  }
													} while (Screen_ChooseOption.ResultCode != ZScreen_ChooseOption::CHOICE_QUIT);
												  }
												  break;

			  case ZScreen_Main::CHOICE_PLAYGAME: // Play the game
												  StartGame = true;
												  ScreenTitle_Continue = false;
												  break;
			}
		  }
      }



      // ****************************************** Entering the game ****************************************************

      if (StartGame)
      {

		  if( !use_external_render )
		  {
        // ****************************** Slot Selection screen ******************************

          ZScreen_SlotSelection Screen_SlotSelection;

          GameEnv.UniverseNum = Screen_SlotSelection.ProcessScreen(&GameEnv);

          // ********************************** Loading Screen ******************************

          ZScreen_Loading Screen_Loading;

          Screen_Loading.ProcessScreen(&GameEnv);

          //     ***************************** THE GAME ******************************
		  }
          // Starting procedure

          if (!GameEnv.Start_Game()) {printf("Start Game Failled\n"); exit(0);}

          // Mouse grab and cursor disable for gaming.

          if (!COMPILEOPTION_NOMOUSECAPTURE)
          {
            SDL_ShowCursor(SDL_DISABLE);
			SDL_WM_GrabInput(SDL_GRAB_ON);
			  GameEnv.Mouse_relative = true;
          }

          // Pre-Gameloop Initialisations.

          FrameTime = 20.0;
          ULong MoveShipCounter = 0;
          GameEnv.Time_FrameTime = 20000;
          ZHighPerfTimer Timer;
          double ReadableDisplayCounter = 0.0;
          GameEnv.GameEventSequencer->SetGameTime(GameEnv.PhysicEngine->GetSelectedActor()->Time_TotalGameTime);


          // *********************************** Main Game Loop **********************************************

          GameEnv.Game_Run = true;
          GameEnv.Time_FrameTime = 20;
		  GameEnv.page_up = PAGE_GAME_WORLD_1;
		  GameEnv.Mouse_relative = true;
          while (GameEnv.Game_Run)
          {
//            Time_Start = SDL_GetTicks();
            Timer.Start();

            // Process Input events (Mouse, Keyboard)

            //GameEnv.EventManager.ProcessEvents();       // Process incoming events.
            GameEnv.Game_Events->Process_StillEvents(); // Process repeating checked events.

            // Process incoming sectors from the make/load working thread

            GameEnv.World->ProcessNewLoadedSectors();

            // if (MoveShipCounter>125 ) {GameEnv.MoveShip(); MoveShipCounter = 0; }

            // Player physics

            GameEnv.PhysicEngine->DoPhysic(GameEnv.Time_FrameTime);

            // Voxel Processor Get Player Position.

            ZActor * Actor;
            Actor = GameEnv.PhysicEngine->GetSelectedActor();
            GameEnv.VoxelProcessor->SetPlayerPosition(Actor->ViewDirection.x(),Actor->ViewDirection.y(),Actor->ViewDirection.z());

            // Sector Ejection processing.

            GameEnv.World->ProcessOldEjectedSectors();


			if( !use_external_render )
			{
            // Advertising messages

            GameEnv.GameWindow_Advertising->Advertising_Actions((double)FrameTime);

            // Tool activation and desactivation

            GameEnv.ToolManager->ProcessAndDisplay();

            // Rendering
				GameEnv.Basic_Renderer->Render( false );
				GameEnv.GuiManager.Render();

				// Swapping OpenGL Surfaces.

				SDL_GL_SwapWindow( GameEnv.screen );
			}
			else
				WakeableSleep( 50 );

            // Game Events.

            GameEnv.GameEventSequencer->ProcessEvents(GameEnv.PhysicEngine->GetSelectedActor()->Time_TotalGameTime);

            // Time Functions

            Timer.End();
            FrameTime = GameEnv.Time_GameLoop = Timer.GetResult() / 1000.0;
            GameEnv.Time_FrameTime = Timer.GetResult();
            GameEnv.Time_GameElapsedTime += GameEnv.Time_FrameTime;
            if (GameEnv.Time_GameLoop > 64.0) GameEnv.Time_GameLoop = 64.0; // Game cannot make too long frames because inaccuracy. In this case, game must slow down.
            GameEnv.GameStat->FrameTime = (ULong) FrameTime;
            GameEnv.GameStat->DoLogRecord();
            MoveShipCounter += FrameTime;

            // Frametime Display;

            ReadableDisplayCounter += FrameTime;
            if (GameEnv.GameWindow_DisplayInfos->Is_Shown() )
            {
              if (ReadableDisplayCounter > 500.0)
              {
                ReadableDisplayCounter = 0.0;
                ZString As;

                As = "FPS: "; As << (ULong)( 1000.0 / FrameTime) << " FTM: " << FrameTime;
                GameEnv.GameWindow_DisplayInfos->SetText(&As);
				As = "Direction: "; 
				As << GameEnv.PhysicEngine->GetSelectedActor()->Camera.orientation.yaw(); 
				As<<  " " ;
				As << GameEnv.PhysicEngine->GetSelectedActor()->Camera.orientation.pitch(); 
				As<<  " " ;
				As << GameEnv.PhysicEngine->GetSelectedActor()->Camera.orientation.roll(); 
				As<<  " " ;
				As << GameEnv.PhysicEngine->GetSelectedActor()->Camera.orientation.m[0][0]; 
				GameEnv.GameWindow_DisplayInfos->SetText2( &As );
              }
            }
          }

          // Display screen "Saving game".

          ZScreen_Saving Screen_Saving;
          Screen_Saving.ProcessScreen(&GameEnv);

          // Save game and cleanup all the mess before returning to title screen

          GameEnv.Basic_Renderer->Cleanup();
          GameEnv.End_Game();
        }

        // Relooping to the title screen
      }

      // Getting out of the game.

#if DEVELOPPEMENT_ON!=1
      ZScreen_Message Screen_Message;
      Screen_Message.ProcessScreen(&GameEnv);
#endif

      GameEnv.End();

      return 0;
    }
	EndSaneWinMain()


