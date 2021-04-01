


```
// set how much you would like to get
uintptr_t size = 500000; 
PMEM heap = DigSpace( "what", "where", &size );

// size will be updated to the actual size resulted.

```

Allocate from the custom heap.

```

char *tenchars = HeapAllocate( heap, 10 );

```