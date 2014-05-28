

#ifndef ACCUMULATOR_STRUCTURE_DEFINED
typedef struct accumulator_tag *PACCUMULATOR;
#endif


#define ACCUM_DOLLARS 0x0001
#define ACCUM_DECIMAL 0x0002
#define ACCUM_TEXT 0x0004

PACCUMULATOR GetAccumulator( CTEXTSTR name, _32 flags );
size_t GetAccumulatorText( PACCUMULATOR accum, TEXTCHAR *text, int nLen );
PACCUMULATOR SetAccumulator( PACCUMULATOR accum, S_64 value );
S_64 GetAccumulatorValue( PACCUMULATOR accum );
PACCUMULATOR AddAcummulator( PACCUMULATOR accum_dest, PACCUMULATOR accum_source );
PACCUMULATOR TransferAccumluator( PACCUMULATOR accum_dest, PACCUMULATOR accum_source );
void KeyTextIntoAccumulator( PACCUMULATOR, CTEXTSTR value ); // works with numeric, if value contains numeric characters
void KeyIntoAccumulator( PACCUMULATOR accum, S_64 val, _32 base );
void KeyDecimalIntoAccumulator( PACCUMULATOR accum );
void ClearAccumulator( PACCUMULATOR accum );
void ClearAccumulatorDigit( PACCUMULATOR accum, _32 base );
void SetAccumulatorUpdateProc( PACCUMULATOR accum
									  , void (*Updated)(PTRSZVAL psv, PACCUMULATOR accum )
									  , PTRSZVAL psvUser
									  );
void SetAccumulatorMask( PACCUMULATOR accum, _64 mask );
