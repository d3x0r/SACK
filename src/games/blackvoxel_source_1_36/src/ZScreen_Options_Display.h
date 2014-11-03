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
/*
 * ZScreen_Options.h
 *
 *  Created on: 30 mai 2011
 *      Author: laurent
 */

#ifndef Z_ZSCREEN_OPTIONS_H
#define Z_ZSCREEN_OPTIONS_H

//#ifndef Z_ZSCREEN_OPTIONS_H
//#  include "ZScreen_Options_Display.h"
//#endif

#ifndef Z_ZGUI_H
#  include "ZGui.h"
#endif

#ifndef Z_GUI_FONTFRAME_H
#  include "ZGui_FontFrame.h"
#endif

#ifndef Z_ZGUI_TILEFRAME_H
#  include "ZGui_TileFrame.h"
#endif

#ifndef Z_ZGUI_CYCLINGCHOICEBOX_H
#  include "ZGui_CyclingChoiceBox.h"
#endif

#ifndef ZGUI_ZNUMERICCHOICEBOX_H
#  include "ZGui_NumericChoiceBox.h"
#endif

#ifndef Z_ZGUI_CHECKBOX_H
#  include "ZGui_CheckBox.h"
#endif

#ifndef Z_ZGAME_H
#  include "ZGame.h"
#endif

class ZScreen_Options_Display : public ZScreen
{
    ZVector2f Size;
  ZVector2f Pos;
  ULong i;
  ULong Count;
  bool Found;

  ZObjectArray Resolution_Array;
protected:
    class ZResolution : public ZObject
    {
      public:
        ZString Name;
        ULong   Resolution_x;
        ULong   Resolution_y;
    };

    static int ResolutionCompare(ZObject * Object_1, ZObject * Object_2);
private:
		ZResolution *Res;

  public:
    enum { CHOICE_QUIT, CHOICE_OPTIONS, CHOICE_PLAYGAME};
	void LateInit()
	//ZScreen_Options_Display()
	{
		  Res = new ZResolution; Res->Resolution_x = 0; Res->Resolution_y = 0; Res->Name = "Automatic"; Resolution_Array.Add(*Res);
		  Res = new ZResolution; Res->Resolution_x = 800; Res->Resolution_y = 600; Res->Name = "800*600"; Resolution_Array.Add(*Res);
		  Res = new ZResolution; Res->Resolution_x = 1024; Res->Resolution_y = 600; Res->Name = "1024*600"; Resolution_Array.Add(*Res);
		  Res = new ZResolution; Res->Resolution_x = 1024; Res->Resolution_y = 768; Res->Name = "1024*768"; Resolution_Array.Add(*Res);
		  Res = new ZResolution; Res->Resolution_x = 1152; Res->Resolution_y = 864; Res->Name = "1152*864"; Resolution_Array.Add(*Res);
		  Res = new ZResolution; Res->Resolution_x = 1280; Res->Resolution_y = 768; Res->Name = "1280*768"; Resolution_Array.Add(*Res);
		  Res = new ZResolution; Res->Resolution_x = 1280; Res->Resolution_y = 800; Res->Name = "1280*800"; Resolution_Array.Add(*Res);
		  Res = new ZResolution; Res->Resolution_x = 1280; Res->Resolution_y = 960; Res->Name = "1280*960"; Resolution_Array.Add(*Res);
		  Res = new ZResolution; Res->Resolution_x = 1280; Res->Resolution_y = 1024; Res->Name = "1280*1024"; Resolution_Array.Add(*Res);
		  Res = new ZResolution; Res->Resolution_x = 1360; Res->Resolution_y = 768; Res->Name = "1360*768"; Resolution_Array.Add(*Res);
		  Res = new ZResolution; Res->Resolution_x = 1366; Res->Resolution_y = 768; Res->Name = "1366*768"; Resolution_Array.Add(*Res);
		  Res = new ZResolution; Res->Resolution_x = 1440; Res->Resolution_y = 900; Res->Name = "1440*900"; Resolution_Array.Add(*Res);
		  Res = new ZResolution; Res->Resolution_x = 1600; Res->Resolution_y = 900; Res->Name = "1600*900"; Resolution_Array.Add(*Res);
		  Res = new ZResolution; Res->Resolution_x = 1600; Res->Resolution_y = 1200; Res->Name = "1600*1200"; Resolution_Array.Add(*Res);
		  Res = new ZResolution; Res->Resolution_x = 1680; Res->Resolution_y = 1050; Res->Name = "1680*1050"; Resolution_Array.Add(*Res);
		  Res = new ZResolution; Res->Resolution_x = 1800; Res->Resolution_y = 1440; Res->Name = "1800*1440"; Resolution_Array.Add(*Res);
		  Res = new ZResolution; Res->Resolution_x = 1920; Res->Resolution_y = 1080; Res->Name = "1920*1080"; Resolution_Array.Add(*Res);
		  Res = new ZResolution; Res->Resolution_x = 1920; Res->Resolution_y = 1200; Res->Name = "1920*1200"; Resolution_Array.Add(*Res);
		  Res = new ZResolution; Res->Resolution_x = 640; Res->Resolution_y = 480; Res->Name = "640*480"; Resolution_Array.Add(*Res);
	}
    virtual ULong ProcessScreen(ZGame * GameEnv);

};


#endif /* Z_ZSCREEN_OPTIONS_H */
