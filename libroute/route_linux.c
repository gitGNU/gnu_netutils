#include "route.h"
#include "route_linux.h"

const route_backend_t linux_backend =
  {
    linux_add,
    linux_append,
    linux_change,
    linux_delete,
    linux_prepend,
    linux_replace,
    linux_show
  };
