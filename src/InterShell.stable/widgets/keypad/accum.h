

#ifndef ACCUMULATOR_STRUCTURE_DEFINED
typedef struct accumulator_tag *PACCUMULATOR;
#endif


#define ACCUM_DOLLARS 0x0001
#define ACCUM_DECIMAL 0x0002
#define ACCUM_TEXT 0x0004

PACCUMULATOR GetAccumulator( CTEXTSTR name, uint32_t flags );
size_t GetAccumulatorText( PACCUMULATOR accum, TEXTCHAR *text, int nLen );
PACCUMULATOR SetAccumulator( PACCUMULATOR accum, int64_t value );
int64_t GetAccumulatorValue( PACCUMULATOR accum );
PACCUMULATOR AddAcummulator( PACCUMULATOR accum_dest, PACCUMULATOR accum_source );
PACCUMULATOR TransferAccumluator( PACCUMULATOR accum_dest, PACCUMULATOR accum_source );
void KeyTextIntoAccumulator( PACCUMULATOR, CTEXTSTR value ); // works with numeric, if value contains numeric characters
void KeyIntoAccumulator( PACCUMULATOR accum, int64_t val, uint32_t base );
void KeyDecimalIntoAccumulator( PACCUMULATOR accum );
void ClearAccumulator( PACCUMULATOR accum );
void ClearAccumulatorDigit( PACCUMULATOR accum, uint32_t base );
void SetAccumulatorUpdateProc( PACCUMULATOR accum
									  , void (*Updated)(uintptr_t psv, PACCUMULATOR accum )
									  , uintptr_t psvUser
									  );
void SetAccumulatorMask( PACCUMULATOR accum, uint64_t mask );
