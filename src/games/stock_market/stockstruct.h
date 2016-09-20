
#ifndef STOCKS_DEFINED
#define STOCKS_DEFINED
#include <sack_types.h>
#include <colordef.h>
#include <fractions.h>


typedef struct stock_tag {
	struct    stock_tag *next;
	struct    stock_tag **me;
	TEXTCHAR      name[64];
	TEXTCHAR      Symbol[4]; // 4 characters NO terminator.
	struct {
		uint32_t inverse : 1;
	} flags;
	CDATA     color;
   FRACTION  Staging[2];
   int16_t      Stage;
	uint16_t       Baseline;
	uint16_t       Minimum;
	uint16_t       Dividend;
	uint16_t       ID;
} STOCK, *PSTOCK;


typedef struct stock_account_tag {
	PSTOCK stock;
	uint32_t    shares;
} STOCKACCOUNT, *PSTOCKACCOUNT;

typedef PLIST *PORTFOLIO;

typedef struct market_tag {
	struct {
		uint32_t linked : 1;
	} flags;
	int16_t SecondStaging; // when second staging factor applied
	int32_t stages;
   int32_t nStocks;
	PLIST stocks; // list of PSTOCK structures...
} MARKET, *PMARKET;

uint32_t GetStockValue( PSTOCK pStock, int bMin );
uint32_t SellAllStocks( PORTFOLIO portfolio, int bForced ); // if forced use minprice

void ShowPortfolio( PORTFOLIO portfolio, int bForced, int bMenu );

//typedef struct 
#endif
//--------------------------------------------------------------------------
// $Log: stockstruct.h,v $
// Revision 1.5  2003/11/30 03:08:31  panther
// Looks like all dialogs done, except original player config.
//
// Revision 1.4  2003/11/29 04:27:18  panther
// Buy, dice dialogs done.  Fixed min sell. Left : player stat, sell
//
// Revision 1.3  2003/11/28 20:57:00  panther
// Almost done - just checkpoint in case of bad things
//
// Revision 1.2  2003/11/28 05:20:44  panther
// Invoke some motion on the board, fix stock paths, implement much - all text
//
// Revision 1.1.1.1  2002/10/09 13:29:18  panther
// Initial commit
//
//
