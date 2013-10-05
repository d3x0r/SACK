
#include <stdhdrs.h>
#include <logging.h>
#include <sharemem.h>
#include <configscript.h>


#include "global.h"
#include "stockstruct.h"

#include "input.h"
#include "stocks.h"

//---------------------------------------------------------------------------

enum buy_controls {
	BTN_ADD1 = 100
						, BTN_ADD5
						, BTN_ADD10
						, BTN_ADD20
						, BTN_ADD50
						, BTN_ADD100
						, BTN_ADD500
						, BTN_ADD1000
						, CHK_SUBTRACT
                  , TXT_STOCK
						, TXT_PRICE
						, TXT_SHARES
						, TXT_SHARES_VALUE
						, TXT_TOTAL
						, TXT_CASH
						, TXT_YOUHAVE
						, TXT_WILLHAVE
						, TXT_SELL
						, TXT_REMAIN
                  , TXT_TARGET
						, TXT_SELL_VALUE
                  , TXT_REMAIN_VALUE
						, TXT_SELL_TARGET
                  , SHT_STOCKS // container
						, SHT_STOCK // page base
                  , SHT_STOCKMAX = SHT_STOCK + 8
};

typedef struct buy_stock_dialog_tag
{
	_32 max_shares;
   _32 have_shares;
   _32 cash;
	S_32 gain;
   _32 value;
   S_32 shares;
} BUY_STOCK_DIALOG;

BUY_STOCK_DIALOG buy;

typedef struct sell_stock_dialog_tag
{
	_32 sell[8];
	_32 value[8];
	_32 owned[8];
   _32 currentpage;
   _32 total;
   _32 target;
	S_32 gain;
} SELL_STOCK_DIALOG;
SELL_STOCK_DIALOG sell;

void CPROC AddSome( PTRSZVAL psv, PCONTROL button )
{
	PCONTROL pc;
	if( (int)psv < 0 )
	{
		buy.gain *= (int)psv;
		return;
	}
	buy.shares += buy.gain * psv;
	if( buy.shares < 0 )
		buy.shares = 0;
	if( buy.max_shares && buy.shares > buy.max_shares )
      buy.shares = buy.max_shares;
	if( buy.shares * buy.value > buy.cash )
      buy.shares = buy.cash / buy.value;
	{
		TEXTCHAR shares[10];
		pc = GetControl( g.BuyStocks, TXT_SHARES );
		snprintf( shares, 10, WIDE("%ld"), buy.shares );
		SetCommonText( pc, shares );
		pc = GetControl( g.BuyStocks, TXT_TOTAL );
		snprintf( shares, 10, WIDE("$%ld"), buy.shares * buy.value );
		SetCommonText( pc, shares );
		snprintf( shares, 10, WIDE("(%ld)"), buy.have_shares + buy.shares );
		SetCommonText( GetControl( g.BuyStocks, TXT_WILLHAVE ), shares );
	}
}

void CPROC DoSale( PTRSZVAL psv, PCONTROL button )
{
	TEXTCHAR strokes[12];
	if( buy.shares )
	{
		snprintf( strokes, 12, WIDE("%ld\nyn"), buy.shares );
		EnqueStrokes( strokes );
	}
   else
		EnqueStrokes( WIDE("0n") );
}

void CPROC CancelSale( PTRSZVAL psv, PCONTROL button )
{
   EnqueStrokes( WIDE("0y") );
}

void BuySomeStock( PSTOCK pStock, _32 max_shares )
{
	TEXTCHAR txt[10];
	PSTOCKACCOUNT pAccount;
	buy.shares = 0;
	buy.gain = 1;
	SetCommonText( GetControl( g.BuyStocks, TXT_STOCK ), pStock->name );
	buy.value = GetStockValue( pStock, FALSE );
	snprintf( txt, 10, WIDE("$%ld"), buy.value );
	SetCommonText( GetControl( g.BuyStocks, TXT_PRICE ), txt );
	SetCommonText( GetControl( g.BuyStocks, TXT_SHARES ), WIDE("0") );
	SetCommonText( GetControl( g.BuyStocks, TXT_TOTAL ), WIDE("$0") );
	snprintf( txt, 10, WIDE("$%ld"), buy.cash = g.pCurrentPlayer->Cash );
	SetCommonText( GetControl( g.BuyStocks, TXT_CASH ), txt );
	pAccount = GetStockAccount( &g.pCurrentPlayer->portfolio, pStock );
	snprintf( txt, 10, WIDE("%ld"), buy.have_shares = (pAccount?pAccount->shares:0) );
	SetCommonText( GetControl( g.BuyStocks, TXT_YOUHAVE ), txt );
	snprintf( txt, 10, WIDE("(%ld)"), buy.have_shares + buy.shares );
	SetCommonText( GetControl( g.BuyStocks, TXT_WILLHAVE ), txt );
	SetCheckState( GetControl( g.BuyStocks, CHK_SUBTRACT ), 0 );
	buy.max_shares = max_shares;
	DisableSheet( g.Panel, PANEL_BUY, FALSE );
	SetCurrentSheet( g.Panel, PANEL_BUY );
	//MountPanel( g.BuyStocks );
}

void StockBuyEnd( void )
{
   DisableSheet( g.Panel, PANEL_BUY, TRUE );
   //UnmountPanel( g.BuyStocks );
   UpdateCommon( g.Panel );
}

void UpdateTarget( void )
{
	if( sell.target )
	{
		INDEX idx;
		int n = 0;
		PSTOCK stock;
		PCONTROL pStocks = GetControl( g.SellStocks, SHT_STOCKS );
      printf( WIDE("Updating targets\n") );
		LIST_FORALL( g.Market.stocks, idx, PSTOCK, stock )
		{
			PCONTROL pc;
			TEXTCHAR text[16];
			snprintf( text, 16, WIDE("(%ld)"), ( ( sell.target - sell.total )
											  + (sell.value[n]-1) )
					                  / sell.value[n] );
			pc = (PCOMMON)GetSheetControl( pStocks, SHT_STOCK + n, TXT_SELL_TARGET );
			SetCommonText( pc, text );
			n++;
		}
	}
}

void CPROC AddSomeSell( PTRSZVAL psv, PCONTROL button )
{
	PCOMMON sheet = GetCurrentSheet( GetControl( g.SellStocks, SHT_STOCKS ) );
   _32 ID = GetControlID( (PCONTROL)sheet ) - SHT_STOCK;
	if( (int)psv < 0 )
	{
		sell.gain *= (int)psv;
		return;
	}
   // remove old from total
	sell.total -= sell.sell[ID] * sell.value[ID];
	sell.sell[ID] += sell.gain * psv;
	if( sell.sell[ID] < 0 )
		sell.sell[ID] = 0;
	if( sell.sell[ID] > sell.owned[ID] )
		sell.sell[ID] = sell.owned[ID];
   // add new back to total
   sell.total += sell.sell[ID] * sell.value[ID];
	UpdateTarget();
	{
		TEXTCHAR text[16];
      PCONTROL pc;
		snprintf( text, 16, WIDE("$%ld"), sell.total );
		pc = GetControl( g.SellStocks, TXT_TOTAL );
		SetCommonText( pc, text );

		snprintf( text, 16, WIDE("%ld"), sell.sell[ID] );
		pc = GetControl( sheet, TXT_SELL );
		SetCommonText( pc, text );

		snprintf( text, 16, WIDE("$%ld"), sell.sell[ID] * sell.value[ID] );
		pc = GetControl( sheet, TXT_SELL_VALUE );
		SetCommonText( pc, text );

		snprintf( text, 16, WIDE("%ld"), sell.owned[ID] - sell.sell[ID] );
		pc = GetControl( sheet, TXT_REMAIN );
		SetCommonText( pc, text );

		snprintf( text, 16, WIDE("$%ld"), (sell.owned[ID] - sell.sell[ID]) * sell.value[ID] );
		pc = GetControl( sheet, TXT_REMAIN_VALUE );
		SetCommonText( pc, text );
	}

}

int GetAccountIndex( PORTFOLIO portfolio, PSTOCK stock )
{
    PSTOCKACCOUNT pAccount;
	 INDEX idx;
    int n = 1;
    LIST_FORALL( *portfolio
                  , idx
                  , PSTOCKACCOUNT
                  , pAccount )
    {
        if( pAccount->stock == stock )
			  break;
        n++;
    }
   return n;

}

void CPROC DoSell( PTRSZVAL psv, PCONTROL button )
{
	int n;
	TEXTCHAR strokes[16];
	for( n = 0; n < 8; n++ )
	{
		if( sell.sell[n] )
		{
			snprintf( strokes, 16
					 , WIDE("%d\n%ld\ny")
					 , (struct stock_tag*)GetAccountIndex( &g.pCurrentPlayer->portfolio
											, GetStockByID( n + 1 ) )
					 , sell.sell[n] );
			EnqueStrokes( strokes );
		}
	}
   EnqueStrokes( WIDE("0") );
}

void CPROC CancelSell( PTRSZVAL psv, PCONTROL button )
{
   EnqueStrokes( WIDE("0") );
}

void StockSellStart( PORTFOLIO portfolio, _32 target, int bForced )
{
	INDEX idx;
   int n = 0;
	TEXTCHAR text[16];
	PSTOCKACCOUNT pAccount;
	PSTOCK stock;
   PCONTROL pStocks = GetControl( g.SellStocks, SHT_STOCKS );
	LIST_FORALL( g.Market.stocks, idx, PSTOCK, stock )
	{
		PCONTROL pc;
		sell.value[n] = GetStockValue( stock, bForced );
		pAccount = GetStockAccount( portfolio, stock );

		sell.owned[n] = pAccount?pAccount->shares:0;

		snprintf( text, 16, WIDE("%ld"), sell.owned[n] );
		pc = (PCOMMON)GetSheetControl( pStocks, SHT_STOCK + n, TXT_SHARES );
		SetCommonText( pc, text );
		pc = (PCOMMON)GetSheetControl( pStocks, SHT_STOCK + n, TXT_REMAIN );
		SetCommonText( pc, text );

		snprintf( text, 16, WIDE("$%ld"), sell.owned[n] * sell.value[n] );
		pc = (PCOMMON)GetSheetControl( pStocks, SHT_STOCK + n, TXT_SHARES_VALUE );
		SetCommonText( pc, text );
		pc = (PCOMMON)GetSheetControl( pStocks, SHT_STOCK + n, TXT_REMAIN_VALUE );
		SetCommonText( pc, text );

		sell.sell[n] = 0;
		pc = (PCOMMON)GetSheetControl( pStocks, SHT_STOCK + n, TXT_SELL );
		SetCommonText( pc, WIDE("0") );
		pc = (PCOMMON)GetSheetControl( pStocks, SHT_STOCK + n, TXT_SELL_TARGET );
		SetCommonText( pc, NULL );
		pc = (PCOMMON)GetSheetControl( pStocks, SHT_STOCK + n, TXT_SELL_VALUE );
		SetCommonText( pc, WIDE("$0") );
      n++;
	}
   sell.gain = 1;
	sell.target = target;
	sell.total = 0;
   snprintf( text, 16, WIDE("$%ld"), target );
   SetCommonText( GetControl( g.SellStocks, TXT_TARGET ), text );
   SetCommonText( GetControl( g.SellStocks, TXT_TOTAL ), WIDE("$0") );
	SetCheckState( GetControl( g.BuyStocks, CHK_SUBTRACT ), 0 );
	if( target )
		UpdateTarget();
	DisableSheet( g.Panel, PANEL_SELL, FALSE );
   SetCurrentSheet( g.Panel, PANEL_SELL );
	//MountPanel( g.SellStocks );
}

void StockSellEnd( void )
{
	DisableSheet( g.Panel, PANEL_SELL, TRUE );
   //UnmountPanel( g.SellStocks );
   UpdateCommon( g.Panel );
}

//---------------------------------------------------------------------------


int CPROC DrawStockBar( PCOMMON pc )
{
	Image Surface;
	INDEX idx;
	PSTOCK stock;
	int x, y, n;
   int colwidth;
	_32 width, height;
	Surface = GetControlSurface( pc );
	ClearImageTo( Surface, Color( 255, 255, 255 ) );
   n = 0;
   colwidth = Surface->width / g.Market.nStocks;
	LIST_FORALL( g.Market.stocks, idx, PSTOCK, stock )
	{
		TEXTCHAR symbol[5];
		TEXTCHAR value[6];

		x = ( Surface->width * n++ ) / g.Market.nStocks;
		y = 3;
		snprintf( symbol, 5, WIDE("%4.4s"), stock->Symbol );
		GetStringSize( symbol, &width, &height );
		PutString( Surface
					, x + ( ( colwidth - width ) / 2 ), y
					, Color( 0, 0, 0 ), 0, symbol );
		snprintf( value, 6, WIDE("%ld"), GetStockValueEx( stock, 1, FALSE ) );
		y += height + 4;
		GetStringSize( value, &width, &height );
		PutString( Surface
					, x + ( ( colwidth - width ) / 2 ), y
					, Color( 0, 0, 128 ), 0, value );

		y+=height;
		snprintf( value, 6, WIDE("%ld"), GetStockValueEx( stock, 0, FALSE ) );
		GetStringSize( value, &width, &height );
		PutString( Surface
					, x + ( ( colwidth - width ) / 2 ), y
					, Color( 0, 0, 0 ), 0, value );

		y+=height;
		snprintf( value, 6, WIDE("%ld"), GetStockValueEx( stock, -1, FALSE ) );
		GetStringSize( value, &width, &height );
		PutString( Surface
					, x + ( ( colwidth - width ) / 2 ), y
					, Color( 128, 0, 0 ), 0, value );

		y+=height;
		if( n > 1 )
		{
			do_vline( Surface, x, 0, y, Color( 0, 0, 0 ) );
		}
	}
	do_hline( Surface, 5 + height, 0, Surface->width, Color( 0, 0, 0 ) );
	return 0;
}

//---------------------------------------------------------------------------

PSTOCKACCOUNT GetStockAccount( PORTFOLIO portfolio, PSTOCK stock )
{
    PSTOCKACCOUNT pAccount;
    INDEX idx;
    LIST_FORALL( *portfolio
                  , idx
                  , PSTOCKACCOUNT
                  , pAccount )
    {
        if( pAccount->stock == stock )
            break;
    }
   return pAccount;
}

//---------------------------------------------------------------------------

int CPROC DrawPortfolio( PCONTROL pc )
{
	PLIST portfolio;
	Image Surface;
	INDEX idx;
	PSTOCK stock;
	int x, y, n;
   int colwidth;
	_32 width, height;
	if( !g.pCurrentPlayer )
      return 0;
   portfolio = g.pCurrentPlayer->portfolio;
	Surface = GetControlSurface( pc );
	ClearImageTo( Surface, Color( 255, 255, 255 ) );
   n = 0;
   colwidth = Surface->width / 4;
	LIST_FORALL( g.Market.stocks, idx, PSTOCK, stock )
	{
		TEXTCHAR symbol[5];
		TEXTCHAR value[6];
		PSTOCKACCOUNT pAccount =
			GetStockAccount( &g.pCurrentPlayer->portfolio
								, stock );
		x = ( Surface->width * (n-GetCommonUserData(pc)) ) / 4;
		if( pAccount &&
			pAccount->shares &&
			(n >= GetCommonUserData(pc)) &&
			(n < (GetCommonUserData(pc) + 4)) )
		{
			y = 2;
			snprintf( symbol, 5, WIDE("%4.4s"), stock->Symbol );
			GetStringSize( symbol, &width, &height );
			PutString( Surface
						, x + ( ( colwidth - width ) / 2 ), y
						, Color( 0, 0, 0 ), 0, symbol );

			y+=height + 2;
			snprintf( value, 6, WIDE("%ld"), pAccount->shares );
			GetStringSize( value, &width, &height );
			PutString( Surface
						, x + ( ( colwidth - width ) / 2 ), y
						, Color( 0, 0, 0 ), 0, value );

			y+=height;
			snprintf( value, 6, WIDE("%ld"), pAccount->shares
					  * GetStockValue( stock, FALSE ) );
			GetStringSize( value, &width, &height );
			PutString( Surface
						, x + ( ( colwidth - width ) / 2 ), y
						, Color( 0, 0, 0 ), 0, value );
		}
		else
		{
			y = 2;
			y+=height + 2;
			y+=height;
			y+=height;
		}
		if( n )
			do_vline( Surface, x, 0, y, Color( 0, 0, 0 ) );
		n++;
	}
   do_hline( Surface, 2 + height, 0, Surface->width, Color( 0, 0, 0 ) );

   return 1;

}

void ShowPortfolio( PORTFOLIO portfolio, int bForced, int bMenu )
{
    INDEX idx;
   int n = 1;
   PSTOCKACCOUNT stock;
    LIST_FORALL( *portfolio, idx, PSTOCKACCOUNT, stock )
    {
        if( bMenu )
         printf( WIDE("%d> "), n++ );
        printf( WIDE("%20s %5ld @ $%3ld $%ld\n")
                , stock->stock->name
                , stock->shares
                , GetStockValue( stock->stock, bForced )
            ,stock->shares
                * GetStockValue( stock->stock, bForced ) );
    }
}

//---------------------------------------------------------------------------

_32 SellStock( PORTFOLIO portfolio
                 , PSTOCKACCOUNT pWhich
                 , int bForced
                 , _32 amount )
{
   _32 cash = 0;
    INDEX idx;
   PSTOCKACCOUNT pAccount;
    LIST_FORALL( *portfolio, idx, PSTOCKACCOUNT, pAccount )
    {
        if( pWhich == pAccount )
        {
         pAccount->shares -= amount;
            cash += amount * GetStockValue( pAccount->stock, bForced );
            if( !pAccount->shares )
            {
                Release( pAccount );
                SetLink( portfolio, idx, NULL );
            }
        }
    }
   return cash;

}

//---------------------------------------------------------------------------

_32 CountShares( PORTFOLIO portfolio )
{
   _32 shares = 0;
    INDEX idx;
   PSTOCKACCOUNT pAccount;
    LIST_FORALL( *portfolio, idx, PSTOCKACCOUNT, pAccount )
    {
        shares += pAccount->shares;
    }
   return shares;
}

//---------------------------------------------------------------------------

_32 SellAllStocks( PORTFOLIO portfolio, int bForced ) // if forced use minprice
{
	_32 cash = 0;
	INDEX idx;
	PSTOCKACCOUNT pAccount;
	LIST_FORALL( *portfolio, idx, PSTOCKACCOUNT, pAccount )
	{
		cash += SellStock( portfolio, pAccount, bForced, pAccount->shares );
	}
	return cash;
}

//---------------------------------------------------------------------------

_32 SellStocks( PORTFOLIO portfolio, _32 target, int bForced )
{
	int n, count, yes;
	_32 cash = 0;
	INDEX idx;
	PSTOCKACCOUNT stock;
   StockSellStart( portfolio, target, bForced );
	do
	{
		ShowPortfolio( portfolio, bForced, TRUE );
		printf( WIDE("Which stock do you wish to sell? (0 to quit)") );
		n = GetANumber();
		if( !n )
			break;
		LIST_FORALL( *portfolio, idx, PSTOCKACCOUNT, stock )
		{
			if( !(--n) )
			{
				printf( WIDE("How much do you wish to sell?") );
				count = GetANumber();
				if( count > stock->shares )
					count = stock->shares;
				printf( WIDE("Are you sure you wish to sell %d shares at $%ld for %ld?")
						, count
						, GetStockValue( stock->stock, bForced )
						, count
						 * GetStockValue( stock->stock, bForced )
						);
				yes = GetYesNo();
				if( yes )
				{
					stock->shares -= count;
					cash +=  count
						* GetStockValue( stock->stock, bForced );
				}
			}
		}
	} while(1);
	StockSellEnd();
	printf( WIDE("Total cash from sales: $%ld"), cash );
	return cash;
}

//---------------------------------------------------------------------------

_32 BuyStock( PORTFOLIO portfolio, PSTOCK stock, _32 shares )
{
    PSTOCKACCOUNT pAccount = GetStockAccount( portfolio, stock );
    if( !pAccount )
    {
        pAccount = New( STOCKACCOUNT );
        pAccount->stock = stock;
        pAccount->shares = shares;
      SetLink( portfolio, stock->ID - 1, pAccount );
    }
    else
        pAccount->shares += shares;

   return shares * GetStockValue( stock, FALSE );
}

//---------------------------------------------------------------------------

PSTOCK GetStockByID( _16 ID )
{   
    INDEX idx;
    PSTOCK pStock;
    LIST_FORALL( g.Market.stocks, idx, PSTOCK, pStock )
    {
        if( pStock->ID == ID )
            return pStock;
    }
   return NULL;
}

//---------------------------------------------------------------------------

void UpdateStock( PSTOCK pStock, S_16 stages )
{
    if( pStock->flags.inverse )
        pStock->Stage -= stages;
    else
        pStock->Stage += stages;

    if( pStock->Stage > g.Market.stages )
        pStock->Stage = g.Market.stages - ( pStock->Stage - g.Market.stages );
    else if( pStock->Stage < -g.Market.stages )
		 pStock->Stage = -g.Market.stages - ( pStock->Stage + g.Market.stages );

}

//---------------------------------------------------------------------------

_32 GetStockValueEx( PSTOCK pStock, int delta, int bMin )
{
    _32 value = pStock->Baseline;
	 FRACTION tmp, tmp2;
    S_32 stage = pStock->Stage + delta;
    S_32 effectivestage;
    if( bMin )
      return pStock->Minimum;
	 //Log2( WIDE("%s stage is: %d"), pStock->name, pStock->Stage );
	 if( stage > g.Market.stages )
		 stage = g.Market.stages - ( stage - g.Market.stages );
	 else if( stage < -g.Market.stages )
       stage = -g.Market.stages - ( stage + g.Market.stages );
    if( stage >= g.Market.SecondStaging )
    {
        effectivestage = stage - g.Market.SecondStaging;
        //Log3( WIDE("%s Effective stage is: %d (%d)"), pStock->name, effectivestage, value );
        ScaleFraction( &tmp, g.Market.SecondStaging, pStock->Staging + 0 );
        ScaleFraction( &tmp2, effectivestage, pStock->Staging + 1 );
        value = pStock->Baseline + ReduceFraction( AddFractions( &tmp, &tmp2 ) );
    }
    else if( stage <= -g.Market.SecondStaging )
    {
        effectivestage = stage + g.Market.SecondStaging;
        //Log3( WIDE("%s Effective stage is: %d (%d)"), pStock->name, effectivestage, value );
        ScaleFraction( &tmp, -g.Market.SecondStaging, pStock->Staging + 0 );
        ScaleFraction( &tmp2, effectivestage, pStock->Staging + 1 );
        value = pStock->Baseline + ReduceFraction( AddFractions( &tmp, &tmp2 ) );
    }
    else
    {
        ScaleFraction( &tmp, stage, pStock->Staging + 0 );
        value = pStock->Baseline + ReduceFraction( &tmp );
    }
    //Log2( WIDE("%s value is: %d"), pStock->name, value );
    return value;
}

_32 GetStockValue( PSTOCK pStock, int bMin )
{
   return GetStockValueEx( pStock, 0, bMin );
}
//---------------------------------------------------------------------------

void DumpStocks( void )
{
    INDEX idx;
    PSTOCK pStock;
    PVARTEXT pvt;
    PTEXT text;
    pvt = VarTextCreate();
    LIST_FORALL( g.Market.stocks, idx, PSTOCK, pStock )
    {
        vtprintf( pvt, WIDE("%4.4s=%3d "), pStock->Symbol, GetStockValue( pStock, FALSE ) );
    }
    text = VarTextGet( pvt );
    printf( WIDE("%s\n"), GetText( text ) );
    LineRelease( text );
    VarTextDestroy( &pvt );
}

//---------------------------------------------------------------------------

void UpdateStocks( S_16 stages )
{
   PSTOCK pStock;
   INDEX idx;
   LIST_FORALL( g.Market.stocks, idx, PSTOCK, pStock )
   {
    UpdateStock( pStock, stages );
	}
   UpdateCommon( g.pStockBar );
   DumpStocks();
}

//---------------------------------------------------------------------------

PTRSZVAL CPROC ConfigStockLink( PTRSZVAL psv, arg_list args )
{
   PARAM( args, LOGICAL, yesno );
/*
    if( yesno )
    {
        Log( WIDE("stocks are linkd") );
    }
    else
        Log( WIDE("stocks are not linked") );
*/
    g.Market.flags.linked = yesno;
    return psv;
}

//---------------------------------------------------------------------------

PTRSZVAL CPROC ConfigStockLevels( PTRSZVAL psv, arg_list args )
{
   PARAM( args, _64, levels );
    //Log1( WIDE("Setting %d stages"), levels );
    g.Market.stages = levels;
    return psv;
}

PTRSZVAL CPROC SetSecondStaging( PTRSZVAL psv, arg_list args )
{
   PARAM( args, _64, SecondStaging );
    g.Market.SecondStaging = SecondStaging;
    return psv;
}

//---------------------------------------------------------------------------


PTRSZVAL CPROC BeginStock( PTRSZVAL psv, arg_list args )
{
   PARAM( args, TEXTSTR, pName );
    PSTOCK pStock;
    //Log1( WIDE("Beginning new stock: %s"), pName );
    if( psv )
	 {
       g.Market.nStocks++;
        AddLink( &g.Market.stocks, (POINTER)psv );
    }
    pStock = New( STOCK );
    MemSet( pStock, 0, sizeof( STOCK ) );
    strcpy( pStock->name, pName );
    return (PTRSZVAL)pStock;;
}

//---------------------------------------------------------------------------

PTRSZVAL CPROC SetStockId( PTRSZVAL psv, arg_list args )
{
   PARAM( args, S_64, ID );
    PSTOCK pStock = (PSTOCK)psv;
    Log1( WIDE("Set ID to %d"), ID );
    if( pStock )
        pStock->ID = (_16)ID;
    return psv;
}

//---------------------------------------------------------------------------

PTRSZVAL CPROC SetStockSymbol( PTRSZVAL psv, arg_list args )
{
   PARAM( args, TEXTSTR, pSymbol );
    if( psv )
    {
        PSTOCK pStock = (PSTOCK)psv;
        MemCpy( pStock->Symbol, pSymbol, 4 );
    }
    else
        Log( WIDE("There's no current stock being defined...") );
    return psv;
}

//---------------------------------------------------------------------------

PTRSZVAL CPROC SetStockMin( PTRSZVAL psv, arg_list args )
{
   PARAM( args, _64, min );
    if( psv )
    {
        PSTOCK pStock = (PSTOCK)psv;
        pStock->Minimum = min;
    }
    else
        Log( WIDE("There's no current stock being defined...") );
    return psv;
}

//---------------------------------------------------------------------------

PTRSZVAL CPROC SetStockStaging( PTRSZVAL psv, arg_list args )
{
    PARAM( args, FRACTION, staging1 );
    PARAM( args, FRACTION, staging2 );
    if( psv )
    {
        PSTOCK pStock = (PSTOCK)psv;
        pStock->Staging[0] = staging1;
        pStock->Staging[1] = staging2;
        /*
        Log2( WIDE("Setting staging to %d/%d")
                , staging1.numerator
                , staging1.denominator );
        */
    }
    else
        Log( WIDE("There's no current stock being defined...") );
    return psv;
}

//---------------------------------------------------------------------------

PTRSZVAL CPROC SetStockDividend( PTRSZVAL psv, arg_list args )
{
   PARAM( args, _64, dividend );
    if( psv )
    {
        PSTOCK pStock = (PSTOCK)psv;
        pStock->Dividend = dividend;
    }
    else
        Log( WIDE("There's no current stock being defined...") );
    return psv;
}

//---------------------------------------------------------------------------

PTRSZVAL CPROC SetStockColor( PTRSZVAL psv, arg_list args )
{
   PARAM( args, CDATA, color );
    if( psv )
    {
        PSTOCK pStock = (PSTOCK)psv;
        Log1( WIDE("Stock's color is: #%08X"), color );
        pStock->color = color;
    }
    else
        Log( WIDE("There's no current stock being defined...") );
    return psv;
}

//---------------------------------------------------------------------------

PTRSZVAL CPROC SetStockBaseline( PTRSZVAL psv, arg_list args )
{
   PARAM( args, _64, baseline );
    if( psv )
    {
        PSTOCK pStock = (PSTOCK)psv;
        pStock->Baseline = baseline;
    }
    else
        Log( WIDE("There's no current stock being defined...") );
    return psv;
}

//---------------------------------------------------------------------------

PTRSZVAL CPROC SetStockInversed( PTRSZVAL psv, arg_list args )
{
    if( psv )
    {
        PSTOCK pStock = (PSTOCK)psv;
        pStock->flags.inverse = 1;
    }
    else
        Log( WIDE("There's no current stock being defined...") );
    return psv;
}

//---------------------------------------------------------------------------

PTRSZVAL CPROC EndStockConfiguration( PTRSZVAL psv )
{
    if( psv )
    {
		 g.Market.nStocks++;
		 AddLink( &g.Market.stocks, (POINTER)psv );
    }
    return 0;
}
#include <psi.h>
CONTROL_REGISTRATION stock_bar = { WIDE("Stock Ticker")
											, { { 420, 180 }, 0, BORDER_INVERT|BORDER_THIN }
											, NULL
											, NULL
											, DrawStockBar
};
PRELOAD( RegisterStockBar ) { DoRegisterControl( &stock_bar ); }

//---------------------------------------------------------------------------
void InitStockDialogs( void )
{
	g.pStockBar = MakeControl( g.board, stock_bar.TypeID
										, g.scale * 7, g.scale * 5
										, g.scale * 6 * 2, g.scale * 3
										, 0 );
	//SetControlDraw( g.pStockBar, DrawStockBar, 0 );
	UpdateCommon( g.pStockBar );

	AddSheet( g.Panel, g.BuyStocks = CreateFrame( WIDE("Buy")
															  , 0, 0
															  , g.PanelWidth, g.PanelHeight
															  , BORDER_NOCAPTION|BORDER_NONE|BORDER_WITHIN, NULL ) );
	SetControlID( g.BuyStocks, PANEL_BUY );
   DisableSheet( g.Panel, PANEL_BUY, TRUE );
   MakeButton( g.BuyStocks, 5, g.scale * 4, 45, 22, BTN_ADD1, WIDE("1"), 0, AddSome, 1 );
   MakeButton( g.BuyStocks, 55, g.scale * 4, 45, 22, BTN_ADD5, WIDE("5"), 0, AddSome, 5 );
   MakeButton( g.BuyStocks, 105, g.scale * 4, 45, 22, BTN_ADD10, WIDE("10"), 0, AddSome, 10 );
   MakeButton( g.BuyStocks, 155, g.scale * 4, 45, 22, BTN_ADD20, WIDE("20"), 0, AddSome, 20 );
   MakeButton( g.BuyStocks, 5, g.scale * 4 + 24, 45, 22, BTN_ADD50, WIDE("50"), 0, AddSome, 50 );
   MakeButton( g.BuyStocks, 55, g.scale * 4 + 24, 45, 22, BTN_ADD100, WIDE("100"), 0, AddSome, 100 );
   MakeButton( g.BuyStocks, 105, g.scale * 4 + 24, 45, 22, BTN_ADD500, WIDE("500"), 0, AddSome, 500 );
	MakeButton( g.BuyStocks, 155, g.scale * 4 + 24, 45, 22, BTN_ADD1000, WIDE("1000"), 0, AddSome, 1000 );
	MakeCheckButton( g.BuyStocks, 5, g.scale * 4 - 21, 100, 16, CHK_SUBTRACT, WIDE("Subtract"), 0, AddSome, -1 );

   MakeTextControl( g.BuyStocks, 5, 5, 160, 16, TXT_STOCK, WIDE("International Shoes"), 0 );
   MakeTextControl( g.BuyStocks, 5, 22, 65, 16, TXT_PRICE, WIDE("$999"), 0 );
   MakeTextControl( g.BuyStocks, 72, 22, 75, 16, TXT_SHARES, WIDE("9999"), 0 );
	MakeTextControl( g.BuyStocks, 149, 22, 100, 16, TXT_TOTAL, WIDE("$99999"), 0 );
	MakeTextControl( g.BuyStocks, 5, 40, 65, 16, TXT_STATIC, WIDE("Cash"), 0 );
	MakeTextControl( g.BuyStocks, 72, 40, 75, 16, TXT_CASH, WIDE("$99999"), 0 );
	MakeTextControl( g.BuyStocks, 5, 58, 65, 16, TXT_STATIC, WIDE("You have:"), 0 );
   MakeTextControl( g.BuyStocks, 72, 58, 45, 16, TXT_YOUHAVE, WIDE("9999"), 0 );
   MakeTextControl( g.BuyStocks, 117, 58, 55, 16, TXT_WILLHAVE, WIDE("(9999)"), 0 );
   MakeButton( g.BuyStocks, 235, g.scale * 4, 55, 22, IDOK, WIDE("Okay"), 0, DoSale, 0 );
   MakeButton( g.BuyStocks, 235, g.scale * 4 + 24, 55, 22, IDCANCEL, WIDE("Done"), 0, CancelSale, 0 );


   AddSheet( g.Panel, g.SellStocks = CreateFrame( WIDE("Sell")
									 , 0, 0
									 , g.PanelWidth, g.PanelHeight
									  , BORDER_NOCAPTION|BORDER_NONE|BORDER_WITHIN, NULL ) );
	SetControlID( g.SellStocks, PANEL_SELL );
   DisableSheet( g.Panel, PANEL_SELL, TRUE );
	{
		PCONTROL sheet;
		INDEX idx;
		PSTOCK stock;
		_32 width, height;
		TEXTCHAR name[10];
		PCONTROL pc = MakeSheetControl( g.SellStocks, 4, 4
												, g.scale * 12 - 8 - 8, g.scale * 3 + 4
												, SHT_STOCKS );
		GetSheetSize( pc, &width, &height );
		LIST_FORALL( g.Market.stocks, idx, PSTOCK, stock )
		{
			snprintf( name, 10, WIDE("%4.4s"), stock->Symbol );
			sheet = CreateFrame( name
									 , 0, 0
									 , width, height
									 , BORDER_NOCAPTION|BORDER_NONE|BORDER_WITHIN, NULL );
			SetControlID( sheet, SHT_STOCK + idx );

			MakeTextControl( sheet, 5, 3, 160, 16, TXT_STOCK, stock->name, 0 );
			MakeTextControl( sheet, 5, 20, 65, 16, TXT_STATIC, WIDE("Shares"), 0 );
			MakeTextControl( sheet, 5, 36, 65, 16, TXT_STATIC, WIDE("Sell"), 0 );
			MakeTextControl( sheet, 5, 52, 65, 16, TXT_STATIC, WIDE("Remain"), 0 );
			MakeTextControl( sheet, 75, 20, 40, 16, TXT_SHARES, WIDE("0000"), 0 );
			MakeTextControl( sheet, 75, 37, 40, 16, TXT_SELL, WIDE("0"), 0 );
			MakeTextControl( sheet, 75, 52, 40, 16, TXT_REMAIN, WIDE("0"), 0 );
			MakeTextControl( sheet, 120, 36, 48, 16, TXT_SELL_TARGET, WIDE("(0000)"), 0 );
			MakeTextControl( sheet, 173, 20, 65, 16, TXT_SHARES_VALUE, WIDE("$0"), 0 );
			MakeTextControl( sheet, 173, 36, 65, 16, TXT_SELL_VALUE, WIDE("$0"), 0 );
			MakeTextControl( sheet, 173, 52, 65, 16, TXT_REMAIN_VALUE, WIDE("$0"), 0 );
			AddSheet( pc, sheet );
		}
		MakeTextControl( g.SellStocks
							, 5 + 95, g.scale * 3 + 7 + 4
                     , 40, 16, TXT_STATIC, WIDE("Total"), 0 );
		MakeTextControl( g.SellStocks
							, 50 + 95, g.scale * 3 + 7 + 4
							, 65, 16, TXT_TOTAL, WIDE("$000000"), 0 );
		MakeTextControl( g.SellStocks
							, 120 + 95, g.scale * 3 + 7 + 4
                     , 55, 16, TXT_STATIC, WIDE("Target"), 0 );
		MakeTextControl( g.SellStocks
							, 180 + 95, g.scale * 3 + 7 + 4
                     , 55, 16, TXT_TARGET, WIDE("$00000"), 0 );
		MakeButton( g.SellStocks, 5, g.scale * 4 + 1, 45, 21, BTN_ADD1, WIDE("1"), 0, AddSomeSell, 1 );
		MakeButton( g.SellStocks, 55, g.scale * 4 + 1, 45, 21, BTN_ADD5, WIDE("5"), 0, AddSomeSell, 5 );
		MakeButton( g.SellStocks, 105, g.scale * 4 + 1, 45, 21, BTN_ADD10, WIDE("10"), 0, AddSomeSell, 10 );
		MakeButton( g.SellStocks, 155, g.scale * 4 + 1, 45, 21, BTN_ADD20, WIDE("20"), 0, AddSomeSell, 20 );
		MakeButton( g.SellStocks, 5, g.scale * 4 + 24, 45, 21, BTN_ADD50, WIDE("50"), 0, AddSomeSell, 50 );
		MakeButton( g.SellStocks, 55, g.scale * 4 + 24, 45, 21, BTN_ADD100, WIDE("100"), 0, AddSomeSell, 100 );
		MakeButton( g.SellStocks, 105, g.scale * 4 + 24, 45, 21, BTN_ADD500, WIDE("500"), 0, AddSomeSell, 500 );
		MakeButton( g.SellStocks, 155, g.scale * 4 + 24, 45, 21, BTN_ADD1000, WIDE("1000"), 0, AddSomeSell, 1000 );
		MakeCheckButton( g.SellStocks, 5, g.scale * 4 - 18, 85, 16, CHK_SUBTRACT, WIDE("Subtract"), 0, AddSomeSell, -1 );
		MakeButton( g.SellStocks, 235, g.scale * 4, 55, 22, IDOK, WIDE("Okay"), 0, DoSell, 0 );
		MakeButton( g.SellStocks, 235, g.scale * 4 + 24, 55, 22, IDCANCEL, WIDE("Cancel"), 0, CancelSell, 0 );
	}
}

int ReadStockDefinitions( TEXTCHAR *filename )
{
    PCONFIG_HANDLER pch = CreateConfigurationEvaluator();
    // reload market.

    // need to figure out how to init the market...

   AddConfiguration( pch, WIDE("stocks %b linked"), ConfigStockLink );
    AddConfiguration( pch, WIDE("stocks have %i stages"), ConfigStockLevels );
    AddConfiguration( pch, WIDE("stock %m"), BeginStock );
    AddConfiguration( pch, WIDE("symbol %w"), SetStockSymbol );
    AddConfiguration( pch, WIDE("minimum %i"), SetStockMin );
    AddConfiguration( pch, WIDE("staging %q %q"), SetStockStaging );
    AddConfiguration( pch, WIDE("dividend %i"), SetStockDividend );
    AddConfiguration( pch, WIDE("baseline %i"), SetStockBaseline );
    AddConfiguration( pch, WIDE("inversed"), SetStockInversed );
    AddConfiguration( pch, WIDE("color %c"), SetStockColor );
    AddConfiguration( pch, WIDE("second staging starts at %i"), SetSecondStaging );
    AddConfiguration( pch, WIDE("ID %i"), SetStockId );
    SetConfigurationEndProc( pch, EndStockConfiguration );
   ProcessConfigurationFile( pch, filename, 0 );

	DestroyConfigurationEvaluator( pch );
	return 1;
}


