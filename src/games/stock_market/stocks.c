
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
	uint32_t max_shares;
   uint32_t have_shares;
   uint32_t cash;
	int32_t gain;
   uint32_t value;
   int32_t shares;
} BUY_STOCK_DIALOG;

BUY_STOCK_DIALOG buy;

typedef struct sell_stock_dialog_tag
{
	uint32_t sell[8];
	uint32_t value[8];
	uint32_t owned[8];
   uint32_t currentpage;
   uint32_t total;
   uint32_t target;
	int32_t gain;
} SELL_STOCK_DIALOG;
SELL_STOCK_DIALOG sell;

void CPROC AddSome( uintptr_t psv, PSI_CONTROL button )
{
	PSI_CONTROL pc;
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
		snprintf( shares, 10, "%ld", buy.shares );
		SetControlText( pc, shares );
		pc = GetControl( g.BuyStocks, TXT_TOTAL );
		snprintf( shares, 10, "$%ld", buy.shares * buy.value );
		SetControlText( pc, shares );
		snprintf( shares, 10, "(%ld)", buy.have_shares + buy.shares );
		SetControlText( GetControl( g.BuyStocks, TXT_WILLHAVE ), shares );
	}
}

void CPROC DoSale( uintptr_t psv, PSI_CONTROL button )
{
	TEXTCHAR strokes[12];
	if( buy.shares )
	{
		snprintf( strokes, 12, "%ld\nyn", buy.shares );
		EnqueStrokes( strokes );
	}
   else
		EnqueStrokes( "0n" );
}

void CPROC CancelSale( uintptr_t psv, PSI_CONTROL button )
{
   EnqueStrokes( "0y" );
}

void BuySomeStock( PSTOCK pStock, uint32_t max_shares )
{
	TEXTCHAR txt[10];
	PSTOCKACCOUNT pAccount;
	buy.shares = 0;
	buy.gain = 1;
	SetControlText( GetControl( g.BuyStocks, TXT_STOCK ), pStock->name );
	buy.value = GetStockValue( pStock, FALSE );
	snprintf( txt, 10, "$%ld", buy.value );
	SetControlText( GetControl( g.BuyStocks, TXT_PRICE ), txt );
	SetControlText( GetControl( g.BuyStocks, TXT_SHARES ), "0" );
	SetControlText( GetControl( g.BuyStocks, TXT_TOTAL ), "$0" );
	snprintf( txt, 10, "$%ld", buy.cash = g.pCurrentPlayer->Cash );
	SetControlText( GetControl( g.BuyStocks, TXT_CASH ), txt );
	pAccount = GetStockAccount( &g.pCurrentPlayer->portfolio, pStock );
	snprintf( txt, 10, "%ld", buy.have_shares = (pAccount?pAccount->shares:0) );
	SetControlText( GetControl( g.BuyStocks, TXT_YOUHAVE ), txt );
	snprintf( txt, 10, "(%ld)", buy.have_shares + buy.shares );
	SetControlText( GetControl( g.BuyStocks, TXT_WILLHAVE ), txt );
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
		PSI_CONTROL pStocks = GetControl( g.SellStocks, SHT_STOCKS );
      printf( "Updating targets\n" );
		LIST_FORALL( g.Market.stocks, idx, PSTOCK, stock )
		{
			PSI_CONTROL pc;
			TEXTCHAR text[16];
			snprintf( text, 16, "(%ld)", ( ( sell.target - sell.total )
											  + (sell.value[n]-1) )
					                  / sell.value[n] );
			pc = (PSI_CONTROL)GetSheetControl( pStocks, SHT_STOCK + n, TXT_SELL_TARGET );
			SetControlText( pc, text );
			n++;
		}
	}
}

void CPROC AddSomeSell( uintptr_t psv, PSI_CONTROL button )
{
	PSI_CONTROL sheet = GetCurrentSheet( GetControl( g.SellStocks, SHT_STOCKS ) );
   uint32_t ID = GetControlID( (PSI_CONTROL)sheet ) - SHT_STOCK;
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
      PSI_CONTROL pc;
		snprintf( text, 16, "$%ld", sell.total );
		pc = GetControl( g.SellStocks, TXT_TOTAL );
		SetControlText( pc, text );

		snprintf( text, 16, "%ld", sell.sell[ID] );
		pc = GetControl( sheet, TXT_SELL );
		SetControlText( pc, text );

		snprintf( text, 16, "$%ld", sell.sell[ID] * sell.value[ID] );
		pc = GetControl( sheet, TXT_SELL_VALUE );
		SetControlText( pc, text );

		snprintf( text, 16, "%ld", sell.owned[ID] - sell.sell[ID] );
		pc = GetControl( sheet, TXT_REMAIN );
		SetControlText( pc, text );

		snprintf( text, 16, "$%ld", (sell.owned[ID] - sell.sell[ID]) * sell.value[ID] );
		pc = GetControl( sheet, TXT_REMAIN_VALUE );
		SetControlText( pc, text );
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

void CPROC DoSell( uintptr_t psv, PSI_CONTROL button )
{
	int n;
	TEXTCHAR strokes[16];
	for( n = 0; n < 8; n++ )
	{
		if( sell.sell[n] )
		{
			snprintf( strokes, 16
					 , "%d\n%ld\ny"
					 , GetAccountIndex( &g.pCurrentPlayer->portfolio
											, GetStockByID( n + 1 ) )
					 , sell.sell[n] );
			EnqueStrokes( strokes );
		}
	}
   EnqueStrokes( "0" );
}

void CPROC CancelSell( uintptr_t psv, PSI_CONTROL button )
{
   EnqueStrokes( "0" );
}

void StockSellStart( PORTFOLIO portfolio, uint32_t target, int bForced )
{
	INDEX idx;
   int n = 0;
	TEXTCHAR text[16];
	PSTOCKACCOUNT pAccount;
	PSTOCK stock;
   PSI_CONTROL pStocks = GetControl( g.SellStocks, SHT_STOCKS );
	LIST_FORALL( g.Market.stocks, idx, PSTOCK, stock )
	{
		PSI_CONTROL pc;
		sell.value[n] = GetStockValue( stock, bForced );
		pAccount = GetStockAccount( portfolio, stock );

		sell.owned[n] = pAccount?pAccount->shares:0;

		snprintf( text, 16, "%ld", sell.owned[n] );
		pc = (PSI_CONTROL)GetSheetControl( pStocks, SHT_STOCK + n, TXT_SHARES );
		SetControlText( pc, text );
		pc = (PSI_CONTROL)GetSheetControl( pStocks, SHT_STOCK + n, TXT_REMAIN );
		SetControlText( pc, text );

		snprintf( text, 16, "$%ld", sell.owned[n] * sell.value[n] );
		pc = (PSI_CONTROL)GetSheetControl( pStocks, SHT_STOCK + n, TXT_SHARES_VALUE );
		SetControlText( pc, text );
		pc = (PSI_CONTROL)GetSheetControl( pStocks, SHT_STOCK + n, TXT_REMAIN_VALUE );
		SetControlText( pc, text );

		sell.sell[n] = 0;
		pc = (PSI_CONTROL)GetSheetControl( pStocks, SHT_STOCK + n, TXT_SELL );
		SetControlText( pc, "0" );
		pc = (PSI_CONTROL)GetSheetControl( pStocks, SHT_STOCK + n, TXT_SELL_TARGET );
		SetControlText( pc, NULL );
		pc = (PSI_CONTROL)GetSheetControl( pStocks, SHT_STOCK + n, TXT_SELL_VALUE );
		SetControlText( pc, "$0" );
      n++;
	}
   sell.gain = 1;
	sell.target = target;
	sell.total = 0;
   snprintf( text, 16, "$%ld", target );
   SetControlText( GetControl( g.SellStocks, TXT_TARGET ), text );
   SetControlText( GetControl( g.SellStocks, TXT_TOTAL ), "$0" );
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


int CPROC DrawStockBar( PSI_CONTROL pc )
{
	Image Surface;
	INDEX idx;
	PSTOCK stock;
	int x, y, n;
   int colwidth;
	uint32_t width, height;
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
		snprintf( symbol, 5, "%4.4s", stock->Symbol );
		GetStringSize( symbol, &width, &height );
		PutString( Surface
					, x + ( ( colwidth - width ) / 2 ), y, 0
					, Color( 0, 0, 0 ), 0, symbol );
		snprintf( value, 6, "%ld", GetStockValueEx( stock, 1, FALSE ) );
		y += height + 4;
		GetStringSize( value, &width, &height );
		PutString( Surface
					, x + ( ( colwidth - width ) / 2 ), y, 0
					, Color( 0, 0, 128 ), 0, value );

		y+=height;
		snprintf( value, 6, "%ld", GetStockValueEx( stock, 0, FALSE ) );
		GetStringSize( value, &width, &height );
		PutString( Surface
					, x + ( ( colwidth - width ) / 2 ), y, 0
					, Color( 0, 0, 0 ), 0, value );

		y+=height;
		snprintf( value, 6, "%ld", GetStockValueEx( stock, -1, FALSE ) );
		GetStringSize( value, &width, &height );
		PutString( Surface
					, x + ( ( colwidth - width ) / 2 ), y, 0
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

int CPROC DrawPortfolio( PSI_CONTROL pc )
{
	PLIST portfolio;
	Image Surface;
	INDEX idx;
	PSTOCK stock;
	int x, y, n;
   int colwidth;
	uint32_t width, height;
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
		x = ( Surface->width * (n- GetControlUserData(pc)) ) / 4;
		if( pAccount &&
			pAccount->shares &&
			(n >= GetControlUserData(pc)) &&
			(n < (GetControlUserData(pc) + 4)) )
		{
			y = 2;
			snprintf( symbol, 5, "%4.4s", stock->Symbol );
			GetStringSize( symbol, &width, &height );
			PutString( Surface
						, x + ( ( colwidth - width ) / 2 ), y, 0
						, Color( 0, 0, 0 ), 0, symbol );

			y+=height + 2;
			snprintf( value, 6, "%ld", pAccount->shares );
			GetStringSize( value, &width, &height );
			PutString( Surface
						, x + ( ( colwidth - width ) / 2 ), y, 0
						, Color( 0, 0, 0 ), 0, value );

			y+=height;
			snprintf( value, 6, "%ld", pAccount->shares
					  * GetStockValue( stock, FALSE ) );
			GetStringSize( value, &width, &height );
			PutString( Surface
						, x + ( ( colwidth - width ) / 2 ), y, 0
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
         printf( "%d> ", n++ );
        printf( "%20s %5ld @ $%3ld $%ld\n"
                , stock->stock->name
                , stock->shares
                , GetStockValue( stock->stock, bForced )
            ,stock->shares
                * GetStockValue( stock->stock, bForced ) );
    }
}

//---------------------------------------------------------------------------

uint32_t SellStock( PORTFOLIO portfolio
                 , PSTOCKACCOUNT pWhich
                 , int bForced
                 , uint32_t amount )
{
   uint32_t cash = 0;
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

uint32_t CountShares( PORTFOLIO portfolio )
{
   uint32_t shares = 0;
    INDEX idx;
   PSTOCKACCOUNT pAccount;
    LIST_FORALL( *portfolio, idx, PSTOCKACCOUNT, pAccount )
    {
        shares += pAccount->shares;
    }
   return shares;
}

//---------------------------------------------------------------------------

uint32_t SellAllStocks( PORTFOLIO portfolio, int bForced ) // if forced use minprice
{
	uint32_t cash = 0;
	INDEX idx;
	PSTOCKACCOUNT pAccount;
	LIST_FORALL( *portfolio, idx, PSTOCKACCOUNT, pAccount )
	{
		cash += SellStock( portfolio, pAccount, bForced, pAccount->shares );
	}
	return cash;
}

//---------------------------------------------------------------------------

uint32_t SellStocks( PORTFOLIO portfolio, uint32_t target, int bForced )
{
	int n, count, yes;
	uint32_t cash = 0;
	INDEX idx;
	PSTOCKACCOUNT stock;
   StockSellStart( portfolio, target, bForced );
	do
	{
		ShowPortfolio( portfolio, bForced, TRUE );
		printf( "Which stock do you wish to sell? (0 to quit)" );
		n = GetANumber();
		if( !n )
			break;
		LIST_FORALL( *portfolio, idx, PSTOCKACCOUNT, stock )
		{
			if( !(--n) )
			{
				printf( "How much do you wish to sell?" );
				count = GetANumber();
				if( count > stock->shares )
					count = stock->shares;
				printf( "Are you sure you wish to sell %d shares at $%ld for %ld?"
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
	printf( "Total cash from sales: $%ld", cash );
	return cash;
}

//---------------------------------------------------------------------------

uint32_t BuyStock( PORTFOLIO portfolio, PSTOCK stock, uint32_t shares )
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

PSTOCK GetStockByID( uint16_t ID )
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

void UpdateStock( PSTOCK pStock, int16_t stages )
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

uint32_t GetStockValueEx( PSTOCK pStock, int delta, int bMin )
{
    uint32_t value = pStock->Baseline;
	 FRACTION tmp, tmp2;
    int32_t stage = pStock->Stage + delta;
    int32_t effectivestage;
    if( bMin )
      return pStock->Minimum;
	 //Log2( "%s stage is: %d", pStock->name, pStock->Stage );
	 if( stage > g.Market.stages )
		 stage = g.Market.stages - ( stage - g.Market.stages );
	 else if( stage < -g.Market.stages )
       stage = -g.Market.stages - ( stage + g.Market.stages );
    if( stage >= g.Market.SecondStaging )
    {
        effectivestage = stage - g.Market.SecondStaging;
        //Log3( "%s Effective stage is: %d (%d)", pStock->name, effectivestage, value );
        ScaleFraction( &tmp, g.Market.SecondStaging, pStock->Staging + 0 );
        ScaleFraction( &tmp2, effectivestage, pStock->Staging + 1 );
        value = pStock->Baseline + ReduceFraction( AddFractions( &tmp, &tmp2 ) );
    }
    else if( stage <= -g.Market.SecondStaging )
    {
        effectivestage = stage + g.Market.SecondStaging;
        //Log3( "%s Effective stage is: %d (%d)", pStock->name, effectivestage, value );
        ScaleFraction( &tmp, -g.Market.SecondStaging, pStock->Staging + 0 );
        ScaleFraction( &tmp2, effectivestage, pStock->Staging + 1 );
        value = pStock->Baseline + ReduceFraction( AddFractions( &tmp, &tmp2 ) );
    }
    else
    {
        ScaleFraction( &tmp, stage, pStock->Staging + 0 );
        value = pStock->Baseline + ReduceFraction( &tmp );
    }
    //Log2( "%s value is: %d", pStock->name, value );
    return value;
}

uint32_t GetStockValue( PSTOCK pStock, int bMin )
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
        vtprintf( pvt, "%4.4s=%3d ", pStock->Symbol, GetStockValue( pStock, FALSE ) );
    }
    text = VarTextGet( pvt );
    printf( "%s\n", GetText( text ) );
    LineRelease( text );
    VarTextDestroy( &pvt );
}

//---------------------------------------------------------------------------

void UpdateStocks( int16_t stages )
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

uintptr_t CPROC ConfigStockLink( uintptr_t psv, arg_list args )
{
   PARAM( args, LOGICAL, yesno );
/*
    if( yesno )
    {
        Log( "stocks are linkd" );
    }
    else
        Log( "stocks are not linked" );
*/
    g.Market.flags.linked = yesno;
    return psv;
}

//---------------------------------------------------------------------------

uintptr_t CPROC ConfigStockLevels( uintptr_t psv, arg_list args )
{
   PARAM( args, uint64_t, levels );
    //Log1( "Setting %d stages", levels );
    g.Market.stages = levels;
    return psv;
}

uintptr_t CPROC SetSecondStaging( uintptr_t psv, arg_list args )
{
   PARAM( args, uint64_t, SecondStaging );
    g.Market.SecondStaging = SecondStaging;
    return psv;
}

//---------------------------------------------------------------------------


uintptr_t CPROC BeginStock( uintptr_t psv, arg_list args )
{
   PARAM( args, TEXTSTR, pName );
    PSTOCK pStock;
    //Log1( "Beginning new stock: %s", pName );
    if( psv )
	 {
       g.Market.nStocks++;
        AddLink( &g.Market.stocks, (POINTER)psv );
    }
    pStock = New( STOCK );
    MemSet( pStock, 0, sizeof( STOCK ) );
    strcpy( pStock->name, pName );
    return (uintptr_t)pStock;;
}

//---------------------------------------------------------------------------

uintptr_t CPROC SetStockId( uintptr_t psv, arg_list args )
{
   PARAM( args, int64_t, ID );
    PSTOCK pStock = (PSTOCK)psv;
    Log1( "Set ID to %d", ID );
    if( pStock )
        pStock->ID = (uint16_t)ID;
    return psv;
}

//---------------------------------------------------------------------------

uintptr_t CPROC SetStockSymbol( uintptr_t psv, arg_list args )
{
   PARAM( args, TEXTSTR, pSymbol );
    if( psv )
    {
        PSTOCK pStock = (PSTOCK)psv;
        MemCpy( pStock->Symbol, pSymbol, 4 );
    }
    else
        Log( "There's no current stock being defined..." );
    return psv;
}

//---------------------------------------------------------------------------

uintptr_t CPROC SetStockMin( uintptr_t psv, arg_list args )
{
   PARAM( args, uint64_t, min );
    if( psv )
    {
        PSTOCK pStock = (PSTOCK)psv;
        pStock->Minimum = min;
    }
    else
        Log( "There's no current stock being defined..." );
    return psv;
}

//---------------------------------------------------------------------------

uintptr_t CPROC SetStockStaging( uintptr_t psv, arg_list args )
{
    PARAM( args, FRACTION, staging1 );
    PARAM( args, FRACTION, staging2 );
    if( psv )
    {
        PSTOCK pStock = (PSTOCK)psv;
        pStock->Staging[0] = staging1;
        pStock->Staging[1] = staging2;
        /*
        Log2( "Setting staging to %d/%d"
                , staging1.numerator
                , staging1.denominator );
        */
    }
    else
        Log( "There's no current stock being defined..." );
    return psv;
}

//---------------------------------------------------------------------------

uintptr_t CPROC SetStockDividend( uintptr_t psv, arg_list args )
{
   PARAM( args, uint64_t, dividend );
    if( psv )
    {
        PSTOCK pStock = (PSTOCK)psv;
        pStock->Dividend = dividend;
    }
    else
        Log( "There's no current stock being defined..." );
    return psv;
}

//---------------------------------------------------------------------------

uintptr_t CPROC SetStockColor( uintptr_t psv, arg_list args )
{
   PARAM( args, CDATA, color );
    if( psv )
    {
        PSTOCK pStock = (PSTOCK)psv;
        Log1( "Stock's color is: #%08X", color );
        pStock->color = color;
    }
    else
        Log( "There's no current stock being defined..." );
    return psv;
}

//---------------------------------------------------------------------------

uintptr_t CPROC SetStockBaseline( uintptr_t psv, arg_list args )
{
   PARAM( args, uint64_t, baseline );
    if( psv )
    {
        PSTOCK pStock = (PSTOCK)psv;
        pStock->Baseline = baseline;
    }
    else
        Log( "There's no current stock being defined..." );
    return psv;
}

//---------------------------------------------------------------------------

uintptr_t CPROC SetStockInversed( uintptr_t psv, arg_list args )
{
    if( psv )
    {
        PSTOCK pStock = (PSTOCK)psv;
        pStock->flags.inverse = 1;
    }
    else
        Log( "There's no current stock being defined..." );
    return psv;
}

//---------------------------------------------------------------------------

uintptr_t CPROC EndStockConfiguration( uintptr_t psv )
{
    if( psv )
    {
		 g.Market.nStocks++;
		 AddLink( &g.Market.stocks, (POINTER)psv );
    }
    return 0;
}
#include <psi.h>
CONTROL_REGISTRATION stock_bar = { "Stock Ticker"
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

	AddSheet( g.Panel, g.BuyStocks = CreateFrame( "Buy"
															  , 0, 0
															  , g.PanelWidth, g.PanelHeight
															  , BORDER_NOCAPTION|BORDER_NONE|BORDER_WITHIN, NULL ) );
	SetControlID( g.BuyStocks, PANEL_BUY );
   DisableSheet( g.Panel, PANEL_BUY, TRUE );
   MakeButton( g.BuyStocks, 5, g.scale * 4, 45, 22, BTN_ADD1, "1", 0, AddSome, 1 );
   MakeButton( g.BuyStocks, 55, g.scale * 4, 45, 22, BTN_ADD5, "5", 0, AddSome, 5 );
   MakeButton( g.BuyStocks, 105, g.scale * 4, 45, 22, BTN_ADD10, "10", 0, AddSome, 10 );
   MakeButton( g.BuyStocks, 155, g.scale * 4, 45, 22, BTN_ADD20, "20", 0, AddSome, 20 );
   MakeButton( g.BuyStocks, 5, g.scale * 4 + 24, 45, 22, BTN_ADD50, "50", 0, AddSome, 50 );
   MakeButton( g.BuyStocks, 55, g.scale * 4 + 24, 45, 22, BTN_ADD100, "100", 0, AddSome, 100 );
   MakeButton( g.BuyStocks, 105, g.scale * 4 + 24, 45, 22, BTN_ADD500, "500", 0, AddSome, 500 );
	MakeButton( g.BuyStocks, 155, g.scale * 4 + 24, 45, 22, BTN_ADD1000, "1000", 0, AddSome, 1000 );
	MakeCheckButton( g.BuyStocks, 5, g.scale * 4 - 21, 100, 16, CHK_SUBTRACT, "Subtract", 0, AddSome, -1 );

   MakeTextControl( g.BuyStocks, 5, 5, 160, 16, TXT_STOCK, "International Shoes", 0 );
   MakeTextControl( g.BuyStocks, 5, 22, 65, 16, TXT_PRICE, "$999", 0 );
   MakeTextControl( g.BuyStocks, 72, 22, 75, 16, TXT_SHARES, "9999", 0 );
	MakeTextControl( g.BuyStocks, 149, 22, 100, 16, TXT_TOTAL, "$99999", 0 );
	MakeTextControl( g.BuyStocks, 5, 40, 65, 16, TXT_STATIC, "Cash", 0 );
	MakeTextControl( g.BuyStocks, 72, 40, 75, 16, TXT_CASH, "$99999", 0 );
	MakeTextControl( g.BuyStocks, 5, 58, 65, 16, TXT_STATIC, "You have:", 0 );
   MakeTextControl( g.BuyStocks, 72, 58, 45, 16, TXT_YOUHAVE, "9999", 0 );
   MakeTextControl( g.BuyStocks, 117, 58, 55, 16, TXT_WILLHAVE, "(9999)", 0 );
   MakeButton( g.BuyStocks, 235, g.scale * 4, 55, 22, IDOK, "Okay", 0, DoSale, 0 );
   MakeButton( g.BuyStocks, 235, g.scale * 4 + 24, 55, 22, IDCANCEL, "Done", 0, CancelSale, 0 );


   AddSheet( g.Panel, g.SellStocks = CreateFrame( "Sell"
									 , 0, 0
									 , g.PanelWidth, g.PanelHeight
									  , BORDER_NOCAPTION|BORDER_NONE|BORDER_WITHIN, NULL ) );
	SetControlID( g.SellStocks, PANEL_SELL );
   DisableSheet( g.Panel, PANEL_SELL, TRUE );
	{
		PSI_CONTROL sheet;
		INDEX idx;
		PSTOCK stock;
		uint32_t width, height;
		TEXTCHAR name[10];
		PSI_CONTROL pc = MakeSheetControl( g.SellStocks, 4, 4
												, g.scale * 12 - 8 - 8, g.scale * 3 + 4
												, SHT_STOCKS );
		GetSheetSize( pc, &width, &height );
		LIST_FORALL( g.Market.stocks, idx, PSTOCK, stock )
		{
			snprintf( name, 10, "%4.4s", stock->Symbol );
			sheet = CreateFrame( name
									 , 0, 0
									 , width, height
									 , BORDER_NOCAPTION|BORDER_NONE|BORDER_WITHIN, NULL );
			SetControlID( sheet, SHT_STOCK + idx );

			MakeTextControl( sheet, 5, 3, 160, 16, TXT_STOCK, stock->name, 0 );
			MakeTextControl( sheet, 5, 20, 65, 16, TXT_STATIC, "Shares", 0 );
			MakeTextControl( sheet, 5, 36, 65, 16, TXT_STATIC, "Sell", 0 );
			MakeTextControl( sheet, 5, 52, 65, 16, TXT_STATIC, "Remain", 0 );
			MakeTextControl( sheet, 75, 20, 40, 16, TXT_SHARES, "0000", 0 );
			MakeTextControl( sheet, 75, 37, 40, 16, TXT_SELL, "0", 0 );
			MakeTextControl( sheet, 75, 52, 40, 16, TXT_REMAIN, "0", 0 );
			MakeTextControl( sheet, 120, 36, 48, 16, TXT_SELL_TARGET, "(0000)", 0 );
			MakeTextControl( sheet, 173, 20, 65, 16, TXT_SHARES_VALUE, "$0", 0 );
			MakeTextControl( sheet, 173, 36, 65, 16, TXT_SELL_VALUE, "$0", 0 );
			MakeTextControl( sheet, 173, 52, 65, 16, TXT_REMAIN_VALUE, "$0", 0 );
			AddSheet( pc, sheet );
		}
		MakeTextControl( g.SellStocks
							, 5 + 95, g.scale * 3 + 7 + 4
                     , 40, 16, TXT_STATIC, "Total", 0 );
		MakeTextControl( g.SellStocks
							, 50 + 95, g.scale * 3 + 7 + 4
							, 65, 16, TXT_TOTAL, "$000000", 0 );
		MakeTextControl( g.SellStocks
							, 120 + 95, g.scale * 3 + 7 + 4
                     , 55, 16, TXT_STATIC, "Target", 0 );
		MakeTextControl( g.SellStocks
							, 180 + 95, g.scale * 3 + 7 + 4
                     , 55, 16, TXT_TARGET, "$00000", 0 );
		MakeButton( g.SellStocks, 5, g.scale * 4 + 1, 45, 21, BTN_ADD1, "1", 0, AddSomeSell, 1 );
		MakeButton( g.SellStocks, 55, g.scale * 4 + 1, 45, 21, BTN_ADD5, "5", 0, AddSomeSell, 5 );
		MakeButton( g.SellStocks, 105, g.scale * 4 + 1, 45, 21, BTN_ADD10, "10", 0, AddSomeSell, 10 );
		MakeButton( g.SellStocks, 155, g.scale * 4 + 1, 45, 21, BTN_ADD20, "20", 0, AddSomeSell, 20 );
		MakeButton( g.SellStocks, 5, g.scale * 4 + 24, 45, 21, BTN_ADD50, "50", 0, AddSomeSell, 50 );
		MakeButton( g.SellStocks, 55, g.scale * 4 + 24, 45, 21, BTN_ADD100, "100", 0, AddSomeSell, 100 );
		MakeButton( g.SellStocks, 105, g.scale * 4 + 24, 45, 21, BTN_ADD500, "500", 0, AddSomeSell, 500 );
		MakeButton( g.SellStocks, 155, g.scale * 4 + 24, 45, 21, BTN_ADD1000, "1000", 0, AddSomeSell, 1000 );
		MakeCheckButton( g.SellStocks, 5, g.scale * 4 - 18, 85, 16, CHK_SUBTRACT, "Subtract", 0, AddSomeSell, -1 );
		MakeButton( g.SellStocks, 235, g.scale * 4, 55, 22, IDOK, "Okay", 0, DoSell, 0 );
		MakeButton( g.SellStocks, 235, g.scale * 4 + 24, 55, 22, IDCANCEL, "Cancel", 0, CancelSell, 0 );
	}
}

int ReadStockDefinitions( TEXTCHAR *filename )
{
    PCONFIG_HANDLER pch = CreateConfigurationEvaluator();
    // reload market.

    // need to figure out how to init the market...

   AddConfiguration( pch, "stocks %b linked", ConfigStockLink );
    AddConfiguration( pch, "stocks have %i stages", ConfigStockLevels );
    AddConfiguration( pch, "stock %m", BeginStock );
    AddConfiguration( pch, "symbol %w", SetStockSymbol );
    AddConfiguration( pch, "minimum %i", SetStockMin );
    AddConfiguration( pch, "staging %q %q", SetStockStaging );
    AddConfiguration( pch, "dividend %i", SetStockDividend );
    AddConfiguration( pch, "baseline %i", SetStockBaseline );
    AddConfiguration( pch, "inversed", SetStockInversed );
    AddConfiguration( pch, "color %c", SetStockColor );
    AddConfiguration( pch, "second staging starts at %i", SetSecondStaging );
    AddConfiguration( pch, "ID %i", SetStockId );
    SetConfigurationEndProc( pch, EndStockConfiguration );
   ProcessConfigurationFile( pch, filename, 0 );

	DestroyConfigurationEvaluator( pch );
	return 1;
}


