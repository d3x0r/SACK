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
 * ZTileSets.cpp
 *
 *  Created on: 7 mai 2011
 *      Author: laurent
 */

#include "ZTileSets.h"
#include <GL/glew.h>
#include <math.h>
#include <stdio.h>
#include "SDL2/SDL.h"

    ZTileSet::ZTileSet()
    {
      CoordTable = new TileCoord[256];
      TextureNum = 0;

      Texture_Width  = 128;
      Texture_Height = 128;
      TileSlot_Width = 8;
      TileSlot_Height= 8;
      Tile_Width     = 8;
      Tile_Height    = 8;
      TileOffset_x   = 0;
      TileOffset_y   = 0;
      TilesPerLine   = 16;
      DefaultDrawColor.r = 1.0f;
      DefaultDrawColor.v = 1.0f;
      DefaultDrawColor.b = 1.0f;
    }

    ZTileSet::~ZTileSet()
    {
      if (CoordTable) delete [] CoordTable;
    }

    void ZTileSet::SetTextureSize(ULong Width, ULong Height)  {Texture_Width = Width; Texture_Height = Height;}

    void ZTileSet::SetTileSlotSize(float Width, float Height) {TileSlot_Width = Width; TileSlot_Height = Height;}

    void ZTileSet::SetTileSize(float Width, float Height)     {Tile_Width = Width, Tile_Height = Height;}

    void ZTileSet::SetTilesPerLine(ULong TilesPerLine)        {this->TilesPerLine = TilesPerLine;}

    void  ZTileSet::SetTileOffset(ULong x, ULong y)           { TileOffset_x = x; TileOffset_y = y; }


    void ZTileSet::ComputeTileCoords()
    {
      ULong i;
	  float x1,y1,x2,y2;

      for (i=0;i<256;i++)
      {
        x1 = (i % TilesPerLine) * TileSlot_Width  + TileOffset_x;
        y1 = (i / TilesPerLine) * TileSlot_Height + TileOffset_y;
        x2 = x1 + TileSlot_Width;
        y2 = y1 + TileSlot_Height;

        CoordTable[i].TopLeft_x     = (float)x1 / (float)Texture_Width;
        CoordTable[i].TopLeft_y     = (float)y1 / (float)Texture_Height;
        CoordTable[i].BottomRight_x = (float)x2 / (float)Texture_Width;
        CoordTable[i].BottomRight_y = (float)y2 / (float)Texture_Height;
        CoordTable[i].Tile_Width = (float)Tile_Width;
        CoordTable[i].Tile_Height= (float)Tile_Height;
      }
    }

    ULong ZTileSet::GetTilePixel(UByte TileNum, ULong x, ULong y)
    {
      ZBitmapImage * Image;
      ULong Image_x, Image_y;

      Image = TextureManager->GetTextureEntry(TextureNum)->Texture;

      Image_x = x + (TileNum % TilesPerLine) * TileSlot_Width;
      Image_y = y + (TileNum / TilesPerLine) * TileSlot_Height;

      return(Image->GetPixel(Image_x,Image_y));
    }

    void ZTileSet::RenderTile( ZRender_Interface *render, uintptr_t psvInit, ZVector3f * TopLeft, ZVector3f * BottomRight, UByte TileNum, ZColor3f * Color)
    {
      TileCoord * Coord;

      Coord = &CoordTable[TileNum];

      if(!Color) Color = &DefaultDrawColor;
	  ULong textureRef = TextureManager->GetTextureEntry(TextureNum)->OpenGl_TextureRef[psvInit];
      //glBindTexture(GL_TEXTURE_2D,TextureManager->GetTextureEntry(TextureNum)->OpenGl_TextureRef[psvInit] );
      glColor3f(Color->r, Color->v, Color->b);
      glBegin(GL_TRIANGLES);
       glTexCoord2f(Coord->TopLeft_x      , Coord->TopLeft_y     ); glVertex3f(TopLeft->x    , TopLeft->y , TopLeft->z);
        glTexCoord2f(Coord->BottomRight_x , Coord->TopLeft_y    ); glVertex3f(BottomRight->x, TopLeft->y , TopLeft->z);
        glTexCoord2f(Coord->BottomRight_x , Coord->BottomRight_y); glVertex3f(BottomRight->x, BottomRight->y , TopLeft->z);
        glTexCoord2f(Coord->BottomRight_x , Coord->BottomRight_y); glVertex3f(BottomRight->x, BottomRight->y , BottomRight->z);
        glTexCoord2f(Coord->TopLeft_x     , Coord->BottomRight_y); glVertex3f(TopLeft->x    , BottomRight->y , BottomRight->z);
        glTexCoord2f(Coord->TopLeft_x     , Coord->TopLeft_y    ); glVertex3f(TopLeft->x    , TopLeft->y     , BottomRight->z);
      glEnd();
    }


    void ZTileSet::RenderFont(ZRender_Interface *render, uintptr_t psvInit, ZTileStyle const * TileStyle , ZBox3f const * DrawBox, char const * TextToRender, ZColor3f * Color=0)
    {
      float x,y, xp,yp, DimX, DimY, LimitX;// LimitY;
      ZColor3f DrawColor;

      if (Color == 0) { Color = & DrawColor; DrawColor.r = 1.0; DrawColor.v = 1.0; DrawColor.b = 1.0; }

      ULong i;
      ZTileSet * TileSet;
      TileCoord * Coord;
      UByte c;

      TileSet = TileStyle->TileSet;
      x = DrawBox->Position_x;
      y = DrawBox->Position_y;
      LimitX = x + DrawBox->Width ;
      //LimitY = y + DrawBox->Height;

      //glBindTexture(GL_TEXTURE_2D,TextureManager->GetTextureEntry(TileSet->TextureNum)->OpenGl_TextureRef[psvInit] );
      //glColor3f(Color->r, Color->v, Color->b);
      for (i=0; (c = (UByte)(TextToRender[i])) ;i++)
      {
        Coord = &TileSet->CoordTable[c];
        DimX = Coord->Tile_Width * TileStyle->HSizeFactor;
        DimY = Coord->Tile_Height* TileStyle->VSizeFactor;
		//lprintf( "%g %g", TileStyle->HSizeFactor, TileStyle->VSizeFactor );
        xp = x + DimX;
        yp = y + DimY;
        if (xp > LimitX)
        {
          x = DrawBox->Position_x;
          y+= DimY + TileStyle->Interligne_sup;
          xp = x + DimX;
          yp = y + DimY;
        }
		{
		ULong TextureRef = TextureManager->GetTextureEntry(TileSet->TextureNum)->OpenGl_TextureRef[psvInit];
		float coords[18];
		float tex_coords[12];
		tex_coords[0] = Coord->TopLeft_x; tex_coords[1] = Coord->TopLeft_y;   coords[0] = x; coords[1] = y; coords[2] = DrawBox->Position_z; 
		tex_coords[2] = Coord->BottomRight_x; tex_coords[3] = Coord->TopLeft_y;  coords[3] = xp; coords[4] = y; coords[5] = DrawBox->Position_z; 
		tex_coords[4] = Coord->BottomRight_x; tex_coords[5] = Coord->BottomRight_y; coords[6] = xp; coords[7] = yp; coords[8] = DrawBox->Position_z; 
		tex_coords[6] = Coord->BottomRight_x; tex_coords[7] = Coord->BottomRight_y; coords[9] = xp; coords[10] = yp; coords[11] = DrawBox->Position_z; 
		tex_coords[8] = Coord->TopLeft_x; tex_coords[9] = Coord->BottomRight_y;  coords[12] = x; coords[13] = yp; coords[14] = DrawBox->Position_z; 
		tex_coords[10] = Coord->TopLeft_x; tex_coords[11] = Coord->TopLeft_y; coords[15] = x; coords[16] = y; coords[17] = DrawBox->Position_z; 
		//	lprintf( "pos %3d %c %g,%g,%g %g,%g,%g  %gx%g   %gx%g", TextureRef, c, x*1920.0, y*1080.0, 0.0, xp*1920.0, yp*1080.0, DrawBox->Position_z, (xp - x ) * 1920, ( yp-y) * 1080, (Coord->BottomRight_x -Coord->TopLeft_x) *Texture_Width, (Coord->BottomRight_y -Coord->TopLeft_y) *Texture_Height  );
		//	lprintf( "tex %3d %g,%g,%g %g,%g,%g", TextureRef, Coord->TopLeft_x, Coord->TopLeft_y, 0.0, Coord->BottomRight_x, Coord->BottomRight_y, 0.0 );
		  render->gui_shader->Draw( TextureRef, &Color->r, coords, tex_coords );
		}
#if 0
        glBegin(GL_TRIANGLES);
          glTexCoord2f(Coord->TopLeft_x     , Coord->TopLeft_y    ); glVertex3f(x , y , DrawBox->Position_z);
          glTexCoord2f(Coord->BottomRight_x , Coord->TopLeft_y    ); glVertex3f(xp, y , DrawBox->Position_z);
          glTexCoord2f(Coord->BottomRight_x , Coord->BottomRight_y); glVertex3f(xp, yp, DrawBox->Position_z);
          glTexCoord2f(Coord->BottomRight_x , Coord->BottomRight_y); glVertex3f(xp, yp, DrawBox->Position_z);
          glTexCoord2f(Coord->TopLeft_x     , Coord->BottomRight_y    ); glVertex3f(x , yp, DrawBox->Position_z);
          glTexCoord2f(Coord->TopLeft_x     , Coord->TopLeft_y    ); glVertex3f(x , y , DrawBox->Position_z);
        glEnd();
#endif
        x += DimX + TileStyle->CharSpacing_Sup;

      }
      //glColor3f(1.0,1.0,1.0);
    }


    void  ZTileSet::GetFontRenderSize(ZTileStyle const * TileStyle , char const * TextToRender, ZVector2f * OutSize)
    {
      float x,y, DimX, DimY;

      ULong i;
      ZTileSet * TileSet;
      TileCoord * Coord;
      UByte c;

      TileSet = TileStyle->TileSet;
      x = 0;
      y = 0;

      for (i=0; (c = (UByte)(TextToRender[i])) ;i++)
      {
        Coord = &TileSet->CoordTable[c];
        DimX = Coord->Tile_Width * TileStyle->HSizeFactor;
        DimY = Coord->Tile_Height* TileStyle->VSizeFactor;
        x += DimX + TileStyle->CharSpacing_Sup;
        if (DimY>y) y = DimY;
      }
      OutSize->x = x;
      OutSize->y = y;
    }

