#include <psi.h>


class SliderFrame {
public:
	PSI_CONTROL frame;
	PSI_CONTROL sliders[40];
	struct psv_type {
		class SliderFrame *_this;
		int n;
	};

	static void CPROC UpdateSliderVal( PTRSZVAL psv, PSI_CONTROL pc, int val )
	{
		struct psv_type *psv_arg = (struct psv_type*)psv;
		psv_arg->_this->values[psv_arg->n] = val;
		switch( psv_arg->n )
		{
		case 0:
			break;
		}
	}

	static void CPROC SaveColors( PTRSZVAL psv, PSI_CONTROL pc )
	{
		struct psv_type *psv_arg = (struct psv_type*)psv;
		FILE *file = sack_fopen( 0, WIDE("values.dat"), WIDE("wb") );

		if( file )
		{
			fwrite( psv_arg->_this->values, sizeof( psv_arg->_this->values ), 1, file );
			fclose( file );
		}
	}

	static void CPROC LoadColors( PTRSZVAL psv, PSI_CONTROL pc )
	{
		struct psv_type *psv_arg = (struct psv_type*)psv;
		FILE *file = sack_fopen( 0, WIDE("values.dat"), WIDE("rb") );
		if( file )
		{
			fread( psv_arg->_this->values, sizeof( psv_arg->_this->values ), 1, file );
			fclose( file );
		}
		{
			int n;
			for( n = 0; n < 40; n++ )
			{
				SetSliderValues( psv_arg->_this->sliders[n], 0, psv_arg->_this->values[n], 256 );
			}
		}
	}

public:
	float values[40];

	SliderFrame()
	{
		int n;
		struct psv_type *psv_arg;

		frame = CreateFrame( "Light Slider Controls", 0, 0, 1024, 768, 0, NULL );
		for( n = 0; n < 40; n++ )
		{
			psv_arg = New( struct psv_type );
			psv_arg->_this = this;
			psv_arg->n = n;
			sliders[n] = MakeSlider( frame, 5 + 25*n, 5, 20, 420, 1, 0, UpdateSliderVal, (PTRSZVAL)psv_arg );
			SetSliderValues( sliders[n], 0, 128, 256 );
		}
		PSI_CONTROL pc;
		pc = MakeButton( frame, 5, 430, 45, 15, 0, "Save", 0, SaveColors, (PTRSZVAL)psv_arg );
		pc = MakeButton( frame, 55, 430, 45, 15, 0, "Load", 0, LoadColors, (PTRSZVAL)psv_arg );
		DisplayFrame( frame );
	}
};
