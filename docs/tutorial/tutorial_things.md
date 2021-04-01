
# Create some things

The development pattern I follow is to define my structures of related values that will be operated on.  Instances of these structures will be called 'thing'.  They might be called 'objects' but that may be confusing for some. 


## Define a type

``` c

struct tutorialType {
    int data;
    char *name;
};
```


## Allocate a thing


``` c
// allocate one thing
struct tutorialType* p = Allocate( sizeof(*p) );
```

## Release a thing

``` C
// allocate one thing
Deallocate( struct tutorialType, p );

```
