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

#include <net/if.h>
#include <netlink/addr.h>
#include <netlink/object.h>
#include <netlink/route/rtnl.h>
#include <netlink/route/route.h>

#include "route.h"
#include "route_linux.h"

struct user_data
{
  route_info_t *head;
  route_info_t *tail;
  const short int resolve_names;
};
typedef struct user_data user_data_t;

char *
linux_conv_addr_to_name (struct nl_addr *const addr,
                         char *const buffer, const size_t buffer_size,
                         const short int resolve_names)
{
  int status;
  struct sockaddr sa;
  socklen_t salen = sizeof (sa);

  if (addr == NULL)
    return NULL;

  status = nl_addr_fill_sockaddr (addr, &sa, &salen);
  if (status != 0)
    return NULL;

  if (sa.sa_family != AF_INET && sa.sa_family != AF_INET6)
    return NULL;

  if (sa.sa_family == AF_INET)
    {
      const struct in_addr addr = ((struct sockaddr_in *) &sa)->sin_addr;
      conv_inet_addr_to_name (sa.sa_family, (void *) &addr, sizeof (addr),
                              buffer, buffer_size, resolve_names);
    }
  else if (sa.sa_family == AF_INET6)
    {
      const struct in6_addr addr = ((struct sockaddr_in6 *) &sa)->sin6_addr;
      conv_inet_addr_to_name (sa.sa_family, (void *) &addr, sizeof (addr),
                              buffer, buffer_size, resolve_names);
    }

  return buffer;
}

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
linux_parse_route (struct nl_object *object, void *data)
{
  const char *status;
  int format;
  const short int resolve_names = ((user_data_t *) data)->resolve_names;
  route_info_t **head = &((user_data_t *) data)->head;
  route_info_t **tail = &((user_data_t *) data)->tail;
  struct nl_addr *addr;
  struct rtnl_route *const route = (struct rtnl_route *) object;

  format = rtnl_route_get_family (route);
  if (format != AF_INET || rtnl_route_get_table (route) != RT_TABLE_MAIN)
    return;

  *tail = route_info_append (*tail);
  if (*head == NULL)
    *head = *tail;

  route_info_init (*tail, resolve_names);

  addr = rtnl_route_get_dst (route);
  if (addr != NULL)
    {
      status = linux_conv_addr_to_name (addr, (*tail)->dest,
                                        sizeof ((*tail)->dest), resolve_names);
      if (status != NULL)
        (*tail)->dest_present = 1;
    }

  (*tail)->dest_len = rtnl_route_get_dst_len (route);
  convert_netmask (format, (*tail)->dest_len, (*tail)->dest_mask,
                   sizeof ((*tail)->dest_mask));

  addr = rtnl_route_get_src (route);
  if (addr != NULL)
    {
      status = linux_conv_addr_to_name (addr, (*tail)->src,
                                        sizeof ((*tail)->src), resolve_names);
      if (status != NULL)
        (*tail)->src_present = 1;
    }

  (*tail)->flag_bits = rtnl_route_get_flags (route);
  rtnl_route_nh_flags2str ((*tail)->flag_bits, (*tail)->flag_str,
                           sizeof ((*tail)->flag_str));

  (*tail)->oif_index = rtnl_route_get_oif (route);
  if_indextoname ((*tail)->oif_index, (*tail)->oif_name);

  addr = rtnl_route_get_gateway (route);
  if (addr != NULL)
    {
      status = linux_conv_addr_to_name (addr, (*tail)->gateway,
                                        sizeof ((*tail)->gateway),
                                        resolve_names);
      if (status != NULL)
        (*tail)->gateway_present = 1;
    }

  (*tail)->metric = rtnl_route_get_prio (route);

  addr = rtnl_route_get_pref_src (route);
  if (addr != NULL)
    {
      status = linux_conv_addr_to_name (addr, (*tail)->pref_src,
                                        sizeof ((*tail)->pref_src),
                                        resolve_names);
      if (status != NULL)
        (*tail)->pref_src_present = 1;
    }
}

const route_info_t *
linux_show (const short int resolve_names)
{
  struct nl_cache *cache;
  struct nl_handle *handle;
  user_data_t data = {NULL, NULL, resolve_names};

  handle = linux_create_handle ();
  if (handle == NULL)
    return NULL;

  cache = rtnl_route_alloc_cache (handle);
  nl_cache_mngt_provide (cache);
  nl_cache_foreach (cache, linux_parse_route, (void *) &data);
  nl_cache_free (cache);

  nl_handle_destroy (handle);
  return data.head;
}
