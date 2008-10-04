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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
  const sa_family_t sa_family;
  const short int resolve_names;
};
typedef struct user_data user_data_t;

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

char * linux_conv_addr_to_name (struct nl_addr *const addr,
                                char *const buffer, const size_t buffer_size,
                                const short int resolve_names);
static struct nl_handle * linux_create_handle (void);
void linux_modify (const int format,
                   const int flags,
                   const void *const dest_addr,
                   const size_t dest_addr_size,
                   const unsigned char dest_len,
                   const void *const gw_addr,
                   const size_t gw_addr_size,
                   const unsigned int oif_index);
void linux_parse_route (struct nl_object *object, void *data);

void
linux_add (const int format,
	   const void *const dest_addr,
           const size_t dest_addr_size,
	   const unsigned char dest_len,
	   const void *const gw_addr,
           const size_t gw_addr_size,
	   const unsigned int iface)
{
  linux_modify (format, NLM_F_CREATE | NLM_F_EXCL,
                dest_addr, dest_addr_size, dest_len,
                gw_addr, gw_addr_size, iface);
}

void
linux_append (const int format,
	      const void *const dest_addr,
              const size_t dest_addr_size,
	      const unsigned char dest_len,
	      const void *const gw_addr,
              const size_t gw_addr_size,
	      const unsigned int iface)
{
  linux_modify (format, NLM_F_CREATE | NLM_F_APPEND,
                dest_addr, dest_addr_size, dest_len,
                gw_addr, gw_addr_size, iface);
}

void
linux_change (const int format,
	      const void *const dest_addr,
              const size_t dest_addr_size,
	      const unsigned char dest_len,
	      const void *const gw_addr,
              const size_t gw_addr_size,
	      const unsigned int iface)
{
  linux_modify (format, NLM_F_REPLACE,
                dest_addr, dest_addr_size, dest_len,
                gw_addr, gw_addr_size, iface);
}

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

void
linux_prepend (const int format,
               const void *const dest_addr,
               const size_t dest_addr_size,
               const unsigned char dest_len,
               const void *const gw_addr,
               const size_t gw_addr_size,
               const unsigned int iface)
{
  linux_modify (format, NLM_F_CREATE,
                dest_addr, dest_addr_size, dest_len,
                gw_addr, gw_addr_size, iface);
}

void
linux_replace (const int format,
               const void *const dest_addr,
               const size_t dest_addr_size,
               const unsigned char dest_len,
               const void *const gw_addr,
               const size_t gw_addr_size,
               const unsigned int iface)
{
  linux_modify (format, NLM_F_CREATE | NLM_F_REPLACE,
                dest_addr, dest_addr_size, dest_len,
                gw_addr, gw_addr_size, iface);
}

void
linux_modify (const int format,
              const int flags,
	      const void *const dest_addr,
              const size_t dest_addr_size,
	      const unsigned char dest_len,
	      const void *const gw_addr,
              const size_t gw_addr_size,
	      const unsigned int oif_index)
{
  int status;
  struct nl_addr *dest;
  struct nl_addr *gw;
  struct nl_handle *handle;
  struct rtnl_route *route;

  handle = linux_create_handle ();
  if (handle == NULL)
    return;

  route = rtnl_route_alloc ();
  if (route == NULL)
    return;

  rtnl_route_set_oif (route, oif_index);

  dest = nl_addr_build (format, dest_addr, dest_addr_size);
  if (dest == NULL)
    return;
  nl_addr_set_prefixlen (dest, dest_len);
  rtnl_route_set_dst (route, dest);
  nl_addr_put (dest);

  gw = nl_addr_build (format, gw_addr, gw_addr_size);
  if (gw == NULL)
    return;
  rtnl_route_set_gateway (route, gw);
  nl_addr_put (gw);

  rtnl_route_set_scope (route, RT_SCOPE_UNIVERSE);
  rtnl_route_set_table (route, RT_TABLE_MAIN);

  status = rtnl_route_add (handle, route, NLM_F_REQUEST | flags);
  if (status != 0)
    return;

  rtnl_route_put (route);
  nl_handle_destroy (handle);
}

void
linux_parse_route (struct nl_object *object, void *data)
{
  const char *status;
  const sa_family_t sa_family = ((user_data_t *) data)->sa_family;
  const short int resolve_names = ((user_data_t *) data)->resolve_names;
  route_info_t **head = &((user_data_t *) data)->head;
  route_info_t **tail = &((user_data_t *) data)->tail;
  struct nl_addr *addr;
  struct rtnl_route *const route = (struct rtnl_route *) object;

  if (rtnl_route_get_family (route) != sa_family
        || rtnl_route_get_table (route) != RT_TABLE_MAIN)
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
  convert_netmask (sa_family, (*tail)->dest_len, (*tail)->dest_mask,
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
linux_show (const sa_family_t sa_family, const short int resolve_names)
{
  struct nl_cache *cache;
  struct nl_handle *handle;
  user_data_t data = {NULL, NULL, sa_family, resolve_names};

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
