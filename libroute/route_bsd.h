/* Copyright (C) 2008 Free Software Foundation, Inc.

   This file is part of Netutils.

   Netutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   Netutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Netutils; see the file COPYING.  If not, write
   to the Free Software Foundation, Inc., 51 Franklin Street,
   Fifth Floor, Boston, MA 02110-1301 USA. */

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

extern const route_info_t * bsd_show (const sa_family_t sa_family,
                                      const short int resolve_names);

extern const route_backend_t bsd_backend;

#endif /* _ROUTE_BSD_H */
