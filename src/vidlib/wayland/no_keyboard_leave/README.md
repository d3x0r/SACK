
This test program fills the surface with green when it has the keyboard focus, and red
when it loses the keyboard focus.  When you move the mouse into the green surface, a
pointer_enter event is generated, that does a set_cursor.  After the set cursor, then
clicking on another window, a keyboard_leave event is not generated, and the surface
stays green.

# To make

```
mkdir build
cd build
cmake ..
make
```

# To run

```
# in the build directory
./client

# or with debuggging

WAYLAND_DEBUG=1 ./client
```

