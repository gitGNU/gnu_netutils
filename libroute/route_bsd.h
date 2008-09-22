#ifndef _ROUTE_BSD_H
#define _ROUTE_BSD_H

#include <stddef.h>

#include "route.h"

extern void bsd_add (const int format,
                     const uint16_t flags,
                     const void *const dest_addr,
                     const size_t dest_addr_size,
                     const unsigned char dest_addr_len,
                     const void *const gw_addr,
                     const size_t gw_addr_size,
                     const unsigned int iface);

extern void bsd_delete (const int format,
                        const void *const dest_addr,
                        const size_t dest_addr_size,
                        const unsigned char dest_len);

extern const route_info_t * bsd_show (const short int resolve_names);

extern const route_backend_t bsd_backend;

#endif /* _ROUTE_BSD_H */
