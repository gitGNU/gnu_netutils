/* Copyright (C) 2008 Free Software Foundation, Inc.

   This file is part of GNU Inetutils.

   GNU Inetutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Inetutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Inetutils; see the file COPYING.  If not, write
   to the Free Software Foundation, Inc., 51 Franklin Street,
   Fifth Floor, Boston, MA 02110-1301 USA. */

#ifndef _ROUTE_LINUX_H
#define _ROUTE_LINUX_H

#include <stddef.h>

#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include "route.h"

struct _rtmroute_t
{
  struct nlmsghdr nlmsghdr;
  struct rtmsg rtmsg;
  /* According to iproute2, 1024 bytes should be enough.  Although BUFSIZ is
     guaranteed to be only 256, in GNU libc it is 8192. */
  char buffer[BUFSIZ];
};
typedef struct _rtmroute_t rtmroute_t;

extern void linux_add (const int format,
                       const void *const dest_addr,
                       const size_t dest_addr_size,
                       const unsigned char dest_len,
                       const void *const gw_addr,
                       const size_t gw_addr_size,
                       const unsigned int iface);

extern void linux_append (const int format,
                          const void *const dest_addr,
                          const size_t dest_addr_size,
                          const unsigned char dest_len,
                          const void *const gw_addr,
                          const size_t gw_addr_size,
                          const unsigned int iface);

extern void linux_change (const int format,
                          const void *const dest_addr,
                          const size_t dest_addr_size,
                          const unsigned char dest_len,
                          const void *const gw_addr,
                          const size_t gw_addr_size,
                          const unsigned int iface);

extern void linux_delete (const int format,
                          const void *const dest_addr,
                          const size_t dest_addr_size,
                          const unsigned char dest_len);

extern void linux_prepend (const int format,
                           const void *const dest_addr,
                           const size_t dest_addr_size,
                           const unsigned char dest_len,
                           const void *const gw_addr,
                           const size_t gw_addr_size,
                           const unsigned int iface);

extern void linux_replace (const int format,
                           const void *const dest_addr,
                           const size_t dest_addr_size,
                           const unsigned char dest_len,
                           const void *const gw_addr,
                           const size_t gw_addr_size,
                           const unsigned int iface);

extern const route_info_t * linux_show (const sa_family_t sa_family,
                                        const short int resolve_names);

extern const route_backend_t linux_backend;

#endif /* _ROUTE_LINUX_H */
