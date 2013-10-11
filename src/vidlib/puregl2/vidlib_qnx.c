#include <stdhdrs.h>


#ifdef __QNX__
//----------------------------------------------------------------------------
//-------------------- QNX specific display code --------------------------
//----------------------------------------------------------------------------
#include <gf/gf.h>
#include <gf/gf3d.h>

void InitQNXDisplays( void )
{
	int n;
	int nDisplay = 1;
	lprintf( "Init QNX DIsplays" );
	for( n = 0; n < 63; n++ )
	{
		int err = gf_dev_attach( &l.qnx_dev[n], GF_DEVICE_INDEX(n), &l.qnx_dev_info[n] );
		if( err == GF_ERR_DEVICE )
		{
			lprintf( "no device at %d", n );
			break;
		}
		if( err != GF_ERR_OK )
		{
			lprintf( "Error attaching device(%d): %d", n, err );
		}
		else
		{
			int m;
			l.qnx_display[n] = NewArray( gf_display_t, l.qnx_dev_info[n].ndisplays );
			l.qnx_display_info[n] = NewArray( gf_display_info_t, l.qnx_dev_info[n].ndisplays );
			for( m = 0; m < l.qnx_dev_info[n].ndisplays; m++ )
			{
				err = gf_display_attach( &l.qnx_display[n][m],
												l.qnx_dev[n],
												m,
												&l.qnx_display_info[n][m] );

				if( err != GF_ERR_OK )
					lprintf( "Error attaching display(%d,%d): %d", n, m, err );
				else
				{
					lprintf( "Display %d=(%d,%d) has %d layers and is %dx%d", nDisplay, n, m
							 , l.qnx_display_info[n][m].nlayers
							 , l.qnx_display_info[n][m].xres
							 , l.qnx_display_info[n][m].yres );
					nDisplay++;
				}
			}
		}
	}
	l.nDevices = n;
	if( n == 0 )
	{
		lprintf( "No Displays available." );
		DebugBreak();
	}
}



void ShutdownQNXDisplays( void )
{
	int n, m;
	for( n= 0; n < l.nDevices; n++ )
	{
		for( m = 0; m < l.qnx_dev_info[n].ndisplays; m++ )
		{
         gf_display_detach( l.qnx_display[n][m] );
		}
		gf_dev_detach( l.qnx_dev[n] );
	}
	l.nDevices = 0;
}

// takes a 1 based display number and results with device and display index.
void ResolveDeviceID( int nDisplay, int *device, int *display )
{
	int n;
	// try and find display number N and return that display size.
	if( nDisplay )
		nDisplay--; // zero bias this.
	for( n = 0; n < l.nDevices; n++ )
	{
		if( nDisplay >= l.qnx_dev_info[n].ndisplays )
		{
			nDisplay -= l.qnx_dev_info[n].ndisplays;
			continue;
		}
		else
		{
			break;
		}
	}
	if( device )
		(*device)=n;
	if( display )
		(*display)=nDisplay;
}

/**************************************************************************************

Utility Functions to implement...


int gf_display_set_mode( gf_display_t display,
                         int xres,
                         int yres,
                         int refresh,
                         gf_format_t format,
                         unsigned flags );

*******************************************************************************************/

void FindPixelFormat( struct display_camera *camera )
{
	int device, display;
	static EGLint attribute_list[]=
	{
	   EGL_NATIVE_VISUAL_ID, 0,
	   EGL_NATIVE_RENDERABLE, EGL_TRUE,
	   EGL_RED_SIZE, 5,
	   EGL_GREEN_SIZE, 5,
	   EGL_BLUE_SIZE, 5,
	   EGL_DEPTH_SIZE, 16,
	   EGL_NONE
	};
	int i;
	ResolveDeviceID( camera->display, &device, &display );
	if (gf_layer_query(camera->pLayer, l.qnx_display_info[device][display].main_layer_index
				, &camera->layer_info) == GF_ERR_OK) 
	{
		lprintf("found a compatible frame "
				"buffer configuration on layer %d\n", l.qnx_display_info[device][display].main_layer_index);
	}    
	for (i = 0; ; i++) 
	{
		/* Walk through all possible pixel formats for this layer */
		if (gf_layer_query(camera->pLayer, i, &camera->layer_info) != GF_ERR_OK) 
		{
			lprintf("Couldn't find a compatible frame "
					"buffer configuration on layer %d\n", i);
			break;
		}    

		/*
		 * We want the color buffer format to match the layer format,
		 * so request the layer format through EGL_NATIVE_VISUAL_ID.
		 */
		attribute_list[1] = camera->layer_info.format;

		/* Look for a compatible EGL frame buffer configuration */
		if (eglChooseConfig(camera->display
			,attribute_list, &camera->config, 1, &camera->num_config) == EGL_TRUE)
		{
			if (camera->num_config > 0) 
			{
				lprintf( "multiple configs? %d", camera->num_config );
				break;
			}
			else
				lprintf( "Config is no good for eglChooseConfig?" );
		}
		else
			lprintf( "Failed to eglChooseConfig? %d", eglGetError() );
	}
}

void CreateQNXOutputForCamera( struct display_camera *camera )
{

	//layer, surface, ...
	// camera has all information about display number, 
	// it has a hvideo structure ready to populate that the 
	// camera owns.  Just create the real drawing surface
	// and keep handles to enable opengl context for later rendering
	int device,display;
	int err;

	ResolveDeviceID( camera->display, &device, &display );
	lprintf( "create output surface for ..." );

	/* get an EGL display connection */
	camera->displayWindow=eglGetDisplay((EGLNativeDisplayType)l.qnx_display[device][display]);

	if(camera->display == EGL_NO_DISPLAY)
	{
		lprintf("ERROR: eglGetDisplay()\n");
		return;
	}
	else
	{
		lprintf("SUCCESS: eglGetDisplay()\n");
	}

		
		OpenEGL( camera, camera->displayWindow );

	/* initialize the EGL display connection */
	if(eglInitialize(camera->display, NULL, NULL) != EGL_TRUE)
	{
		lprintf( "ERROR: eglInitialize: error 0x%x\n", eglGetError());
		return;
	}

	else
	{
		lprintf( "SUCCESS: eglInitialize()\n");
	};

	err = gf_layer_attach( &camera->pLayer, l.qnx_display[device][display], 0, GF_LAYER_ATTACH_PASSIVE );
	if( err != GF_ERR_OK )
	{
		lprintf( "Error attaching layer(%d): %d", camera->display, err );
		return;
	}
	else
	{
		gf_layer_enable( camera->pLayer );
		//gf_surface_create_layer( &camera->pSurface
		//						, &camera->pLayer, 1, 0
		//						, camera->w, camera->h
		//						, 
	}

	FindPixelFormat( camera );


	/* create a 3D rendering target */
	// the list of surfacs should be 2 if manually created
	// cann pass null so surfaces are automatically created
	// (should remove necessicty to get pixel mode?
	if ( ( err = gf_3d_target_create(&camera->pTarget, camera->pLayer,
		NULL, 0, camera->w, camera->h, camera->layer_info.format) ) != GF_ERR_OK) 
	{
		lprintf("Unable to create rendering target:%d\n",err );
	}



	/* create an EGL window surface */
	camera->surface = eglCreateWindowSurface(camera->display
		, camera->config, camera->pTarget, NULL);

	if (camera->surface == EGL_NO_SURFACE) 
	{
		lprintf("Create surface failed: 0x%x\n", eglGetError());
		return;
	}

	// icing?
	gf_layer_set_src_viewport(camera->pLayer, 0, 0, camera->w-1, camera->h-1);
	gf_layer_set_dst_viewport(camera->pLayer, 0, 0, camera->w-1, camera->h-1);
   
}



ATEXIT( ExitTest )
{
   ShutdownQNXDisplays();
}

void  HostSystem_InitDisplayInfo( void )
{
   InitQNXDisplays();
}


#endif


void  GetDisplaySizeEx ( int nDisplay
												 , S_32 *x, S_32 *y
												 , _32 *width, _32 *height)
{
		if( nDisplay > 0 )
		{
			int n, m;
			ResolveDeviceID( nDisplay, &n, &m );
				// try and find display number N and return that display size.
			if( x )
				(*x) = 0;
			if( y )
				(*y) = 0;
			if( width )
				(*width) = l.qnx_display_info[n][m].xres;
			if( height )
				(*height) = l.qnx_display_info[n][m].yres;
		}
		else
		{
			// if any displays, otherwise, arrays are unallocated.
			// application should set insane source values which remain
			// unchanged.
			if( l.nDisplays )
			{
				if( x )
					(*x) = 0;
				if( y )
					(*y) = 0;
				if( width )
					(*width) = l.qnx_display_info[0][0].xres;
				if( height )
					(*height) = l.qnx_display_info[0][0].yres;
			}
			// return default display size
		}
}

