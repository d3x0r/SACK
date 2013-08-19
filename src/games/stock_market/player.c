#include <stdhdrs.h>
#include <sharemem.h>

#include "global.h"

#include "board.h"
#include "stocks.h"
#include "input.h"

#include <psi.h>
extern CONTROL_REGISTRATION player_token;

#define MAX_ARCHTYPES ( sizeof( archtypes ) / sizeof( ARCHTYPE ) )

ARCHTYPE archtypes[] = { { "red", 0, { 0 } }
                       , { "blue", 0, { 0 } }
                       , { "green", 0, { 0 } }
                       , { "yellow", 0, { 0 } }
                       , { "orange", 0, { 0 } }
                       , { "black", 0, { 0 } }
                       , { "white" , 0, { 0 } }
                       , { "purple", 0, { 0 } }
                       };

enum player_controls {
	TXT_PLAYER = 100
							, TXT_VALUE
							, TXT_CASH
                     , CST_PORTFOLIO

};

void UpdatePlayerDialog( void )
{
	char txt[64];
	SetCommonText( GetControl( g.Player, TXT_PLAYER ), g.pCurrentPlayer->name );
	snprintf( txt, sizeof( txt ), "$%ld", g.pCurrentPlayer->Cash );
	SetCommonText( GetControl( g.Player, TXT_CASH ), txt );
	snprintf( txt, sizeof( txt ), "$%ld", g.pCurrentPlayer->NetValue );
	SetCommonText( GetControl( g.Player, TXT_VALUE ), txt );
	UpdateCommon( g.Player );
}

int CPROC DrawPlayerToken( PCOMMON pControl )
{
	ValidatedControlData( PPLAYER*, player_token.TypeID, ppPlayer, pControl );
	PPLAYER pPlayer = (*ppPlayer);
	//PPLAYER pPlayer = *ppPlayer;
	Image Surface = GetControlSurface( pControl );
	ClearImageTo( Surface, pPlayer->archtype->color );
	if( g.pCurrentPlayer == pPlayer )
	{
		CDATA Border;
		if( g.flags.bFlashOn )
			Border = Color( 255, 0, 0 );
		else
			Border = Color( 255, 255, 255 );
		do_hline( Surface, 0, 0, Surface->width, Border );
		do_hline( Surface, 1, 0, Surface->width, Border );
		do_hline( Surface, Surface->height-1, 0, Surface->width, Border );
		do_hline( Surface, Surface->height-2, 0, Surface->width, Border );
		do_vline( Surface, 0, 2, Surface->height-3, Border );
		do_vline( Surface, 1, 2, Surface->height-3, Border );
		do_vline( Surface, Surface->width-1, 2, Surface->height-3, Border );
		do_vline( Surface, Surface->width-2, 2, Surface->height-3, Border );
	}
	return TRUE;
}

int CPROC PlayerMouse( PCOMMON pc, S_32 x, S_32 y, _32 b )
{
	ValidatedControlData( PPLAYER*, player_token.TypeID, ppPlayer, pc );
	PPLAYER pPlayer = (*ppPlayer);
	static _32 _b;
	if( g.flags.bSelectPlayer && (b & MK_LBUTTON) && !(_b & MK_LBUTTON ) )
	{
		INDEX idx;
		PPLAYER pCheck;
		LIST_FORALL( g.PossiblePlayers, idx, PPLAYER, pCheck )
		{
			if( pCheck == pPlayer )
			{
				char select[4];
				snprintf( select, sizeof( select ), "%ld%s"
						 , idx + 1
						 , g.flags.bChoiceNeedsEnter?"\n":"" );
				EnqueStrokes( select );
				break;
			}
		}
	}
	_b = b;
	return 1;
}

int CPROC InitToken( PCOMMON pc, POINTER userdata )
{
	ValidatedControlData( PPLAYER*, player_token.TypeID, ppPlayer, pc );
	if( ppPlayer )
	{
		(*ppPlayer) = (PPLAYER)userdata;
	}
	return TRUE;
}

CONTROL_REGISTRATION player_token = { "StockMarket Player Token"
												, { { 16, 16 }, sizeof( PLAYER ), BORDER_NONE }
// this is a deliberate abuse.  It is created iwth MakeControlparam which passes a different init routine signature
												, (int(CPROC*)(PCOMMON))InitToken
												, NULL
												, DrawPlayerToken
												, PlayerMouse
};
PRELOAD( RegsiterPlayerToken )
{
	DoRegisterControl( &player_token );
}

void CreatePlayerToken( PPLAYER pPlayer )
{
	pPlayer->pPlayerToken = MakeControlParam( NULL
												  , player_token.TypeID
												  , 0, 0
												  , 16, 16
												  , 0, pPlayer );
	//SetControlDraw( pPlayer->pPlayerToken, DrawPlayerToken, (PTRSZVAL)pPlayer );
   //SetControlMouse( pPlayer->pPlayerToken, PlayerMouse, (PTRSZVAL)pPlayer );
}

void AllowCurrentPlayerSell( void )
{
   _32 cash = 0;
	// do you wish to sell?
   do
	{
		if( g.pCurrentPlayer->pCurrentSpace &&
			g.pCurrentPlayer->pCurrentSpace->type != SPACE_PROFESSION &&
			CountShares( &g.pCurrentPlayer->portfolio ) )
		{
         int yes;
			g.flags.bAllowSell = 1;
			AddRollToPossible();
			AddSellToPossible();
			StartFlash();
			printf( "Do you wish to sell stocks?" );
			yes = GetYesNo();
			g.flags.bAllowSell = 0;
			StopFlash();

			if( yes )
			{
				_32 cash;
				cash = SellStocks( &g.pCurrentPlayer->portfolio, 0, FALSE );
				printf( "%s sold stocks for a profit of $%ld\n"
						, g.pCurrentPlayer->name
						, cash );
				g.pCurrentPlayer->Cash += cash;
				UpdatePlayerDialog();
			}
			else
			{
				break;
			}
		}
		else
		{
         break;
		}
	}
   while( 1 );
}


void ResetPlayers( void ) 
{
    int i;
    INDEX idx;
    PPLAYER pPlayer;
    for(i = 0; i < MAX_ARCHTYPES; i++ )
    {
        archtypes[i].flags.used = 0;
    }
    LIST_FORALL( g.Players, idx, PPLAYER, pPlayer )
    {
        // reset cash, reset portfolio...
        pPlayer->archtype = NULL;
    }
}

void ChoosePlayerColor( PPLAYER pPlayer )
{
    int i, n, color;
	 char buffer[32];
	 pPlayer->archtype = NULL;

    do
    {
        printf( "Enter a name for player %d:", pPlayer->id );
        GetAString( pPlayer->name, sizeof( pPlayer->name ) );
        pPlayer->name[strlen(pPlayer->name)-1] = 0; // kill \n
        printf( "Player %d is %s, right?", pPlayer->id, pPlayer->name );
        buffer[0] = GetYesNo();
        if( buffer[0] )
            break;
	 } while( TRUE );
	 do
	 {
		 for( n = 1, i = 0; i < MAX_ARCHTYPES; i++ )
		 {
        if( !archtypes[i].flags.used )
			  printf( "%d > %s\n", n++, archtypes[i].colorname );
		 }
		 printf( "Select %s's color:", pPlayer->name );
		 color = GetANumber();
		 for( n = 1, i = 0; i < MAX_ARCHTYPES; i++ )
		 {
			 if( !archtypes[i].flags.used )
			 {
				 if( (n++) == color )
				 {
					 archtypes[i].flags.used = 1;
                pPlayer->archtype = archtypes + i;
					 break;
				 }
			 }
		 }
	 } while( !pPlayer->archtype );
	 CreatePlayerToken( pPlayer );
}

void ShowPositions( void )
{
    INDEX idx;
    PPLAYER player;
    LIST_FORALL( g.Players, idx, PPLAYER, player )
    {
        if( player->pCurrentSpace->type == SPACE_PROFESSION )
        {
        }
    }
}



void SetFirstPlayer( void )
{
    LIST_FORALL( g.Players, g.iCurrentPlayer, PPLAYER, g.pCurrentPlayer )
    {
        break;
    }
}

void StepNextPlayer( void )
{
	if( g.pCurrentPlayer )
	{
		if( g.pCurrentPlayer->nHistory == g.pCurrentPlayer->nHistoryAvail )
		{
         g.pCurrentPlayer->nHistoryAvail += 32;
			g.pCurrentPlayer->History = (_32*)Reallocate( g.pCurrentPlayer->History
															  , sizeof( _32 ) * g.pCurrentPlayer->nHistoryAvail );
		}
		g.pCurrentPlayer->History[g.pCurrentPlayer->nHistory++] =
         g.pCurrentPlayer->NetValue;
	}
	LIST_NEXTALL( g.Players, g.iCurrentPlayer, PPLAYER, g.pCurrentPlayer )
	{
		printf( "Setting current player to %d %s\n", g.iCurrentPlayer, g.pCurrentPlayer->name );
		break;
	}
	if( !g.pCurrentPlayer )
		SetFirstPlayer();
}

void ShowCurrentPlayer( void )
{
	UpdatePlayerDialog();
    printf( "Player: %15s Color: %s\n"
            , g.pCurrentPlayer->name
            , g.pCurrentPlayer->archtype->colorname
            );
    printf( "Current Value: $%ld $(%ld)\n"
            , g.pCurrentPlayer->Cash
            , g.pCurrentPlayer->NetValue
			 );
	ShowPortfolio( &g.pCurrentPlayer->portfolio, FALSE, FALSE );
	//DisableSheet( g.Panel, PANEL_PLAYER, FALSE );
	SetCurrentSheet( g.Panel, PANEL_PLAYER );
}

void ChoosePlayerProfessions( void )
{
	INDEX idx;
	int ch;
	PPLAYER player;
	do
	{
		int n = 0;
		LIST_FORALL( g.Players, idx, PPLAYER, player )
		{
            if( !player->pCurrentSpace )
            {
                printf( "%s(%s) is not on any space...\n"
                            , player->name
                            , player->archtype->colorname
							 );
                SetPlayerSpace( ChooseProfessionSpace( player ), player );
            }   
            if( player->pCurrentSpace->type == SPACE_PROFESSION )
			{
				AddLink( &g.PossiblePlayers, player );
				printf( "%d> %s(%s) $%ld is on %s\n"
							, ++n // incrememnt then display
							, player->name
							, player->archtype->colorname
							, player->NetValue
							, player->pCurrentSpace->attributes.profession.name );
            }
        }
		if( !n ) // noone's on a profession space...
			return;
		printf( "Enter player to move... L to list, 0 to continue");
		fflush( stdout );
		g.flags.bSelectPlayer = 1;
		AddRollToPossible();
        StartFlash();
		ch = GetCh();
		printf( "\n" );
        StopFlash();
        g.flags.bSelectPlayer = 0;
		  if( ch == '0' || ch == '\r' || ch == '\n' )
            break;
        if( ch == 'L' || ch == 'l' )
			continue;
        if( ch >= '1' && ( ch <= ('0' + n ) ) )
        {
            int test = 0;
            LIST_FORALL( g.Players, idx, PPLAYER, player )
            {
                if( player->pCurrentSpace->type == SPACE_PROFESSION )
                {
                    test++;
                    if( test == (ch -'0') )
                    {
							  SetPlayerSpace( ChooseProfessionSpace( player ), player );
                    }
                }
            }
        }
	} while( 1 );
	EmptyList( &g.PossiblePlayers );
}


void EvaluatePlayerValue( int bAtStart )
{
    PSTOCKACCOUNT account;
    INDEX idx;
   g.pCurrentPlayer->MinValue =
        g.pCurrentPlayer->NetValue = g.pCurrentPlayer->Cash;
    LIST_FORALL( g.pCurrentPlayer->portfolio, idx, PSTOCKACCOUNT, account )
    {
        g.pCurrentPlayer->NetValue += account->shares * GetStockValue( account->stock, FALSE );
      g.pCurrentPlayer->MinValue += account->shares * GetStockValue( account->stock, TRUE );
    }
    if( bAtStart )
    {
        if( g.pCurrentPlayer->pCurrentSpace )
        {
            if( g.pCurrentPlayer->pCurrentSpace->type == SPACE_PROFESSION )
            {
                if( g.pCurrentPlayer->NetValue >= 1000 )
                {
						 UpdatePlayerDialog();
						 SetPlayerSpace( ChooseStartSpace(), g.pCurrentPlayer );
                }
            }
        }
    }
}

CONTROL_REGISTRATION portfolio = { "StockMarket portfolio"
											, { { 520, 320 }, 0, BORDER_THIN|BORDER_INVERT }
											, NULL
											, NULL
											, DrawPortfolio
};



PRELOAD( RegisterPorfolioControl ){ DoRegisterControl( &portfolio ); }

void InitPlayer( void )
{
	archtypes[0].color = Color( 192, 40, 40 );
	archtypes[1].color = Color( 0, 0, 128);
	archtypes[2].color = Color( 0, 128, 0 );
	archtypes[3].color = Color( 164, 165, 0 );
	archtypes[4].color = Color( 128, 0, 92 );
	archtypes[5].color = Color( 0, 0, 1 );
	archtypes[6].color = Color( 192, 192, 192 );
	archtypes[7].color = Color( 128, 0, 128 );

   AddSheet( g.Panel, g.Player = CreateFrame( "Status"
									 , 0, 0
									 , g.PanelWidth, g.PanelHeight
														  , BORDER_NOCAPTION|BORDER_NONE|BORDER_WITHIN, NULL ) );
   SetControlID( g.Player, PANEL_PLAYER );
   //DisableSheet( g.Panel, PANEL_PLAYER, TRUE );
	MakeTextControl( g.Player, 5, 5, 55, 15, TXT_STATIC, "Player:", 0 );
	MakeTextControl( g.Player
						, 65, 5
						, 100, 15, TXT_PLAYER, "Bob", 0 );
	MakeTextControl( g.Player, 5, 22, 45, 15, TXT_STATIC, "Cash", 0 );
	MakeTextControl( g.Player, 55, 22, 75, 15, TXT_CASH, "$99999", 0 );
	MakeTextControl( g.Player, 135, 22, 45, 15, TXT_STATIC, "Value", 0 );
	MakeTextControl( g.Player, 185, 22, 100, 15, TXT_VALUE, "$999999", 0 );
	{
      Image Surface = GetFrameSurface( g.Player );
		PCONTROL pc = MakeControl( g.Player
										 , portfolio.TypeID
										 , 5, 56
										 , Surface->width - 18, ( g.scale * 3 / 2)
										 , CST_PORTFOLIO
										 );
      SetCommonUserData( pc, 0 );
		//SetControlDraw( pc, DrawPortfolio, 0 );
		pc = MakeControl( g.Player, portfolio.TypeID
							 , 5, 56 + 3 + ( g.scale * 3 / 2)
							 , Surface->width - 18, ( g.scale * 3 / 2)
							 , CST_PORTFOLIO );
		//SetControlDraw( pc, DrawPortfolio, 4 );
      SetCommonUserData( pc, 4 );
	}

}

