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

#include <errno.h>
#include <error.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netlink/addr.h>
#include <netlink/route/rtnl.h>
#include <netlink/route/route.h>

#include "route.h"
#include "route_linux.h"

static struct nl_handle *
linux_create_handle (void)
{
  int status;
  struct nl_cb *cb;
  struct nl_handle *handle;

  cb = nl_cb_alloc (NL_CB_VERBOSE);
  handle = nl_handle_alloc_cb (cb);
  if (handle == NULL)
    return NULL;

  status = nl_connect (handle, NETLINK_ROUTE);
  if (status < 0)
    {
      nl_handle_destroy (handle);
      return NULL;
    }

  return handle;
}

void
linux_delete (const int format,
              const void *const dest_addr,
              const size_t dest_addr_size,
              const unsigned char dest_len)
{
  int status;
  struct nl_addr *dest;
  struct nl_handle *handle;
  struct rtnl_route *route;

  handle = linux_create_handle ();
  if (handle == NULL)
    return;

  route = rtnl_route_alloc ();
  if (route == NULL)
    return;

  dest = nl_addr_build (format, dest_addr, dest_addr_size);
  if (dest == NULL)
    return;
  nl_addr_set_prefixlen (dest, dest_len);
  rtnl_route_set_dst (route, dest);
  nl_addr_put (dest);

  rtnl_route_set_scope (route, RT_SCOPE_UNIVERSE);
  rtnl_route_set_table (route, RT_TABLE_MAIN);

  status = rtnl_route_del (handle, route, NLM_F_REQUEST);
  if (status != 0)
    return;

  rtnl_route_put (route);
  nl_handle_destroy (handle);
}
