
# Containers 

The definition for types themselves is split into `sack_types.h` and `sack_typelib.h` both of which are included by `stdhdrs.h`.

``` c
#include <stdhdrs.h>
```

## Conventions

Many of the type'd containers take a address of the pointer to the container.  This allows the library to udpate this pointer.  This leads to the practice of having the equivalent of `unique_ptr` in C++, where there's really only ever one pointer to the container, and everything else has a reference to that pointer.

In this case, it is really more proper to say 'pass a reference to the container' rather than 'pass the address of the pointer to the container'; although they mean the same thing.


# Containers that store references

## Lists

The first, most basic sort of container is an unordered list of things.  This functions more like a C++ 'Vector'; it is a slab allocated to contain pointers to a user's [objects](tutorial_things.md).



### Create a List.

``` c
PLIST myList = NULL;
```

#### Advanced

A list also has a creation function which can control the initial list capacity and the expansion factor.

``` c
PLIST myList = CreateList( 10000, 20000 );
```


### Add to a List.

Assuming you created an object and have a referce of that object `p`....

``` c
    AddLink( &myList, p );
```


### Iterate the list


``` c
{
    struct tutorialType* p;
    INDEX idx;
    LIST_FORALL( myList, idx, struct tutorialType*, p ) {
        // do something with each memeber 'p' in the list.    
    }
}
```

In the above framgment, at the end, `p` will be NULL if the list was empty.
    `p` will also be NULL if the list iterated to the end.  If a `break` is issued within the loop, then `p` will be set to whatever the current value was.

Checking the value after the list can indicate if a item was found in the list.


### Delete from the list

``` c
    DeleteLink( &myList, p );
```

## Queues

Queues are a first in first out sort of container.  They have an enque and deque method.



### Create a Queue

``` c
PLINKQUEUE myQueue = NULL;
```

### Add to a queue

Assuming you created an object and have a referce of that object `p`....

``` c
    EnqueLink( &myQueue, p );
```

### Get an item from the queue


``` c
{
    struct tutorialType* p;
    p = DequeLink( &myQueue );

    // this will have to be typecast if used in CPP
    //p = (struct tutorialType*)DequeLink( &myQueue );
}
```


### Inspect items in the queue

You can look at the first item in the queue, without actually removing it.

``` c
{
    struct tutorialType* p;
    p = PeekQueue( &myQueue );
    // returns the next item which would be dequeued
}
```

You can also look down the queue, at all of the items already in the queue.  This can be used to prevent inserting the same item which is already outstanding.

``` c
{
    struct tutorialType* p;
    int index = 0;
    while( p = PeekQueueEx ( &myQueue,  index++ ) ){
        // gets each item (if there is one) from the queue, in the 
        // order they would be dequeued; or from the first added to the last added.
    }
}
```

You might also iterate from the newest item to the oldest...

``` c
{
    struct tutorialType* p;
    int index = -1;
    while( p = PeekQueueEx ( &myQueue,  index-- ) ){
        // gets each item (if there is one) from the end queue, in the 
        // opposite order they would be dequeued; or from the newest to the
        // oldest.
    }
}
```


## Stacks

Stacks are a last in first out sort of container.  They have a push and a pop method.



### Create a Stack

``` c
PLINKSTACK myStack = NULL;
```

### Add to a stack

Assuming you created an object and have a referce of that object `p`....

``` c
    PushLink &myStack, p );
```

### Get an item from the stack


``` c
{
    struct tutorialType* p;
    p = PopLink( &myStack );
}
```

### Inspect items on the stack

This will return the next item to be popped, without actually popping the item.

``` c
{
    struct tutorialType* p;
    p = PeekLink( &myStack );
}
```

You might also inspect values deeper on the stack.

``` c
{
    struct tutorialType* p;
    int index = 0;

    while( p = PeekLinkEx( &myStack, index++ ) ){
        // for each item in the stack, look at, but do not remove...
    }
}
```

# Data Containers

## Lists

The DataList does not know whether a entry is filled or not, and used entries must be tracked in the structure used, or by some other method.  see also [GenericSet](#GenericSet) (below).

### Create a List.

When creating a list that stores data items, the size of the item must be specified with the create.

``` c
PDATALIST myDataList = CreateDataList( sizeof( struct tutorialStruct ) );
```


### Add to a List.

Assuming you created an object and have a referce of that object `p`....

``` c
    AddDataItem( &myDataList, p );
```

### Set an item specifically in the list

Assuming you created an object and have a referce of that object `p`....

``` c
    int index = 5;
    SetDataItem( &myDataList, index, p );
```

### Get an item specifically in the list

``` c
    struct tutorialType* p;
    int index = 5;
    p = GetDataItem( &myDataList, index );
```


### Iterate the list


``` c
{
    struct tutorialType* p;
    INDEX idx;
    DATA_FORALL( myDataList, idx, struct tutorialType*, p ) {
        // do something with each memeber 'p' in the list.    
    }
}
```



## Queues

Data queues are like queues, but stores the object data as a value.



### Create a Queue

``` c
PDATAQUEUE myDataQueue = NULL;
```

### Add to a queue

Assuming you created an object and have a referce of that object `p`, this copies
the value from the pointer passed to the queue.

``` c
    EnqueData( &myDataQueue, p );
```

### Get an item from the queue


``` c
{
    struct tutorialType value;
    
    while( DequeData( &myDataQueue, &value ) ) {
        // value will be copied from the values stored in the queue
        
    }

}
```


### Inspect items in the queue

You can look at the first item in the queue, without actually removing it.

``` c
{
    struct tutorialType value;
    PeekDataQueue( &myQueue, index, &value );
    // returns a copy of the next value to be dequeued.
}
```

You can also look down the queue, at all of the items already in the queue.  This can be used to prevent inserting the same item which is already outstanding.

``` c
{
    struct tutorialType value;
    int index = 0;
    while( PeekDataQueueEx ( &myQueue,  index++, &value ) ){
        // gets each item (if there is one) from the queue, in the 
        // order they would be dequeued; or from the first added to the last added.
    }
}
```

You might also iterate from the newest item to the oldest...

``` c
{
    struct tutorialType value;
    int index = -1;
    while( PeekDataQueueEx ( &myQueue,  index++, &value ) ){
        // gets each item (if there is one) from the end queue, in the 
        // opposite order they would be dequeued; or from the newest to the
        // oldest.
    }
}
```


## Stacks

Data stacks are a last in first out sort of container.  They have a push and a pop method.
They store a copy of the value rather than the pointer to the value.



### Create a Stack

``` c
PDATASTACK myDataStack = CreateDataStack( sizeof( struct tutorialType ) );
```

### Add to a stack

Assuming you created an object and have a referce of that object `p`....

``` c
    PushData &myStack, p );
```

### Get an item from the stack

This returns an allocated item, which is a copy of the data on the stack.

``` c
{
    struct tutorialType* p;
    p = PopData( &myStack );
    Release( p );
}
```

### Inspect items on the stack

This will return the next item to be popped, without actually popping the item.

``` c
{
    struct tutorialType* p;
    p = PeekData( &myDataStack );
}
```

You might also inspect values deeper on the stack.

``` c
{
    struct tutorialType* p;
    int index = 0;

    while( p = PeekDataEx( &myStack, index++ ) ){
        // for each item in the stack, look at, but do not remove...
    }
}
```

# Other ...

## Linked List

Linked list utilities are really just macros.  They rely on standard `*next` and `**me` members in a structure.

### Create a structure

Linked list member elements can use the `DeclareLink()` macro to declare the additional members.

``` c
struct tutorialListMember {
    DeclareLink( struct tutorialListMember );

    int some_data;
}


```

### Create a list

``` c

struct tutorialListMember *root = NULL;


```

### Link a new member to the list

Create a new member to be linked to the list, and link it into the list.

``` c

struct tutorialListMember *p = New( struct tutorialListMember );

LinkThing( &root, p );


```

### Delete a member of the list

For some pointer to a member in the list, can easily remove the item.

``` c
UnlinkThing( p );
```





## Generic Sets

Generic sets are a hybrid container; they are a data list, which also has a bit array tracking usage in items.  Generic sets are kept in a list of slabs, so there's no reallocation.

This requires the structure have a typedef, because there are several token pastes used to build the required values.

Generic sets are responsible for allocating the objects also, so you would use this to allocate them instead of `Allocate()`.  

### Create a type

``` c
typedef struct tutorialType *SETTYPE;

```

### `#define` a size for the blocks

``` c
//#define MAX##name##SPERSET
#define MAXSETTYPESPERSET 256
```



### Create a set

``` c
DeclareSet( SETTYPE ) tutorialSet;
```

### Get a member

This results in a 'p' which is zeroed.

``` c
struct tutorialType* p = GetFromSet( SETTYPE, &tutorialSet );
```

### Return a member to the set

This releases the member of the set, allowing it to be retrieved later for a new item.

``` c
DeleteFromSet( SETTYPE, &tutorialSet, p );
```

### More on Generic Sets

One of the purposes of this type was to track dynamic arrays of values such as vertex and color data used in graphics programming.  There are functions to get a flat array from the generic set.  There also other iteration methodd for the set.  

# Text

The string functions in the C standard library are often sufficient, but inconvenient for representing absolutely any and every content.

## `PTEXT` 

This is a single allocation that contains the text of a string plus additional meta data.  Attribute information like color and position can be stored on the text segment.  The text segments rely on a length, instead of scanning for a length.

## `PVARTEXT`

Variable Text can be used to build strings.  It works like a buffer you can `vtprintf()` into; which uses the same format specifications as `printf()`.

### Create a string buffer

``` c
PVARTEXT pvtString = VarTextCreate();
```

### Add content

``` c
vtprintf( pvtString, "Hello, %s", "World" );
```

### Get content built into buffer

To get the content currently collected, you can either get the result, which returns a 
copy of the buffer, clearing any content collected in the `PVARTEXT`.

``` c
PTEXT result = VarTextGet( pvtString );

/* do something with result */

LineRelease( result );  // Make sure you release the result when done.

```

You can also just peek at the current buffer, which leaves the content in-place.


``` c
PTEXT result = VarTextPeek( pvtString );

/* do something with result */

```

### Destroy the PVARTEXT when done

when done with the PVARTEXT builder, destroy it... The `VarTextDestory()` function takes the address of the string builder so it can set it to NULL, helping to make sure it's not used later.

``` c
VarTextDestroy( &pvtString );
```


## Utf8

There are a few utility functions for UTF8 to widechar conversions.  Internally all text is really utf8, and converted to wide character on the edges where functions interface with the physical system.
