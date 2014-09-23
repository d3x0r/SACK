
// oops for convienice we will include this.
#include "stockstruct.h"

void InitStockDialogs( void );
void BuySomeStock( PSTOCK pStock, _32 max_shares );
void StockBuyEnd( void );
void StockSellEnd( void );
int CPROC DrawPortfolio( PCONTROL pc );

int CPROC DrawStockBar( PCOMMON pc );

PSTOCKACCOUNT GetStockAccount( PORTFOLIO portfolio, PSTOCK stock );
_32 BuyStock( PORTFOLIO portfolio, PSTOCK stock, _32 shares );
_32 SellStock( PORTFOLIO portfolio
				 , PSTOCKACCOUNT pWhich
				 , int bForced
				 , _32 amount );
_32 CountShares( PORTFOLIO portfolio );

int ReadStockDefinitions( TEXTCHAR *filename );
PSTOCK GetStockByID( _16 ID );

void DumpStocks( void );
void UpdateStocks( S_16 stages );

_32 GetStockValueEx( PSTOCK pStock, int delta, int bMin );
_32 SellStocks( PORTFOLIO portfolio, _32 target, int bForced );
