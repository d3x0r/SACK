#include <windows.h>								// Header File For Windows
#include <stdio.h>

#include <gl\gl.h>								// Header File For The OpenGL32 Library
#include <gl\glu.h>								// Header File For The GLu32 Library
#include <gl\glaux.h>								// Header File For The GLaux Library

#include "\common\include\matrix.h"

#include "object.h"
#include "GLFrame.h"

//extern GLFRAME glFrame;

GLfloat	xrot;									// X Rotation
GLfloat	yrot;									// Y Rotation
GLfloat xspeed = 0.3;									// X Rotation Speed
GLfloat yspeed = 1;									// Y Rotation Speed

GLfloat	z=-5.0f;								// Depth Into The Screen

GLfloat LightAmbient[]= { 0.5f, 0.5f, 0.5f, 1.0f }; 				// Ambient Light Values ( NEW )

GLfloat LightDiffuse[]= { 1.0f, 1.0f, 1.0f, 1.0f };				 // Diffuse Light Values ( NEW )

GLfloat LightPosition[]= { 0.0f, 0.0f, 2.0f, 1.0f };				 // Light Position ( NEW )

GLuint	texture[3];								// Storage for 3 textures

AUX_RGBImageRec *LoadBMP(char *Filename)					// Loads A Bitmap Image
{
	FILE *File=NULL;							// File Handle

	if (!Filename)								// Make Sure A Filename Was Given
	{
		return NULL;							// If Not Return NULL
	}

	File=fopen(Filename,"r");						// Check To See If The File Exists

	if (File)								// Does The File Exist?
	{
		fclose(File);							// Close The Handle
//		return auxDIBImageLoad(Filename);				// Load The Bitmap And Return A Pointer
	}

	return NULL;								// If Load Failed Return NULL
}

int LoadGLTextures()								// Load Bitmaps And Convert To Textures
{

	int Status=FALSE;							// Status Indicator

	AUX_RGBImageRec *TextureImage[1];					// Create Storage Space For The Texture

   //TransformClear( &t );
   //TransformTranslate( &t, 3.0, 1.0, z );
   //TransformRotateAbsCoord( &t, -xrot * M_PI/180, -yrot*M_PI/180, 0 );
   //memcpy(m, &t, sizeof( float ) * 16 );

	memset(TextureImage,0,sizeof(void *)*1);				// Set The Pointer To NULL

	if (TextureImage[0]=LoadBMP("Data/Crate.bmp"))
	{
		Status=TRUE;							// Set The Status To TRUE

		glGenTextures(3, &texture[0]);					// Create Three Textures

		// Create Nearest Filtered Texture
		glBindTexture(GL_TEXTURE_2D, texture[0]);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST); 
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST); 
		glTexImage2D(GL_TEXTURE_2D, 0, 3, TextureImage[0]->sizeX, TextureImage[0]->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, TextureImage[0]->data);

		// Create Linear Filtered Texture
		glBindTexture(GL_TEXTURE_2D, texture[1]);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, 3, TextureImage[0]->sizeX, TextureImage[0]->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, TextureImage[0]->data);

		// Create MipMapped Texture
		glBindTexture(GL_TEXTURE_2D, texture[2]);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST); 

		gluBuild2DMipmaps(GL_TEXTURE_2D, 3, TextureImage[0]->sizeX, TextureImage[0]->sizeY, GL_RGB, GL_UNSIGNED_BYTE, TextureImage[0]->data);
	}

	if (TextureImage[0])							// If Texture Exists
	{
		if (TextureImage[0]->data)					// If Texture Image Exists
		{
			free(TextureImage[0]->data);				// Free The Texture Image Memory
		}

		free(TextureImage[0]);						// Free The Image Structure
	}


	return Status;								// Return The Status
}


void SetLighting( void )
{
	glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbient);				// Setup The Ambient Light

	glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuse);				// Setup The Diffuse Light

	glLightfv(GL_LIGHT1, GL_POSITION,LightPosition);			// Position The Light

	glEnable(GL_LIGHT1);							// Enable Light One

}



//----------------------------------------------------------------------

PGLFRAME pglCurrentFrame;

void GLShowObject( POBJECT po )
{
//   PFACET pp;
   PFACETSET pfs;
	int bDrawLines = FALSE;
   int f;
   // for all objects that have been created.
   // draw the object on the screen.
   static TRANSFORM Tmp;

   while( !po->flags.Updated ) 
      Sleep(0);

   // finally rotate into view space.... which is at MASter level...
   TransformClear( &Tmp );
   TransformApplyInverseMatrix( &pglCurrentFrame->T, &Tmp, &po->Ti );

   glLoadMatrixf( (float*)&Tmp );

   pfs = po->pPlaneSet;
   for( f = 0; f < pfs->nUsedPlanes; f++ )
   {
      int p;
      PPOINTSET ppts;
      if( !pfs->pPlanes[f].bUsed )
         continue;
      ppts = pfs->pPlanes[f].Points;
      glBegin( GL_POLYGON );
      for( p = 0; p < ppts->nUsedPoints; p++ )
      {
         glColor3ubv( &ppts->pPoints[p].color );
         glVertex3f( ppts->pPoints[p].p[0]
                   , ppts->pPoints[p].p[1]
                   , ppts->pPoints[p].p[2] );
      }
      glEnd();
   }

#ifdef DRAW_AXIS
   if( bDrawAxis )
   {
       DrawLine( pView->pImage, TransformReturnOrigin(&Tmp), TransformReturnAxis(&Tmp, vRight), 0, 100, 0x7f );
       DrawLine( pView->pImage, TransformReturnOrigin(&Tmp), TransformReturnAxis(&Tmp, vUp), 0, 100, 0x7f00 );
       DrawLine( pView->pImage, TransformReturnOrigin(&Tmp), TransformReturnAxis(&Tmp, vForward), 0, 100, 0x7f0000 );
   }
#endif
}

//----------------------------------------------------------------------

void GLShowObjectChildren( POBJECT po )
{
   POBJECT pc;
	POBJECT pobj;
   extern POBJECT pCurrent;
   if( !po )
      return;
   FORALL( po, &pc )
   {
//      if( pc != pCurrent )
      GLShowObject( pc );
      if( pc->pHolds )
         GLShowObjectChildren( pc->pHolds );
   }
}

//----------------------------------------------------------------------

void GLShowObjects( void )
{
   POBJECT po;
   POBJECT pFirstObject = NULL;
   po = pFirstObject; // some object..........
   while( po && ( po->pIn ) ) // go to TOP of tree...
   {
      if( po->pIn )
         po = po->pIn;
   }
   GLShowObjectChildren( po );  // all children...
}

//----------------------------------------------------------------------

int DrawGLScene(PGLFRAME pglFrame)								// Here's Where We Do All The Drawing
{
   VECTOR o;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);			// Clear The Screen And The Depth Buffer

   pglCurrentFrame = pglFrame;
   GLShowObjects();

   /*
	glLoadIdentity();					// Reset The Current Modelview Matrix
	glTranslatef(0.0f,0.0f,-81.0f);				// Move Right 1.5 Units And Into The Screen 6.0
	glRotatef(xrot,1.0f,0.0f,0.2f);			// Rotate The Quad On The X axis ( NEW )

	glBegin(GL_QUADS);						// Draw A Quad
		glColor3f(0.0f,1.0f,0.0f);			// Set The Color To Blue
		glVertex3f( 1.0f, 1.0f,-1.0f);			// Top Right Of The Quad (Top)
		glVertex3f(-1.0f, 1.0f,-1.0f);			// Top Left Of The Quad (Top)
		glVertex3f(-1.0f, 1.0f, 1.0f);			// Bottom Left Of The Quad (Top)
		glVertex3f( 1.0f, 1.0f, 1.0f);			// Bottom Right Of The Quad (Top)

      glColor3f(1.0f,0.5f,0.0f);			// Set The Color To Orange
		glVertex3f( 1.0f,-1.0f, 1.0f);			// Top Right Of The Quad (Bottom)
		glVertex3f(-1.0f,-1.0f, 1.0f);			// Top Left Of The Quad (Bottom)
		glVertex3f(-1.0f,-1.0f,-1.0f);			// Bottom Left Of The Quad (Bottom)
		glVertex3f( 1.0f,-1.0f,-1.0f);			// Bottom Right Of The Quad (Bottom)

		glColor3f(1.0f,0.0f,0.0f);			// Set The Color To Red
		glVertex3f( 1.0f, 1.0f, 1.0f);			// Top Right Of The Quad (Front)
		glVertex3f(-1.0f, 1.0f, 1.0f);			// Top Left Of The Quad (Front)
		glVertex3f(-1.0f,-1.0f, 1.0f);			// Bottom Left Of The Quad (Front)
		glVertex3f( 1.0f,-1.0f, 1.0f);			// Bottom Right Of The Quad (Front)

		glColor3f(1.0f,1.0f,0.0f);			// Set The Color To Yellow
		glVertex3f( 1.0f,-1.0f,-1.0f);			// Bottom Left Of The Quad (Back)
		glVertex3f(-1.0f,-1.0f,-1.0f);			// Bottom Right Of The Quad (Back)
		glVertex3f(-1.0f, 1.0f,-1.0f);			// Top Right Of The Quad (Back)
		glVertex3f( 1.0f, 1.0f,-1.0f);			// Top Left Of The Quad (Back)

		glColor3f(0.0f,0.0f,1.0f);			// Set The Color To Blue
		glVertex3f(-1.0f, 1.0f, 1.0f);			// Top Right Of The Quad (Left)
		glVertex3f(-1.0f, 1.0f,-1.0f);			// Top Left Of The Quad (Left)
		glVertex3f(-1.0f,-1.0f,-1.0f);			// Bottom Left Of The Quad (Left)
		glVertex3f(-1.0f,-1.0f, 1.0f);			// Bottom Right Of The Quad (Left)

		glColor3f(1.0f,0.0f,1.0f);			// Set The Color To Violet
		glVertex3f( 1.0f, 1.0f,-1.0f);			// Top Right Of The Quad (Right)
		glVertex3f( 1.0f, 1.0f, 1.0f);			// Top Left Of The Quad (Right)
		glVertex3f( 1.0f,-1.0f, 1.0f);			// Bottom Left Of The Quad (Right)
		glVertex3f( 1.0f,-1.0f,-1.0f);			// Bottom Right Of The Quad (Right)

	glEnd();							// Done Drawing The Quad
   

	xrot+=xspeed;								// Add xspeed To xrot
	yrot+=yspeed;								// Add yspeed To yrot
   */

   return TRUE;								// Everything Went OK
}
