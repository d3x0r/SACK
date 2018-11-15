
# SACK Includes



## sack_types.h

### DeclareLink

[DeclareLink() macros](https://github.com/d3x0r/SACK/blob/master/include/sack_types.h#L1348-L1493)

DeclareLink macro...

I really like this

``` C
struct foo {
	struct foo **me, *next;
}
// same as the above, but using macros from sack_types.h
struct fooSame {
	DeclareLink( struct fooSame );
}

```

Declare the pointer to the next one, and the pointer to the pointer pointing at me.

Make One...

``` C
struct foo *makeFoo() {
	struct foo *foo = malloc( sizeof *foo );
        return foo;
}

// same as the above, but using macros from sack_types.h
struct fooSame *makeFooSame() {
	struct fooSame *foo = malloc( sizeof *foo );
        return foo;
}
```

Have a list of them declared somewhere

``` C
static struct localFooModule {
	struct foo *fooList;
	struct fooSame *fooSameList;
} localFoo;
```

Add one to list...

``` C
void AddFoo( struct foo *foo ) {
	if( foo->next = localFoo.fooList )
        	localFoo.fooList->me = &foo->next;
        foo->me = &localFoo.fooList;
        localFoo.fooList = foo;
}

// same as the above, but using macros from sack_types.h  This wouldn't NEED 
// a function, but would just be done inline.
void AddFooSame( struct fooSame *foo ) {
	LinkThing( localFoo.fooSame, foo );
}
```


Remove One From anywhere... notice, doesn't require knowing the root of the list...

``` C
void RemoveFoo( struct foo * foo ) {
	if( foo->me[0] = foo->next )
        	foo->next->me = foo->me;
}

// same as the above, but using macros from sack_types.h  This wouldn't NEED 
// a function, but would just be done inline.
void RemoveFooSame( struct fooSame * foo ) {
	UnlinkThing( foo );
}
```


