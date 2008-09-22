#include "route.h"
#include "route_bsd.h"

const route_backend_t bsd_backend =
  {
    bsd_add,
    bsd_add,
    bsd_add,
    bsd_delete,
    bsd_add,
    bsd_add,
    bsd_show
  };
