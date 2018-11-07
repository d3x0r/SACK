#ifndef INCLUDED__CACHE_H__
#define INCLUDED__CACHE_H__

#include <sack_types.h>

#if 0 /* This header file is going in SACK only now, so forget this */
    /* This should help this library*/
    #ifdef USE_SACK
        #include <sack_types.h>
    #else
        #define POINTER (void *)
    #endif /* USE_SACK */
#endif

#ifndef TRUE
    #define TRUE ( 0 == 0 )
    #define FALSE !( TRUE )
#endif /* TRUE */

#ifndef SUCCESS
    #define SUCCESS TRUE
    #define FAILURE FALSE
#endif /* SUCCESS */

enum
{
    CACHE_PRIORITY_NONE ,
    
    CACHE_PRIORITY_OLDEST_FIRST ,
    CACHE_PRIORITY_NEWEST_FIRST ,
    
    CACHE_PRIORITY_COUNT
};

typedef struct structCacheData
{
    POINTER data;
    
    unsigned int size;
    
    struct structCacheData *newer;
    struct structCacheData *older;
} CacheData;

typedef struct structCache
{
    CacheData *newest;
    CacheData *oldest;
    
    int (*storeItem)( POINTER , unsigned int );
    void * (*getItemByKey)( POINTER , unsigned int , int );
    
    unsigned int itemCount;
} Cache;

CacheData * createCacheData( POINTER data , unsigned int size );

Cache * createCache();
void deleteCache( Cache *cache );

int setCacheStoreFunction( Cache *cache , 
        int (*storeItem)( POINTER , unsigned int ) );
int setCacheSearchFunction( Cache *cache ,
        POINTER (*getItemByKey)( POINTER , unsigned int , int ) );

int addItemToCache( Cache *cache , CacheData *data );
int storeItemInCache( Cache *cache );
unsigned int getCacheItemCount( Cache *cache );

void * getValueForKey( Cache *cache , int key );

#endif /* __CACHE_H__ */
