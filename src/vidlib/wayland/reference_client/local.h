
struct pointer_data {
    struct wl_surface *surface;
    struct wl_buffer *buffer;
    int32_t hot_spot_x;
    int32_t hot_spot_y;
    struct wl_surface *target_surface;
};

struct client_state {
    /* Globals */
    struct wl_display *wl_display;
    struct wl_registry *wl_registry;
    struct wl_shm *wl_shm;
    struct wl_compositor *wl_compositor;
    struct xdg_wm_base *xdg_wm_base;
    /* Objects */
    struct wl_surface *wl_surface;
    struct xdg_surface *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;
	 struct wl_seat* wl_seat;
	 struct wl_keyboard* wl_keyboard;
	 struct wl_keyboard_data {
		 int a;
	 }wl_keyboard_data;
	 struct wl_pointer* wl_pointer;
	 struct pointer_data pointer_data;
	struct wl_surface *wl_cursor_surface;
	struct wl_cursor_image *wl_cursor_image;
	struct wl_cursor_theme *wl_cursor_theme;
};
