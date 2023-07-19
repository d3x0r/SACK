#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>
#include <wayland-server.h>
#include <wayland-client-protocol.h>
#include <wayland-egl.h>

struct wl_display *display = NULL;
struct wl_compositor *compositor = NULL;
struct wl_surface *surface;
struct wl_shell *shell = NULL;
struct wl_shell_surface *shell_surface;

static void
global_registry_handler(void *data, struct wl_registry *registry, uint32_t id,
	       const char *interface, uint32_t version)
{
    if (strcmp(interface, "wl_compositor") == 0) {
        compositor = wl_registry_bind(registry, 
				      id, 
				      &wl_compositor_interface, 
				      1);
    } else if (strcmp(interface, "wl_shell") == 0) {
        shell = wl_registry_bind(registry, id,
                                 &wl_shell_interface, 1);
    }
}

static void
global_registry_remover(void *data, struct wl_registry *registry, uint32_t id)
{
    printf("Got a registry losing event for %d\n", id);
}

static const struct wl_registry_listener registry_listener = {
    global_registry_handler,
    global_registry_remover
};


int main(int argc, char **argv) {

    display = wl_display_connect(NULL);
    if (display == NULL) {
	fprintf(stderr, "Can't connect to display\n");
	exit(1);
    }
    printf("connected to display\n");

    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);

    wl_display_dispatch(display);
    wl_display_roundtrip(display);

    if (compositor == NULL) {
	fprintf(stderr, "Can't find compositor\n");
	exit(1);
    } else {
	fprintf(stderr, "Found compositor\n");
    }

    surface = wl_compositor_create_surface(compositor);
    if (surface == NULL) {
	fprintf(stderr, "Can't create surface\n");
	exit(1);
    } else {
	fprintf(stderr, "Created surface\n");
    }

    if (shell == NULL) {
	fprintf(stderr, "Haven't got a Wayland shell\n");
	exit(1);
    }
    shell_surface = wl_shell_get_shell_surface(shell, surface);
    if (shell_surface == NULL) {
	fprintf(stderr, "Can't create shell surface\n");
	exit(1);
    } else {
	fprintf(stderr, "Created shell surface\n");
    }
    wl_shell_surface_set_toplevel(shell_surface);

    wl_display_disconnect(display);
    printf("disconnected from display\n");
    
    exit(0);
}