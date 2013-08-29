// don't define exit() apache has a definition for it.
#define NO_SACK_EXIT_OVERRIDE

#include <stdhdrs.h>

#include <httpd.h>  // apache header files.



#if 0
module AP_MODULE_DECLARE_DATA foo_module =
{
    STANDARD20_MODULE_STUFF,
    NULL, //create_dir_conf, /* Per-directory configuration handler */
    NULL, //merge_dir_conf,  /* Merge handler for per-directory configurations */
    NULL, //create_svr_conf, /* Per-server configuration handler */
    NULL, //merge_svr_conf,  /* Merge handler for per-server configurations */
    NULL, //directives,      /* Any directives we may have for httpd */
    register_hooks   /* Our hook registering function */
};
#endif

