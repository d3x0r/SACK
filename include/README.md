
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

---

### Declare Link Other Nots

In the above example...
``` C
struct foo {
	struct foo **me, *next;
};
```

This is the reverse of what Declarelink actually declares.  The order can be
(ab)used to use *me also as a prior pointer.  Except that fails at the root
of the list, and requires that the root also be declared as a pointer pair 
so the `me` of root can be NULL.  (nothing points at this, it just IS)

``` C
struct foo {
	struct foo *next;
        struct foo **me;
};

static struct localFooModule {
	struct foo *fooList;
	struct foo *fooListMe; // must be NULL
} localFoo;


```


Then in the above case, since `node->me == &otherNode->next`, and `&otherNode->next == &otherNode`,
then `node->me` could just be recast as `((struct foo*)node->me)` and it would be the same as `prior`
in a doubly linked list with `next` and `prior`.

## vectlib.h

Vector/Matrix floatoint point library.  Workes on a type 'RCOORD' which should be 
configured as either `float` or `double`.  The librar attempts to compile dual-moded
with suffixes on the routines; so the user can choose one or the other...



 