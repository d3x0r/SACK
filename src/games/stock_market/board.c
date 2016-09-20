#include <stdhdrs.h>
#include <logging.h>
#include <sharemem.h>
#include <timers.h>
#include <idle.h>
#include <configscript.h>

#include "global.h"

#include "stocks.h"
#include "board.h"
#include "input.h"
#include "player.h"


//-----------------------------------------------------------------
#include <psi.h>
extern CONTROL_REGISTRATION board_space;
//-----------------------------------------------------------------
/*
void MountPanel( PFRAME frame )
{
	if( frame != g.Mounted )
	{
      if( g.Mounted )
			OrphanFrame( g.Mounted );
		AdoptFrame( g.Panel, NULL, frame );
      g._Mounted = g.Mounted;
      g.Mounted = frame;
	}
}

void UnmountPanel( PFRAME frame )
{
	if( frame == g.Mounted && g._Mounted)
	{
		OrphanFrame( g.Mounted );
		AdoptFrame( g.Panel, NULL, g._Mounted );
		g.Mounted = g._Mounted;
      g._Mounted = NULL;
	}
}
*/
void AlignTokens( PSPACE pSpace )
{
	int x, y;
	Image Surface = GetControlSurface( pSpace->region );
	x = Surface->width - 2;
	y = Surface->height - 2;
	switch( pSpace->type )
	{
	default:
		{
			PPLAYER pPlayer;
			Image Token;
			for( pPlayer = pSpace->pPlayers; pPlayer; pPlayer = pPlayer->next)
			{
				Token = GetControlSurface( pPlayer->pPlayerToken );
				x -= Token->width + 1;
				if( x < 0 )
				{
					x = (Surface->width - 2) - (Token->width + 1 );
					y-= Token->height + 1;
				}
				lprintf( WIDE("Moving token to %d,%lu\n")
						, x, y - (Token->height+1));
				MoveControl( pPlayer->pPlayerToken, x, y - (Token->height+1));
			}
		}
      break;
	}
}

//-----------------------------------------------------------------

void SetPlayerSpace( PSPACE pSpace, PPLAYER pPlayer )
{
	pPlayer->pCurrentSpace = pSpace;
	lprintf( WIDE("ORPHAN TOKEN") );
	OrphanControlEx( pPlayer->pPlayerToken, TRUE );
	RelinkThing( pSpace->pPlayers, pPlayer );
	lprintf( WIDE("ADOPT TOKEN") );
	AdoptControlEx( pSpace->region
               , NULL
					, pPlayer->pPlayerToken, TRUE );
	AlignTokens( pSpace );
	UpdateCommon( pSpace->region ); // update
}

//-----------------------------------------------------------------

void ShowASpace( PSPACE pSpace, PSTOCK pBranch )
{
	lprintf( WIDE("(%ld)"), pSpace->ID );
    switch( pSpace->type )
    {
    case SPACE_START:
        lprintf( WIDE("Start (Fee $%ld)\n")
                , pSpace->attributes.start.cost );
        break;
    case SPACE_BROKER:
        lprintf( WIDE("Broker's Fee ($%ld/share)\n")
                , pSpace->attributes.broker.fee );
        break;
    case SPACE_STOCKSELL:
        lprintf( WIDE("Sell all %s at %ld\n")
                , pSpace->attributes.buy_sell.stock->name
                , GetStockValue( pSpace->attributes.buy_sell.stock, TRUE )
                );
        break;
    case SPACE_STOCKBUY:
        lprintf( WIDE("Buy %s\n")
                , pSpace->attributes.buy_sell.stock->name
                );
        break;
    case SPACE_HOLDERSMEETING:
        lprintf( WIDE("Split %s %d-%d\n")
                , pBranch->name
                , pSpace->attributes.split.ratio.numerator
                , pSpace->attributes.split.ratio.denominator
                );
        break;
    case SPACE_HOLDERSENTRANCE:
        lprintf( WIDE("Buy 1 %s (enter meeting)\n")
                , pSpace->attributes.buy_sell.stock->name
                );
		  break;
	 default:
       lprintf( WIDE("A Space.\n") );
       break;
    }
}

void ChoosePlayerDestination( void )
{
    int n;
    int nPossible, nPossibility;
	int bLeft[4], bInMeeting[4];
	PSPACE pPossible[4];
	PSTOCK pBranch[4]; // stock which the meeting is for
	PSPACE pSpace = g.pCurrentPlayer->pCurrentSpace;
	for( n = 0; n < 4; n++ )
      bInMeeting[n] = 0;
	// current roll determines possible spaces from here...
    switch( pSpace->type )
    {
    case SPACE_PROFESSION:
		return; // no motion this way...
    case SPACE_START:
        if( g.nCurrentRoll & 1 )
            g.pCurrentPlayer->flags.GoingLeft =
                !pSpace->attributes.start.flags.bEvenRight;
		else
            g.pCurrentPlayer->flags.GoingLeft =
                pSpace->attributes.start.flags.bEvenRight;
		break;
	case SPACE_HOLDERSENTRANCE:
	case SPACE_STOCKSELL:
	case SPACE_STOCKBUY:
	case SPACE_BROKER:
		g.pCurrentPlayer->flags.GoingLeft = pSpace->flags.bMoveLeft;
		break;
	case SPACE_HOLDERSMEETING:
		 pBranch[0] = g.pCurrentPlayer->pMeeting->stock;
		 break;
	default:
		break;
    }
    if( g.pCurrentPlayer->flags.GoingLeft )
    {
        lprintf( WIDE("Going left... %d\n"), g.nCurrentRoll );
		  bLeft[0] = TRUE;
    }
	else
	{
		lprintf( WIDE("Going right...%d\n"), g.nCurrentRoll );
		bLeft[0] = FALSE;
	}
	nPossible = 1;
	pPossible[0] = pSpace;
    for( n = 0; n < g.nCurrentRoll; n++ )
    {
		 int nCheck = nPossible;
		 //lprintf( WIDE("Processing %d"), n );
		 for( nPossibility = 0; nPossibility < nCheck; nPossibility++ )
		 {
            //lprintf( WIDE("possibility %d (%d)"), nPossibility, pPossible[nPossibility]->alternate );
            if( pPossible[nPossibility]->alternate != INVALID_INDEX &&
                !bInMeeting[nPossibility] // coming out of a meeting
              )
            {
                PSTOCKACCOUNT pAccount =
                    GetStockAccount( &g.pCurrentPlayer->portfolio
											  , pPossible[nPossibility]->attributes.buy_sell.stock );
					 //lprintf( WIDE("had stock: %p"), pAccount );
                if( pAccount && pAccount->shares )
                {
                    //lprintf( WIDE("Alternate path.") );
                    pBranch[nPossible] = pPossible[nPossibility]->attributes.buy_sell.stock;
                    bLeft[nPossible] = pPossible[nPossibility]->flags.bAlternateLeft;
                    pPossible[nPossible++] = (PSPACE)GetLink( &g.Board, pPossible[nPossibility]->alternate );
						  //ShowASpace( pPossible[nPossible-1], pBranch[nPossible-1] );
                }
            }
            if( pPossible[nPossibility]->type == SPACE_HOLDERSMEETING )
                bInMeeting[nPossibility] = 1;
         else
                bInMeeting[nPossibility] = 0;
            //ShowASpace( pPossible[nPossibility], pBranch[nPossibility] );
            if( bLeft[nPossibility] )
                pPossible[nPossibility] = (PSPACE)GetLink( &g.Board, pPossible[nPossibility]->left );
            else
                pPossible[nPossibility] = (PSPACE)GetLink( &g.Board, pPossible[nPossibility]->right );
        }
    }
    do
	 {
        for( n = 0; n < nPossible; n++ )
        {
            if( nPossible > 1 )
            {
                lprintf( WIDE("%d> "), n + 1);
					 AddLink( &g.PossibleSpaces, pPossible[n] );
				}
				ShowASpace( pPossible[n], pBranch[n] );
        }
        if( nPossible > 1 )
        {
			  lprintf( WIDE("Choose a space:") );
           StartFlash();
			  n = GetANumber();
        }
    }
	 while( nPossible > 1 && ( n < 1 || n > nPossible ) );
    StopFlash();
	 n--;
    SetPlayerSpace( pPossible[n], g.pCurrentPlayer );
    g.pCurrentPlayer->pCurrentSpace = pPossible[n];
	 g.pCurrentPlayer->flags.GoingLeft = bLeft[n];
	 if( pPossible[n]->type == SPACE_HOLDERSMEETING )
		 g.pCurrentPlayer->pMeeting =
			 GetStockAccount( &g.pCurrentPlayer->portfolio
								 , pBranch[n] );
}

void ProcessCurrentSpace( void )
{
   PSPACE pSpace = g.pCurrentPlayer->pCurrentSpace;
    switch( pSpace->type )
    {
    case SPACE_START:
        if( g.pCurrentPlayer->MinValue < pSpace->attributes.start.cost )
        {
            lprintf( WIDE("%s has gone bankrupt.  Please start again.\n"), g.pCurrentPlayer->name );
            g.pCurrentPlayer->Cash += SellAllStocks( &g.pCurrentPlayer->portfolio, TRUE );
            g.pCurrentPlayer->pCurrentSpace = NULL;
				ChoosePlayerProfessions();
        }
        while( g.pCurrentPlayer->Cash < pSpace->attributes.start.cost )
        {
			  lprintf( WIDE("Not enough cash to cover start fee...\n") );
			  g.pCurrentPlayer->Cash += SellStocks( &g.pCurrentPlayer->portfolio
															  , pSpace->attributes.start.cost
																- g.pCurrentPlayer->Cash
															  , TRUE );
        }
        g.pCurrentPlayer->Cash -= pSpace->attributes.start.cost;
		  lprintf( WIDE("%s paid $%ld start fee\n")
				  , g.pCurrentPlayer->name
				  , pSpace->attributes.start.cost );
        break;
    case SPACE_BROKER:
        {
         uint32_t shares;
            UpdateStocks( pSpace->FixedStageAdjust );
            shares = CountShares( &g.pCurrentPlayer->portfolio );
            if( shares )
            {
                lprintf( WIDE("%s must pay %ld to the broker.\n")
                        , g.pCurrentPlayer->name
                        , shares * pSpace->attributes.broker.fee
                        );
					 while( g.pCurrentPlayer->Cash < (shares * pSpace->attributes.broker.fee) )
					 {
						 if( shares * pSpace->attributes.broker.fee > g.pCurrentPlayer->Cash )
						 {
							 lprintf( WIDE("Sorry you must sell some stocks to cover the fee.\n") );
							 g.pCurrentPlayer->Cash +=
								 SellStocks( &g.pCurrentPlayer->portfolio
											  , shares * pSpace->attributes.broker.fee
												- g.pCurrentPlayer->Cash
											  , TRUE );
						 }
					 }
					 {
                   TEXTCHAR msg[128];
						 snprintf( msg, sizeof( msg ), WIDE("%s paid $%ld in broker's fees.")
								  , g.pCurrentPlayer->name
								  , shares * pSpace->attributes.broker.fee );
						 SimpleMessageBox( g.board, WIDE("Forced Sell"), msg );
					 }
					 g.pCurrentPlayer->Cash -= shares * pSpace->attributes.broker.fee;
            }
            else
            {
					lprintf( WIDE("You have no shares - no broker to pay!\n") );
            }
        }
        break;
    case SPACE_STOCKSELL:
        {
         uint32_t cash;
            PSTOCKACCOUNT pAccount =
                GetStockAccount( &g.pCurrentPlayer->portfolio
                                    , pSpace->attributes.buy_sell.stock );
            UpdateStocks( pSpace->FixedStageAdjust );
            if( pAccount && pAccount->shares )
				{
                cash = SellStock( &g.pCurrentPlayer->portfolio
                                     , pAccount
                                     , TRUE
										  , pAccount->shares );
					 {
						TEXTCHAR msg[128];
						 snprintf( msg, sizeof( msg ), WIDE("%s had to sell %s, received $%ld")
								  , g.pCurrentPlayer->name
								  , pSpace->attributes.buy_sell.stock->name
								  , cash );
						 SimpleMessageBox( g.board, WIDE("Forced Sell"), msg );
					 }
                lprintf( WIDE("Sold all %s for $%ld\n")
                        , pSpace->attributes.buy_sell.stock->name
                        , cash );
                g.pCurrentPlayer->Cash += cash;
            }
				else
				{
                lprintf( WIDE("Had no %s to sell.\n")
                        , pSpace->attributes.buy_sell.stock->name
							 );
				}
        }
        break;
    case SPACE_HOLDERSMEETING:
        {
            FRACTION result;
         uint32_t shares = g.pCurrentPlayer->pMeeting->shares;
            ScaleFraction( &result
                             , shares
                             , &pSpace->attributes.split.ratio );
            {
                TEXTCHAR msg[256];
					 sLogFraction( msg, &result );
            }
            g.pCurrentPlayer->pMeeting->shares += ReduceFraction( &result );
            lprintf( WIDE("%s had %ld shares and now has %ld shares\n")
                    ,g.pCurrentPlayer->name
                    , shares
                    , g.pCurrentPlayer->pMeeting->shares
                    );
        }
        break;
    case SPACE_HOLDERSENTRANCE:
        {
            int max_shares;
			max_shares = 1;
            if(0) {
    case SPACE_STOCKBUY:
                max_shares = 0; // set max by cash amount...
            }
				{
					uint32_t value;
					PSTOCKACCOUNT pAccount =
						GetStockAccount( &g.pCurrentPlayer->portfolio
											, pSpace->attributes.buy_sell.stock );
					uint32_t shares;

					if( pAccount && pAccount->shares )
					{
						g.pCurrentPlayer->Cash +=
							pAccount->stock->Dividend * pAccount->shares;
						lprintf( WIDE("Paid %s a dividend of $%ld for %ld of %s\n")
                        , g.pCurrentPlayer->name
								, pAccount->stock->Dividend * pAccount->shares
								, pAccount->shares
								, pAccount->stock->name );
					}
					UpdateStocks( pSpace->FixedStageAdjust );

					value = GetStockValue( pSpace->attributes.buy_sell.stock, FALSE );
					do {
						lprintf( WIDE("%s is going for $%ld\n")
								, pSpace->attributes.buy_sell.stock->name
								, value );
						if( max_shares == 1 )
						{
							if( value <= g.pCurrentPlayer->Cash )
								lprintf( WIDE("%s may buy one share\n")
										, g.pCurrentPlayer->name
										);
							else
							{
								max_shares = 0;
							}
						}
						else
						{
							lprintf( WIDE("%s has %ld shares and can buy up to %ld more shares\n")
									, g.pCurrentPlayer->name
									, pAccount?pAccount->shares:0
                            , max_shares = g.pCurrentPlayer->Cash / value
									);
							if( max_shares )
								lprintf( WIDE("5=$%ld 25=$%ld\n")
										, 5 * value
										, 25 * value
										);
						}
						if( max_shares )
						{
							lprintf( WIDE("How many shares do you want? (0 to abort)") );
							BuySomeStock( pSpace->attributes.buy_sell.stock, max_shares );
							shares = GetANumber();
							StockBuyEnd();
							if( shares <= max_shares )
							{
								max_shares -= shares;
								if( shares )
								{
                            lprintf( WIDE("Buy %ld shares for $%ld (total of %ld)")
											 , shares
											 , shares * value
											 , shares + (pAccount?pAccount->shares:0) );
									 if( GetYesNo() )
										 g.pCurrentPlayer->Cash -=
											 BuyStock( &g.pCurrentPlayer->portfolio
														, pSpace->attributes.buy_sell.stock
														, shares );
								}
								lprintf( WIDE("Are you done?") );
								if( GetYesNo() )
									break;
							}
							else
							{
								lprintf( WIDE("Cannot buy that many!\n") );
							}
						}
						else
							lprintf( WIDE("Sorry, cannot afford any more shares.\n") );
					}
					while( max_shares );
					UpdatePlayerDialog();
				}
				break;
		default:
			break;
		}
	}
}

void PayWageSlaves( void )
{
	INDEX idx;
	PPLAYER player;
	// pay players
	LIST_FORALL( g.Players, idx, PPLAYER, player )
	{
		if( player->pCurrentSpace->type == SPACE_PROFESSION )
		{
			if( TESTFLAG( player->pCurrentSpace->attributes.profession.payon, g.nCurrentRoll ) )
			{
				player->Cash += player->pCurrentSpace->attributes.profession.pay;
				lprintf( WIDE("%s(%s) gets $%ld\n")
						, player->name
						, player->archtype->colorname
						, player->pCurrentSpace->attributes.profession.pay );
			}
		}
	}
}

//-----------------------------------------------------------------

PSPACE ChooseStartSpace( void )
{
    int n, which;
    INDEX idx;
	 PSPACE pSpace;
    do
	 {
        n = 0;
        LIST_FORALL( g.Board, idx, PSPACE, pSpace )
        {
            if( pSpace->type == SPACE_START )
				{
					AddLink( &g.PossibleSpaces, pSpace );
               lprintf( WIDE("%d> Start\n"), ++n );
            }
		  }
        StartFlash();
		  which = GetANumber();
		  if( which <= n && which >= 1 )
           break;
	 } while( 1 );
	 pSpace = (PSPACE)GetLink( &g.PossibleSpaces
						  , which -1 );
    StopFlash();
	 return pSpace;
}

//-----------------------------------------------------------------

PSPACE ChooseProfessionSpace( PPLAYER player )
{
	int n;
	int ch;
	INDEX idx;
	PSPACE pSpace;
	do
	{
		do
		{
			n = 0;
			LIST_FORALL( g.Board, idx, PSPACE, pSpace )
			{
				if( pSpace->type == SPACE_PROFESSION )
				{
					int roll;
					AddLink( &g.PossibleSpaces, pSpace );
					StartFlash();
					lprintf( WIDE("%d > %s (pays $%ld on ")
							, ++n
							, pSpace->attributes.profession.name
							, pSpace->attributes.profession.pay
							);
					for( roll = 1; roll <=12; roll++ )
						if( TESTFLAG( pSpace->attributes.profession.payon, roll ) )
						{
							lprintf( WIDE("%d"), roll++ );
							break;
						}
					lprintf( WIDE(" or ") );
					for( ; roll <=12; roll++ )
						if( TESTFLAG( pSpace->attributes.profession.payon, roll ) )
						{
							lprintf( WIDE("%d"), roll++ );
							break;
						}
					lprintf( WIDE(")\n") );
				}
			}

			lprintf( WIDE("Select profession space for %s(%s):")
					, player->name
					, player->archtype->colorname );
			fflush( stdout );
			ch = GetCh();
			lprintf( WIDE("\n") );
			if( ch < '1' || ( ch > ( '0' + n ) ) )
				continue;
		} while( 0 );

		n = 0;
		LIST_FORALL( g.Board, idx, PSPACE, pSpace )
		{
			if( pSpace->type == SPACE_PROFESSION )
			{
				n++;
				if( n + '0' == ch )
					break;
			}
		}
	}
	while (!pSpace );
	StopFlash();
	return pSpace;
}

//-----------------------------------------------------------------

void ValidateBoard( void )
{   
    INDEX idx;
    PSPACE pSpace;
    LIST_FORALL( g.Board, idx, PSPACE, pSpace )
    {
        PSPACE left, right;
        //Log3( WIDE("Check %d and %d from %d"), pSpace->left, pSpace->right, idx );
        if( pSpace->type == SPACE_PROFESSION ||
             pSpace->type == SPACE_QUIT )
            continue;
        left = (PSPACE)GetLink( &g.Board, pSpace->left );
        right = (PSPACE)GetLink( &g.Board, pSpace->right );
        if( pSpace->type == SPACE_HOLDERSENTRANCE )
            if( pSpace->alternate == INVALID_INDEX )
            Log( WIDE("Holders entrance lacks a meeting path") );
        if( !left || !right )
        {
            Log1( WIDE("%d - Space lacks left or right..."), idx );
        }
        if( pSpace->type == SPACE_HOLDERSMEETING )
        {
         if( left->type == SPACE_HOLDERSENTRANCE )
            {
                if( GetLink( &g.Board, left->alternate ) != pSpace )
               Log2( WIDE("entrance %d and exit %d mismatch"), pSpace->left, idx );
            }
            else if( GetLink( &g.Board, left->right ) != pSpace )
            {
                Log2( WIDE("%d and %d mismatch"), idx, pSpace->left );
            }
            if( right->type == SPACE_HOLDERSENTRANCE )
            {
                if( GetLink( &g.Board, right->alternate ) != pSpace )
               Log2( WIDE("entrance %d and exit %d mismatch"), pSpace->right, idx );
            }
         else if( GetLink( &g.Board, right->left ) != pSpace )
            {
                Log2( WIDE("%d and %d mismatch"), idx, pSpace->right );
            }
        }
        else
        {
            if( GetLink( &g.Board, left->right ) != pSpace )
            {
                Log2( WIDE("%d and %d mismatch"), idx, pSpace->left );
            }
            if( GetLink( &g.Board, right->left ) != pSpace )
            {
                Log2( WIDE("%d and %d mismatch"), idx, pSpace->right );
            }
        }
    }
}

//-----------------------------------------------------------------

uintptr_t CPROC AddSpace( uintptr_t psv, arg_list args )
{
   PARAM( args, int64_t, ID );
    if( psv )
    {
        // check for last space's completeness
    }
    {
        PSPACE pSpace = New( SPACE );
      MemSet( pSpace, 0, sizeof( SPACE ) );
      SetLink( &g.Board, (INDEX)ID, pSpace );
      pSpace->ID = ID;
      pSpace->left = INVALID_INDEX;
        pSpace->right = INVALID_INDEX;
      pSpace->alternate = INVALID_INDEX;
        return (uintptr_t)pSpace;
    }
}

//-----------------------------------------------------------------

uintptr_t CPROC AddCenter( uintptr_t psv, arg_list args )
{
    PARAM( args, TEXTCHAR *, name );
    PARAM( args, uint64_t, pay );
    PARAM( args, uint64_t, roll1 );
    PARAM( args, uint64_t, roll2 );
    PSPACE pSpace = (PSPACE)psv;
    Log4( WIDE("Name: %s Pay %d on %d or %d"), name, (uint32_t)pay, (uint32_t)roll1, (uint32_t)roll2 );
    pSpace->type = SPACE_PROFESSION;
    pSpace->attributes.profession.name = NewArray( TEXTCHAR, strlen( name ) + 1 );
    strcpy( pSpace->attributes.profession.name, name );
    pSpace->attributes.profession.pay = pay;
    SETFLAG( pSpace->attributes.profession.payon, roll1 );
    SETFLAG( pSpace->attributes.profession.payon, roll2 );
    return psv;
}

//-----------------------------------------------------------------

uintptr_t CPROC SetMarketUp( uintptr_t psv, arg_list args )
{
   PARAM( args, int64_t, amount );
    PSPACE pSpace = (PSPACE)psv;
    pSpace->FixedStageAdjust = amount;
    return psv;
}

//-----------------------------------------------------------------

uintptr_t CPROC SetMarketDown( uintptr_t psv, arg_list args )
{
   PARAM( args, int64_t, amount );
    PSPACE pSpace = (PSPACE)psv;
    pSpace->FixedStageAdjust = -amount;
    return psv;
}

//-----------------------------------------------------------------

uintptr_t CPROC SetStockBuy( uintptr_t psv, arg_list args )
{
   PARAM( args, int64_t, stock );
    PSPACE pSpace = (PSPACE)psv;
   if( pSpace->type == SPACE_UNKNOWN )
        pSpace->type = SPACE_STOCKBUY;
   pSpace->attributes.buy_sell.stock = GetStockByID( stock );
   return psv;
}

//-----------------------------------------------------------------

uintptr_t CPROC SetStockSell( uintptr_t psv, arg_list args )
{
   PARAM( args, int64_t, stock );
    PSPACE pSpace = (PSPACE)psv;
   pSpace->type = SPACE_STOCKSELL;
   pSpace->attributes.buy_sell.stock = GetStockByID( stock );
   return psv;
}

//-----------------------------------------------------------------

uintptr_t CPROC SetSpaceBroker( uintptr_t psv, arg_list args )
{
    PSPACE pSpace = (PSPACE)psv;
    pSpace->type = SPACE_BROKER;
   pSpace->attributes.broker.fee = 10;
   return psv;
}

//-----------------------------------------------------------------

uintptr_t CPROC SetSpaceLeft( uintptr_t psv, arg_list args )
{
   PARAM( args, int64_t, ID );
    PSPACE pSpace = (PSPACE)psv;
   pSpace->left = ID;
   return psv;
}

//-----------------------------------------------------------------

uintptr_t CPROC SetSpaceRight( uintptr_t psv, arg_list args )
{
   PARAM( args, int64_t, ID );
    PSPACE pSpace = (PSPACE)psv;
   pSpace->right = ID;
   return psv;
}

//-----------------------------------------------------------------

uintptr_t CPROC SetHolderEntranceLeft( uintptr_t psv, arg_list args )
{
   PARAM( args, int64_t, ID );
    PSPACE pSpace = (PSPACE)psv;
    pSpace->alternate = ID;
   pSpace->type = SPACE_HOLDERSENTRANCE;
   pSpace->flags.bAlternateLeft = 1;
   return psv;
}

//-----------------------------------------------------------------

uintptr_t CPROC SetHolderEntranceRight( uintptr_t psv, arg_list args )
{
   PARAM( args, int64_t, ID );
    PSPACE pSpace = (PSPACE)psv;
   pSpace->type = SPACE_HOLDERSENTRANCE;
   pSpace->alternate = ID;
   pSpace->flags.bAlternateLeft = 0;
   return psv;
}

//-----------------------------------------------------------------

uintptr_t CPROC SetExitLeft( uintptr_t psv, arg_list args )
{
    PSPACE pSpace = (PSPACE)psv;
   pSpace->flags.bMoveLeft = 1;
   return psv;
}

//-----------------------------------------------------------------

uintptr_t CPROC SetExitRight( uintptr_t psv, arg_list args )
{
    PSPACE pSpace = (PSPACE)psv;
   pSpace->flags.bMoveLeft = 0;
   return psv;
}

//-----------------------------------------------------------------

uintptr_t CPROC SetSpaceStart( uintptr_t psv, arg_list args )
{
   PSPACE pSpace = (PSPACE)psv;
	pSpace->type = SPACE_START;
   // should option this also...
   pSpace->attributes.start.cost = 100;
   return psv;
}

//-----------------------------------------------------------------

uintptr_t CPROC SetStockSplit( uintptr_t psv, arg_list args )
{
   PARAM( args, FRACTION, fraction );
    PSPACE pSpace = (PSPACE)psv;
    pSpace->type = SPACE_HOLDERSMEETING;
   pSpace->attributes.split.ratio = fraction;
   return psv;
}

//-----------------------------------------------------------------

uintptr_t CPROC SetSpaceSize( uintptr_t psv, arg_list args )
{
    PARAM( args, int64_t, width );
    PARAM( args, int64_t, height );
    PSPACE pSpace = (PSPACE)psv;
    pSpace->position.width = width;
    pSpace->position.height = height;
    return psv;
}

//-----------------------------------------------------------------

uintptr_t CPROC SetSpacePosition( uintptr_t psv, arg_list args )
{
    PARAM( args, int64_t, x );
    PARAM( args, int64_t, y );
    PSPACE pSpace = (PSPACE)psv;
    pSpace->position.x = x;
    pSpace->position.y = y;
    return psv;
}

//-----------------------------------------------------------------

uintptr_t CPROC SetSpaceColor( uintptr_t psv, arg_list args )
{
    PARAM( args, CDATA, color );
    PSPACE pSpace = (PSPACE)psv;
    pSpace->attributes.profession.color = color;
   return psv;
}

//-----------------------------------------------------------------

uintptr_t CPROC SetRandomRoll( uintptr_t psv, arg_list args )
{
	PARAM( args, LOGICAL, yesno );
   g.flags.bRandomRoll = yesno;
   return psv;
}

//-----------------------------------------------------------------

uintptr_t CPROC SetHorizontalAlignment( uintptr_t psv, arg_list args )
{
    PSPACE pSpace = (PSPACE)psv;
    pSpace->flags.bVertical = FALSE;
    pSpace->flags.bInvert = FALSE;
    return psv;
}

//-----------------------------------------------------------------

uintptr_t CPROC SetHorizontalLeftAlignment( uintptr_t psv, arg_list args )
{
    PSPACE pSpace = (PSPACE)psv;
    pSpace->flags.bVertical = FALSE;
    pSpace->flags.bInvert = TRUE;
    return psv;
}

//-----------------------------------------------------------------

uintptr_t CPROC SetVerticalUpAlignment( uintptr_t psv, arg_list args )
{
    PSPACE pSpace = (PSPACE)psv;
    pSpace->flags.bVertical = TRUE;
    pSpace->flags.bInvert = TRUE;
    return psv;
}

//-----------------------------------------------------------------

uintptr_t CPROC SetSpaceQuit( uintptr_t psv, arg_list args )
{
    PSPACE pSpace = (PSPACE)psv;
    pSpace->type = SPACE_QUIT;
    return psv;
}

//-----------------------------------------------------------------

uintptr_t CPROC SetRollDice( uintptr_t psv, arg_list args )
{
    PSPACE pSpace = (PSPACE)psv;
    pSpace->type = SPACE_ROLL;
    return psv;
}

//-----------------------------------------------------------------

uintptr_t CPROC SetPlayerSell( uintptr_t psv, arg_list args )
{
    PSPACE pSpace = (PSPACE)psv;
    pSpace->type = SPACE_SELL;
    return psv;
}

//-----------------------------------------------------------------

uintptr_t CPROC SetVerticalDownAlignment( uintptr_t psv, arg_list args )
{
    PSPACE pSpace = (PSPACE)psv;
    pSpace->flags.bVertical = TRUE;
    pSpace->flags.bInvert = FALSE;
    return psv;
}

//-----------------------------------------------------------------

void PutDirectedArrow( Image image, int pos
							, int width, int height
							, CDATA color
							, int dir, int bLeft )
{
	switch( dir )
	{
	case 0:
		do_hline( image, pos, 5, width - 10, color );
		if( bLeft )
		{
			do_line( image, width-10, pos, width-10 - 3, pos-3, color );
			do_line( image, width-10, pos, width-10 - 3, pos+3, color );
		}
		else
		{
			do_line( image, 5, pos, 5+3, pos-3, color );
			do_line( image, 5, pos, 5+3, pos+3, color );
		}
      break;
	case 1:
		do_vline( image, width-pos, 5, height - 10, color );
		if( bLeft )
		{
			do_line( image, width-pos, height-10, width-pos-3, height-10 - 3, color );
			do_line( image, width-pos, height-10, width-pos+3, height-10 - 3, color );
		}
		else
		{
			do_line( image, width-pos, 5, width-pos-3, 5+3, color );
			do_line( image, width-pos, 5, width-pos+3, 5+3, color );
		}
      break;
	case 2:
		do_hline( image, height-pos, 5, width - 10, color );
		if( bLeft )
		{
			do_line( image, 5, height-pos, 5+3, height-pos-3, color );
			do_line( image, 5, height-pos, 5+3, height-pos+3, color );
		}
		else
		{
			do_line( image, width-10, height-pos, width-10 - 3, height-pos-3, color );
			do_line( image, width-10, height-pos, width-10 - 3, height-pos+3, color );
		}
      break;
	case 3:
		do_vline( image, pos, 5, height - 10, color );
		if( bLeft )
		{
			do_line( image, pos, 5, pos-3, 5+3, color );
			do_line( image, pos, 5, pos+3, 5+3, color );
		}
		else
		{
			do_line( image, pos, height-10, pos-3, height-10 - 3, color );
			do_line( image, pos, height-10, pos+3, height-10 - 3, color );
		}
      break;
	}
}

//-----------------------------------------------------------------

void PutDirectedText( Image image, int x, int y
                    , int width, int height
						  , CDATA fore, CDATA back
                      , int dir, TEXTCHAR *text )
{
    switch( dir )
    {
    case 0:
        PutString( image, x, y, fore, back, text );
        break;
    case 1:
		 PutStringVertical( image
								, width - y, x
								, fore, back, text );
		 break;
    case 2:
		 PutStringInvert( image
							 , width-x, height-y
							 , fore, back, text );
        break;
    case 3:
		 PutStringInvertVertical( image
										, y, height - x
										, fore, back, text );
      break;
    }
}

//-----------------------------------------------------------------

int CPROC DrawSpace( PCOMMON pc )
{
	PCOMMON pRegion = pc;
	PSPACE pSpace = (PSPACE)GetCommonUserData( pc );
	//ValidatedControlData( PSPACE, board_space.TypeID, pSpace, pc );
    TEXTCHAR text[15];
    int x = 2
     , y = 2
     , width
	 , height;
    int dir;
    Image Surface;
    Surface = GetControlSurface( pRegion );
    width = Surface->width - 4;
    height = Surface->height - 4;

	if( pSpace->flags.bFlashing )
	{
		if( pSpace->flags.bFlashOn )
			ClearImageTo( Surface, Color( 255, 0, 0 ) );
		else
			ClearImageTo( Surface, Color( 255, 255, 255 ) );
	}
	else
		ClearImageTo( Surface, Color(0,0,1) );
    dir = (int)pSpace->flags.bInvert * 2 + pSpace->flags.bVertical;
	switch( pSpace->type )
	{
	case SPACE_STOCKBUY:
	case SPACE_HOLDERSENTRANCE:
	case SPACE_STOCKSELL:
    {
		BlatColor( Surface, x, y, width, height, pSpace->attributes.buy_sell.stock->color );
		snprintf( text, sizeof( text ), WIDE("%4.4s"), pSpace->attributes.buy_sell.stock->Symbol );
		PutDirectedText( Surface
                          , x + 3, 5, Surface->width, Surface->height
                          , Color( 255,255,255 ), AColor( 0,0,0, 32)
                          , dir
							  , text );
		  if( pSpace->type == SPACE_STOCKSELL )
		  {
			  PutDirectedText( Surface
								  , x + 8, 17, Surface->width, Surface->height
								  , Color( 255,255,255 ), AColor( 0,0,0, 32)
								  , dir
								  , WIDE("SELL") );
		  }
		  PutDirectedArrow( Surface, height - 5
								, Surface->width, Surface->height
								, Color( 0, 0, 0 )
								, dir
								, pSpace->flags.bMoveLeft );
		{
			TEXTCHAR staging[8];
			  if( pSpace->FixedStageAdjust > 0 )
			  {
              snprintf( staging, sizeof( staging ), WIDE("Up %d"), pSpace->FixedStageAdjust );
			  }
			  else
			  {
              snprintf( staging, sizeof( staging ), WIDE("Dn %d"), -pSpace->FixedStageAdjust );
			  }
			  PutDirectedText( Surface
								  , x+8, 29
								  ,Surface->width, Surface->height
								  , Color( 255,255,255 ), AColor( 0,0,0, 32)
								  , dir
								  , staging );
		  }
		  break;
	 }
	case SPACE_BROKER:
    {
        BlatColor( Surface, x, y, width, height, Color( 255,255,255 ) );
        snprintf( text, sizeof( text ), WIDE("BRKR") );
        PutDirectedText( Surface
                          , 5, 5, Surface->width, Surface->height
                          , Color( 0,0,0 ), AColor( 255,255,255, 32)
                          , dir
                          ,  text
							  );
		  PutDirectedArrow( Surface, height - 5
								, Surface->width, Surface->height
								, Color( 0, 0, 0 )
								, dir
                        , pSpace->flags.bMoveLeft );
		  {
           TEXTCHAR staging[8];
			  if( pSpace->FixedStageAdjust > 0 )
			  {
              snprintf( staging,  sizeof( staging ), WIDE("Up %d"), pSpace->FixedStageAdjust );
			  }
			  else
			  {
              snprintf( staging, sizeof( staging ), WIDE("Dn %d"), -pSpace->FixedStageAdjust );
			  }
			  PutDirectedText( Surface
								  , x+8, 29
								  , Surface->width, Surface->height
								  , Color( 0, 0, 0 ), AColor( 255,255,255, 32)
								  , dir
								  , staging );
		  }
        break;
    }
	case SPACE_HOLDERSMEETING:
    {
        snprintf( text, sizeof( text ), WIDE("%d:%d")
                    , pSpace->attributes.split.ratio.numerator
                    , pSpace->attributes.split.ratio.denominator );
        BlatColor( Surface, x, y, width, height, Color( 255,255,255 ) );
        PutDirectedText( Surface
                          , 5, 5, Surface->width, Surface->height
                          , Color( 0,0,1 ), 0
                          , dir
                          ,  text);
        break;
    }
	case SPACE_PROFESSION:
    {
        BlatColor( Surface, x, y, width, height, Color( 255,255,255 ) );
        PutDirectedText( Surface
							  , 5, 5, Surface->width, Surface->height
                          , Color( 0,0,0 ), AColor( 255,255,255, 32)
                        , dir
                            , pSpace->attributes.profession.name);
        snprintf( text, sizeof( text ), WIDE("Pays $%ld"), pSpace->attributes.profession.pay );
        PutDirectedText( Surface
							  , 5, 5 + GetFontHeight( NULL ), Surface->width, Surface->height
                          , Color( 0,0,0 ), AColor( 255,255,255, 32)
                        , dir
                            ,  text);
        {
            int ofs = 0, roll;
            ofs = snprintf( text, sizeof( text ), WIDE("On ") );
            for( roll = 1; roll <=12; roll++ )
                if( TESTFLAG( pSpace->attributes.profession.payon, roll ) )
                {
                    ofs += snprintf( text + ofs, sizeof( text ) - ofs, WIDE("%d"), roll++ );
                    break;
                }
         ofs += snprintf( text + ofs, sizeof( text )-ofs, WIDE(" or ") );
            for( ; roll <=12; roll++ )
                if( TESTFLAG( pSpace->attributes.profession.payon, roll ) )
                {
                    ofs += snprintf( text + ofs, sizeof( text ) - ofs, WIDE("%d"), roll++ );
                    break;
                }
        }
        PutDirectedText( Surface
							  , 5, 5 + GetFontHeight( NULL )*2, Surface->width, Surface->height
                          , Color( 0,0,0 ), AColor( 255,255,255, 32)
                          , dir
                            ,  text);
        break;
    }

	case SPACE_START:
    {
        BlatColor( Surface, x, y, width, height, Color( 255,255,255 ) );
		  PutDirectedText( Surface
							  , 8, 2, Surface->width, Surface->height
							  , Color( 255,0,0 ), AColor( 0,0,0, 32)
							  , dir
							  ,  WIDE("Start"));
		  break;
	 }
	case SPACE_QUIT:
		 {
			 BlatColor( Surface, x, y, width, height, Color( 255,255,255 ) );
			 PutDirectedText( Surface
								 , 10, 2, Surface->width, Surface->height
								 , Color( 255,0,0 ), AColor( 0,0,0, 32)
								 , dir
								 ,  WIDE("Quit"));
			 break;
		 }
	case SPACE_ROLL:
		 {
			 BlatColor( Surface, x, y, width, height, Color( 140,120, 43 ) );
			 PutDirectedText( Surface
								 , 10, 2, Surface->width, Surface->height
								 , Color( 255,255,255 ), AColor( 0,0,0, 32)
								 , dir
								 ,  WIDE("Roll"));
			 break;
		 }
	case SPACE_SELL:
		{
			BlatColor( Surface, x, y, width, height, Color( 180,150, 80 ) );
			PutDirectedText( Surface
								 , 10, 2, Surface->width, Surface->height
								 , Color( 255,255,255 ), AColor( 0,0,0, 32)
								 , dir
								 ,  WIDE("Sell"));
			break;
		}
	case SPACE_UNKNOWN:
		break;
	}
	return TRUE;
}

//---------------------------------------------------------------------------

void StartFlash( void )
{
	g.flags.bFlashing = 1;
   RescheduleTimerEx( g.nFlashTimer, 0 ); // flash NOW
}

//---------------------------------------------------------------------------

void StopFlash( void )
{
	if( g.flags.bFlashing )
	{
      // wait for a flash at least once...
		while( !g.flags.bFlashed )
			Idle();
		g.flags.bFlashing = 0;
		RescheduleTimerEx( g.nFlashTimer, 0 ); // flash NOW
      // wait for last flash...
		while( g.flags.bFlashed )
			Idle();
	}
}


//---------------------------------------------------------------------------

void CPROC FlashSpaces( uintptr_t psv )
{
	PSPACE pSpace;
	CDATA Border;
	Image Surface;
	INDEX idx;
   //lprintf( WIDE("FLASH!!!!!!!!!!") );
	g.flags.bFlashOn = !g.flags.bFlashOn;
	if( g.flags.bFlashing )
	{
		LIST_FORALL( g.PossibleSpaces, idx, PSPACE, pSpace )
		{
			lprintf( WIDE("It's a possible space...") );
			pSpace->flags.bFlashOn = g.flags.bFlashOn;
			pSpace->flags.bFlashing = 1;
         SmudgeCommon( pSpace->region );
		}
      lprintf( WIDE("How many psosible spaces was that?") );
      g.flags.bFlashed = 1;
	}
	else
	{
		if( g.flags.bFlashed )
		{
			LIST_FORALL( g.PossibleSpaces, idx, PSPACE, pSpace )
			{
            pSpace->flags.bFlashing = 0;
				SmudgeCommon( pSpace->region );
			}
			// so we don't get confused....
         lprintf( WIDE("Emptying list of possibles...\n") );
         EmptyList( &g.PossibleSpaces );
			g.flags.bFlashed = 0;
		}
	}
   //lprintf( WIDE("Doing smudge for token.") );
   // flash the player token...
	{
		static PPLAYER _player; // prior player peice...
		if( g.pCurrentPlayer != _player )
		{
			if( _player )
			{
            lprintf( WIDE("Update last player token") );
				SmudgeCommon( _player->pPlayerToken );
			}
         _player = g.pCurrentPlayer;
		}
		if( g.pCurrentPlayer )
		{
			lprintf( WIDE("Update this player's token.") );
			SmudgeCommon( g.pCurrentPlayer->pPlayerToken );
		}
	}
   //SmudgeCommon( g.board );
   //UpdateFrame( g.board, 0, 0, 0, 0 );
}

//-----------------------------------------------------------------

int CPROC MouseSpace( PCOMMON pc, int32_t x, int32_t y, uint32_t b )
{
   PSPACE  pSpace = (PSPACE)GetCommonUserData( pc );
    // hmm not sure how these buttons will work
    // and what I'm supposed to track... maybe alias some of the
	// combinations...
   static uint32_t _b;
    if( ( b & 1 ) && !( _b & 1 ) )
	 {
		 INDEX idx;
		 PSPACE pCheck;
		 switch( pSpace->type )
		 {
		 case SPACE_QUIT:
			 exit(0);
			 break;
		 case SPACE_ROLL:
			 if( g.flags.bSelectPlayer )
			 {
				 EnqueStrokes( WIDE("0") );
             return TRUE;
			 }
			 else
			 {
				 EnqueStrokes( WIDE("n") ); // otherwise is sell...
			 }
			 break;
		 case SPACE_SELL:
			 if( g.flags.bAllowSell )
             EnqueStrokes( WIDE("y") );
			 break;
		 default:
			 LIST_FORALL( g.PossibleSpaces, idx, PSPACE, pCheck )
			 {
				 if( pCheck == pSpace )
				 {
					 TEXTCHAR select[4];
					 snprintf( select, sizeof( select ), WIDE("%ld%s")
							  , idx + 1
							  , g.flags.bChoiceNeedsEnter?WIDE("\n"):WIDE("") );
					 EnqueStrokes( select );
					 break;
				 }
			 }
          break;
		 }
    }
	 _b = b;
    return TRUE;
}

//-----------------------------------------------------------------
#include <psi.h>
CONTROL_REGISTRATION board_space = { WIDE("StockMarket Board Space")
											  , { { 40, 40 }, sizeof( SPACE ), BORDER_NONE }
											  ,NULL
											  , NULL
											  , DrawSpace
											  , MouseSpace
};
PRELOAD( RegisterSpace ){ DoRegisterControl( &board_space ); };
//-----------------------------------------------------------------

void CreateBoardDisplay( void )
{
    INDEX idx;
	PSPACE pSpace;
	g.board = CreateFrame( WIDE("Stock Board"), 0, 0, 800, 768, BORDER_NORMAL, NULL );
	if( !g.board )
		return;
	g.Panel = MakeSheetControl( g.board
										, g.scale * 7
										, g.scale * 15
										, g.scale * 12
										, (g.scale * 13/2)
										, 0 );
	GetSheetSize( g.Panel, &g.PanelWidth, &g.PanelHeight );
    LIST_FORALL( g.Board, idx, PSPACE, pSpace )
	{
		 /*
        Log4( WIDE("Createing a region at : %d,%d %d by %d"), pSpace->position.x
                                , pSpace->position.y
                                , pSpace->position.width
										  , pSpace->position.height);
                                */
        pSpace->region = MakeControl( g.board, board_space.TypeID
                                                , pSpace->position.x * g.scale
                                                , pSpace->position.y * g.scale
                                                , pSpace->position.width * g.scale
                                              , pSpace->position.height * g.scale
												, 0 );
        SetCommonUserData( pSpace->region, (uintptr_t)pSpace );
        //SetControlDraw( pSpace->region, (RedrawCallback)DrawSpace, (uintptr_t)pSpace );
        //SetControlMouse( pSpace->region, (MouseCallback)MouseSpace, (uintptr_t)pSpace );
	}
	DisplayFrame( g.board );
}

//-----------------------------------------------------------------

void ReadBoardDefinitions( TEXTCHAR *filename )
{
	PCONFIG_HANDLER pch = CreateConfigurationEvaluator();
	AddConfiguration( pch, WIDE("Roll is random %b"), SetRandomRoll );
    AddConfiguration( pch, WIDE("center %m pays %i on %i or %i"),     AddCenter );
    AddConfiguration( pch, WIDE("space %i"), AddSpace );
	AddConfiguration( pch, WIDE("color %c"), SetSpaceColor );
    AddConfiguration( pch, WIDE("market up %i"), SetMarketUp );
    AddConfiguration( pch, WIDE("market down %i"), SetMarketDown );
	AddConfiguration( pch, WIDE("Stock %i"), SetStockBuy );
	AddConfiguration( pch, WIDE("do sell"), SetPlayerSell );
    AddConfiguration( pch, WIDE("roll"), SetRollDice );
    AddConfiguration( pch, WIDE("sell stock %i"), SetStockSell );
    AddConfiguration( pch, WIDE("broker"), SetSpaceBroker );
    AddConfiguration( pch, WIDE("left %i"), SetSpaceLeft );
    AddConfiguration( pch, WIDE("right %i"), SetSpaceRight );
    AddConfiguration( pch, WIDE("holders %i left"), SetHolderEntranceLeft );
    AddConfiguration( pch, WIDE("holders %i right"), SetHolderEntranceRight );
    AddConfiguration( pch, WIDE("leave left"), SetExitLeft );
    AddConfiguration( pch, WIDE("leave right"), SetExitRight );
	AddConfiguration( pch, WIDE("split %q"), SetStockSplit );
	AddConfiguration( pch, WIDE("start"), SetSpaceStart );
	AddConfiguration( pch, WIDE("size %i,%i"), SetSpaceSize );
	AddConfiguration( pch, WIDE("position %i, %i"), SetSpacePosition );
	AddConfiguration( pch, WIDE("align horizontal right"), SetHorizontalAlignment );
	AddConfiguration( pch, WIDE("align horizontal left"), SetHorizontalLeftAlignment );
	AddConfiguration( pch, WIDE("align vertical up"), SetVerticalUpAlignment );
	AddConfiguration( pch, WIDE("align vertical down"), SetVerticalDownAlignment );
	AddConfiguration( pch, WIDE("quit"), SetSpaceQuit );
    ProcessConfigurationFile( pch, filename, 0 );
    DestroyConfigurationEvaluator( pch );
    ValidateBoard();

    CreateBoardDisplay();
    g.nFlashTimer = AddTimer( 250, FlashSpaces, 0 );
}


void AddRollToPossible( void )
{
	INDEX idx;
   PSPACE pSpace;
	LIST_FORALL( g.Board, idx, PSPACE, pSpace )
	{
		if( pSpace->type == SPACE_ROLL )
		{
			AddLink( &g.PossibleSpaces, pSpace );
		}
	}
}

void AddSellToPossible( void )
{
	INDEX idx;
   PSPACE pSpace;
	LIST_FORALL( g.Board, idx, PSPACE, pSpace )
	{
		if( pSpace->type == SPACE_SELL )
		{
			AddLink( &g.PossibleSpaces, pSpace );
		}
	}
}

