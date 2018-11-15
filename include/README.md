
# SACK Includes



## sack_types.h

### DeclareLink

[DeclareLink() macros](https://github.com/d3x0r/SACK/blob/master/include/sack_types.h#L1348-L1493)

DeclareLink macro...

I really like this

```

struct foo {
	struct foo **me, *next;
}

```

Declare the pointer to the next one, and the pointer to the pointer pointing at me.

Make One...

```
struct foo *makeFoo() {
	struct foo *foo = malloc( sizeof *foo );
        return foo;
}
```

Have a list of them declared somewhere

```
static struct localFooModule {
	struct foo *fooList;
} localFoo;
```

Add one to list...

```
void AddFoo( struct foo *foo ) {
	if( foo->next = localFoo )
        	localFoo->me = &foo->next;
        foo->me = &localFoo;
        localFoo = foo;
}
```


Remove One From anywhere

```
void RemoveFoo( struct foo * foo ) {
	if( foo->me[0] = foo->next )
        	foo->next->me = foo->me;
}
```


