#define USES_INTERSHELL_INTERFACE
#define DEFINES_INTERSHELL_INTERFACE
#include <stdhdrs.h>
#include <sackcomm.h>

#include <intershell_export.h>
#include <intershell_registry.h>
#include <widgets/include/banner.h>
#include <psi/knob.h>

#include "USBpulse100Drvr.h"

#define l local_usb_pulse_data

typedef struct usb_pulse_100
{
	TEXTCHAR port[32];
	int com_port;
	TEXTCHAR _product[256];
	TEXTCHAR _serial[256];
	TEXTCHAR *product;
	TEXTCHAR *serial;
	struct USB_PulseFlags
	{
		BIT_FIELD enabled : 1;
		BIT_FIELD inverted : 1;
		BIT_FIELD RNG : 1;
	} flags;
	int level;
} USB_PULSE_100, *PUSB_PULSE_100;

typedef struct key_data
{
	PMENU_BUTTON button;
	PSI_CONTROL slider;
	PSI_CONTROL knob;
	PUSB_PULSE_100  device;
} KEY_DATA, *PKEY_DATA;

static struct local_usb_pulse_data
{
   HANDLE driver;
   PLIST devices;
   struct {
	   int enable;
	   int invert;
	   int RNG;
	   int voltage;
	   int knob;
   } buttons;
}l;

PUBLIC( void, ExportThis )( void )
{
}

ATEXIT( CloseUSBPulse )
{
	USBpulse100Drvr_CloseDrvr( l.driver );
}

PRELOAD( InitUSBPulse100 )
{
	int devices;
	int d;
	l.driver = USBpulse100Drvr_OpenDrvr();
	if( !l.driver )
	{
		lprintf( WIDE("Failed to load lowlevel driver") );
		return;
	}

	USBpulse100Drvr_InitFrequencies();
	devices = USBpulse100Drvr_Enumerate( NULL, NULL );

	if( !devices )
	{
		lprintf( WIDE("No Devices Attached.") );
		//BannerMessage( WIDE("No Devices Attached.") );
		return;
	}
	
	for( d = 0; d < devices; d++ )
	{
		struct usb_pulse_100 *device;
		int len;
		device = New( struct usb_pulse_100 );
		device->com_port = d+1;
		device->product = device->_product;
		device->serial = device->_serial;
		USBpulse100Drvr_GetProductName( d + 1, device->product, &len );
		lprintf( WIDE("device %d = %s"), d, device->product );
		USBpulse100Drvr_GetSerialNumber( d + 1, device->serial, &len );
		lprintf( WIDE("device %d = %s"), d, device->serial );
		AddLink( &l.devices, device );
		{
			TEXTCHAR tmp[256];
			snprintf( tmp, 256, WIDE("<PulseUSB %d/product>"), d + 1 );
			CreateLabelVariable( tmp, LABEL_TYPE_STRING, &device->product );
			snprintf( tmp, 256, WIDE("<PulseUSB %d/serial>"), d + 1 );
			CreateLabelVariable( tmp, LABEL_TYPE_STRING, &device->serial );
		}
	}
#if 0
	{


		PUSB_PULSE_100 device = New( USB_PULSE_100 );
		TEXTCHAR section[32];
		TEXTCHAR buffer[32];
		snprintf( section, 32, "Device %d", d + 1 );
		SACK_GetPrivateProfileString( section, "COM Port", "com1", device->port, sizeof( device->port ), "usb_pulse.ini" );
		device->com_port = SackOpenComm( device->port, 0, 0 );


	}
#endif
}

//---------------------------------------------------------------------------------

OnCreateMenuButton( WIDE("USB Pulse/Enable Output") )( PMENU_BUTTON button )
{
	PKEY_DATA key_data = New( KEY_DATA );
	key_data->button = button;
	key_data->device = GetLink( &l.devices, l.buttons.enable++ );
	return (PTRSZVAL)key_data;
}

OnKeyPressEvent( WIDE("USB Pulse/Enable Output") )( PTRSZVAL psv )
{
	PKEY_DATA key_data = (PKEY_DATA)psv;
	key_data->device->flags.enabled = !key_data->device->flags.enabled;
	USBpulse100Drvr_SetEnable( key_data->device->com_port, key_data->device->flags.enabled );
	UpdateButton( key_data->button );
}

OnShowControl( WIDE("USB Pulse/Enable Output") )( PTRSZVAL psv )
{
	PKEY_DATA key_data = (PKEY_DATA)psv;
	InterShell_SetButtonHighlight( key_data->button, key_data->device->flags.enabled );
}

//---------------------------------------------------------------------------------

OnCreateMenuButton( WIDE("USB Pulse/Invert Output") )( PMENU_BUTTON button )
{
	PKEY_DATA key_data = New( KEY_DATA );
	key_data->button = button;
	key_data->device = GetLink( &l.devices, l.buttons.invert++ );
	return (PTRSZVAL)key_data;
}

OnKeyPressEvent( WIDE("USB Pulse/Invert Output") )( PTRSZVAL psv )
{
	PKEY_DATA key_data = (PKEY_DATA)psv;
	key_data->device->flags.inverted = !key_data->device->flags.inverted;
	USBpulse100Drvr_SetInvert( key_data->device->com_port, key_data->device->flags.inverted );
	UpdateButton( key_data->button );
}

OnShowControl( WIDE("USB Pulse/Invert Output") )( PTRSZVAL psv )
{
	PKEY_DATA key_data = (PKEY_DATA)psv;
	InterShell_SetButtonHighlight( key_data->button, key_data->device->flags.inverted );
}

//---------------------------------------------------------------------------------

OnCreateMenuButton( WIDE("USB Pulse/Enable RNG Output") )( PMENU_BUTTON button )
{
	PKEY_DATA key_data = New( KEY_DATA );
	key_data->button = button;
	key_data->device = GetLink( &l.devices, l.buttons.RNG++ );
	return (PTRSZVAL)key_data;
}

OnKeyPressEvent( WIDE("USB Pulse/Enable RNG Output") )( PTRSZVAL psv )
{
	PKEY_DATA key_data = (PKEY_DATA)psv;
	key_data->device->flags.RNG = !key_data->device->flags.RNG;
	USBpulse100Drvr_SetPRNG( key_data->device->com_port, key_data->device->flags.RNG );
	UpdateButton( key_data->button );
}

OnShowControl( WIDE("USB Pulse/Enable RNG Output") )( PTRSZVAL psv )
{
	PKEY_DATA key_data = (PKEY_DATA)psv;
	InterShell_SetButtonHighlight( key_data->button, key_data->device->flags.RNG );
}

//---------------------------------------------------------------------------------

static void CPROC OnSliderVoltageProc( PTRSZVAL psv, PSI_CONTROL pc, int val )
{
	PKEY_DATA key_data = (PKEY_DATA)psv;
	key_data->device->level = val;
	// back calculates to a scalar 255 value.  There's a 1.5V minimum or something though.
	USBpulse100Drvr_SetAmplitude( key_data->device->com_port, ((float)key_data->device->level) / 100.0f );	
}

OnCreateControl( WIDE("USB Pulse/Voltage Slider") )( PSI_CONTROL frame, S_32 x, S_32 y, _32 w, _32 h )
{
	PKEY_DATA key_data = New( KEY_DATA );
	key_data->slider = MakeNamedControl( frame, SLIDER_CONTROL_NAME, x, y, w, h, -1);
	SetSliderUpdateHandler( key_data->slider, OnSliderVoltageProc, (PTRSZVAL)key_data );     
	SetSliderValues( key_data->slider, 150, 300, 500 );
	key_data->device = GetLink( &l.devices, l.buttons.voltage++ );
	return (PTRSZVAL)key_data;
}

OnGetControl( WIDE("USB Pulse/Voltage Slider") )( PTRSZVAL psv )
{
	return ((PKEY_DATA)psv)->slider;
}

//---------------------------------------------------------------------------------

static void CPROC OnSliderPLLProc( PTRSZVAL psv, PSI_CONTROL pc, int val )
{
	PKEY_DATA key_data = (PKEY_DATA)psv;
	key_data->device->level = val;
	//USBpulse100Drvr_SetPLL( key_data->device->com_port, m, n, u, dly );
	//USBpulse100Drvr_SetPLL( key_data->device->com_port, m, n, u, dly );
}

OnCreateControl( WIDE("USB Pulse/Phase Lock Loop") )( PSI_CONTROL frame, S_32 x, S_32 y, _32 w, _32 h )
{
	PKEY_DATA key_data = New( KEY_DATA );
	key_data->slider = MakeNamedControl( frame, SLIDER_CONTROL_NAME, x, y, w, h, -1);
	SetSliderUpdateHandler( key_data->slider, OnSliderPLLProc, (PTRSZVAL)key_data );     
	SetSliderValues( key_data->slider, 150, 300, 500 );
	key_data->device = GetLink( &l.devices, l.buttons.voltage++ );
	return (PTRSZVAL)key_data;
}

OnGetControl( WIDE("USB Pulse/Phase Lock Loop") )( PTRSZVAL psv )
{
	return ((PKEY_DATA)psv)->slider;
}

//---------------------------------------------------------------------------------

static void CPROC OnFrequencyProc( PTRSZVAL psv, int change )
{
   PKEY_DATA key_data = (PKEY_DATA)psv;
	
}

OnCreateControl( WIDE("USB Pulse/Frequency Knob") )( PSI_CONTROL frame,  S_32 x, S_32 y, _32 w, _32 h )
{
	PKEY_DATA key_data = New( KEY_DATA );
	key_data->knob = MakeNamedControl( frame, CONTROL_SCROLL_KNOB_NAME, x, y, w, h, -1);
	SetScrollKnobEvent( key_data->knob, OnFrequencyProc, (PTRSZVAL)key_data );
	SetScrollKnobImageName( key_data->knob, WIDE("images/knobarrow.png") );
	SetScrollKnobImageZeroAngle( key_data->knob, (0x100000000LL / 2) + (0x100000000LL / 3) );
	key_data->device = GetLink( &l.devices, l.buttons.knob++ );
	return (PTRSZVAL)key_data;
}

OnGetControl( WIDE("USB Pulse/Frequency Knob") )( PTRSZVAL psv )
{
	return ((PKEY_DATA)psv)->knob;
}


//---------------------------------------------------------------------------------

