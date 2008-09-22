#include "route.h"

#ifdef __FreeBSD__
#include "route_bsd.h"
#endif  /* __FreeBSD__ */

#ifdef __linux__
#include "route_linux.h"
#endif /* __linux__ */

const route_backend_t *route_backend = NULL;

void
route_backend_init (void)
{
#ifdef __FreeBSD__
  route_backend = &bsd_backend;
#endif /* __FreeBSD__ */

#ifdef __linux__
  route_backend = &linux_backend;
#endif /* __linux__ */
}
