

	PSI_CONTROL frame = LoadXMLFrame( "ConfigureMacroButton.isFrame" );
	if( frame )
	{
		int okay = 0;
		int done = 0;
      SetCommonButtons( frame, &done, &okay );
		DisplayFrame( frame );
		CommonWait( frame );
		if( okay )
		{

		}
      DestroyFrame( &frame );
	}
