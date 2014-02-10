#include <generic_cache.h>

#include <stdlib.h>
#include <string.h>

CacheData * createCacheData( POINTER data , unsigned int size )
{
    CacheData *cacheData = malloc( sizeof( CacheData ) + size );
    
    cacheData->data = malloc( size );
    memcpy( cacheData->data , data , size );
    cacheData->size = size;
    
    cacheData->newer = NULL;
    cacheData->older = NULL;
    
    return cacheData;
}

Cache * createCache()
{
    Cache *cache = malloc( sizeof( Cache ) );
    
    cache->itemCount = 0;
    cache->newest = NULL;
    cache->oldest = NULL;
    
    return cache;
}

void deleteCache( Cache *cache )
{
    /* This function hasn't been tested.  It may not (probably won't) work */

    if ( cache->itemCount )
    {
        CacheData *data;
        
        for ( data = cache->newest ; data != NULL ; data = data->older )
        {
            if ( data->newer != NULL )
            {
                free( data->newer );
            }
        }
    }
}

int setCacheStoreFunction( Cache *cache , 
        int (*storeItem)( POINTER , unsigned int ) )
{
    cache->storeItem = storeItem;
    
    return TRUE;
}

int setCacheSearchFunction( Cache *cache ,
        POINTER (*getItemByKey)( POINTER , unsigned int , int ) )
{
    cache->getItemByKey = getItemByKey;
    
    return TRUE;
}

int addItemToCache( Cache *cache , CacheData *data )
{
    if ( cache->newest == NULL )
    {
        cache->newest = data;
        
        if ( cache->oldest == NULL )
        {
            cache->oldest = data;
        }
    }
    else
    {
        data->older = cache->newest;
        data->newer = NULL;
        
        cache->newest->newer = data;
        
        cache->newest = data;
    }
    
    cache->itemCount++;
    
    return TRUE;
}

int storeItemInCache( Cache *cache )
{
    if ( cache->itemCount )
    {
        cache->storeItem( cache->oldest->data , cache->oldest->size );
        
        if ( cache->oldest->newer != NULL )
        {
            cache->oldest = cache->oldest->newer;

            free( cache->oldest->older );
        }

        cache->itemCount--;

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

unsigned int getCacheItemCount( Cache *cache )
{
    return cache->itemCount;
}

POINTER getValueForKey( Cache *cache , int key )
{
    if ( cache->itemCount )
    {
        POINTER item = NULL;
        CacheData *data = cache->newest;
        
        while ( data != NULL )
        {
            item = cache->getItemByKey( data->data , data->size , key );
            
            if ( item != NULL )
            {
                return item;
            }
            
            data = data->older;
        }
        
        return NULL;
    }
    else
    {
        return NULL;
    }
}
