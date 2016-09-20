
// oops for convienice we will include this.
#include "stockstruct.h"

void InitStockDialogs( void );
void BuySomeStock( PSTOCK pStock, uint32_t max_shares );
void StockBuyEnd( void );
void StockSellEnd( void );
int CPROC DrawPortfolio( PCONTROL pc );

int CPROC DrawStockBar( PCOMMON pc );

PSTOCKACCOUNT GetStockAccount( PORTFOLIO portfolio, PSTOCK stock );
uint32_t BuyStock( PORTFOLIO portfolio, PSTOCK stock, uint32_t shares );
uint32_t SellStock( PORTFOLIO portfolio
				 , PSTOCKACCOUNT pWhich
				 , int bForced
				 , uint32_t amount );
uint32_t CountShares( PORTFOLIO portfolio );

int ReadStockDefinitions( TEXTCHAR *filename );
PSTOCK GetStockByID( uint16_t ID );

void DumpStocks( void );
void UpdateStocks( int16_t stages );

uint32_t GetStockValueEx( PSTOCK pStock, int delta, int bMin );
uint32_t SellStocks( PORTFOLIO portfolio, uint32_t target, int bForced );
