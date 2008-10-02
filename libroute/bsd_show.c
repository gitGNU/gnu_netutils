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

#include <arpa/inet.h>
#include <sys/param.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/mbuf.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/route.h>
#include <netinet/in.h>

#include <sys/sysctl.h>

#include <errno.h>
#include <error.h>
#include <netdb.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "route.h"
#include "xalloc.h"

enum
{
  K_INET = 20,
  K_INET6 = 21
};

struct flag
{
  int mask;
  char * str;
};
typedef struct flag flag_t;

void bsd_conv_addr_to_name (const struct sockaddr *const sock,
                            char *const buffer, const size_t buffer_size,
                            const short int resolve_names);
void bsd_flags (const int field, char *const buffer, size_t size);
const route_info_t * bsd_parse_msg (const sa_family_t sa_family,
                                    const struct rt_msghdr *msg, size_t nread,
                                    const short int resolve_names);
size_t bsd_sysctl (void ** buffer, size_t *const size);
struct rt_msghdr * rt_msg_next (const struct rt_msghdr *rtm,
                                size_t *const length);

void
bsd_conv_addr_to_name (const struct sockaddr *const sock,
                       char *const buffer, const size_t buffer_size,
                       const short int resolve_names)
{
  if (sock->sa_family == AF_LINK)
    conv_link_addr_to_name (sock, sock->sa_len, buffer, buffer_size,
                            resolve_names);
  else if (sock->sa_family == AF_INET || sock->sa_family == AF_INET6)
    {
      void *addr;
      size_t addr_size;

      if (sock->sa_family == AF_INET)
	{
	  addr = &(((struct sockaddr_in *) sock)->sin_addr);
	  addr_size = sizeof (struct in_addr);
	}
      else  /* AF_INET */
	{
	  addr = &(((struct sockaddr_in6 *) sock)->sin6_addr);
	  addr_size = sizeof (struct in6_addr);
	}

      conv_inet_addr_to_name (sock->sa_family, addr, addr_size,
                              buffer, buffer_size, resolve_names);
    }
  else
    printf ("%u\n", sock->sa_family);
}

void
bsd_flags (const int field, char *const buffer, size_t size)
{
  const flag_t flags[] = {
                           {RTF_UP, "U"},
                           {RTF_GATEWAY, "G"},
                           {RTF_HOST, "H"},
                           {RTF_REJECT, "R"},
                           {RTF_DYNAMIC, "D"},
                           {RTF_LLINFO, "L"},
                           {RTF_STATIC, "S"},
                           {RTF_WASCLONED, "W"},
                         };
  int i;

  memset ((void*) buffer, 0, size);
  for (i = 0; i < sizeof (flags) / sizeof (flags[0]); i++)
    {
      if ((field & flags[i].mask) != 0)
        {
          strncat (buffer, flags[i].str, size - 1);
          size -= strlen (flags[i].str);
        }
    }
}

const route_info_t *
bsd_show (const sa_family_t sa_family, const short int resolve_names)
{
  const route_info_t *list;
  size_t nread;
  size_t size;
  struct rt_msghdr *msg;

  nread = bsd_sysctl ((void **) &msg, &size);
  list = bsd_parse_msg (sa_family, msg, nread, resolve_names);
  free (msg);
  return list;
}

const route_info_t *
bsd_parse_msg (const sa_family_t sa_family,
               const struct rt_msghdr *msg, size_t nread,
               const short int resolve_names)
{
  const route_info_t *list = NULL;
  route_info_t *route_info = NULL;
  const struct sockaddr *sock_addr;

  while (msg != NULL)
    {
      sock_addr = (struct sockaddr *) (msg + 1);
      if (sock_addr->sa_family != sa_family)
        goto next;

      route_info = route_info_append (route_info);
      if (list == NULL)
        list = route_info;
      route_info_init (route_info, resolve_names);

      if_indextoname (msg->rtm_index, route_info->oif_name);
      bsd_flags (msg->rtm_flags, route_info->flag_str,
                 sizeof (route_info->flag_str));

      if ((msg->rtm_addrs & RTA_DST) != 0)
        {
#ifdef __KAME__
          /* Remove embedded scope id-field added by KAME implementation. */
          if(sa_family == AF_INET6)
            {
              struct in6_addr *addr_6
                = &(((struct sockaddr_in6 *) sock_addr)->sin6_addr);

              if((IN6_IS_ADDR_LINKLOCAL (addr_6)
                    || IN6_IS_ADDR_MC_LINKLOCAL (addr_6))
                   && ((struct sockaddr_in6 *) sock_addr)->sin6_scope_id == 0)
		{
                  addr_6->s6_addr[2] = 0;
                  addr_6->s6_addr[3] = 0;
                }
            }
#endif  /* __KAME__ */
          route_info->dest_present = 1;
          bsd_conv_addr_to_name (sock_addr, route_info->dest,
                                 sizeof (route_info->dest), resolve_names);
        }

      sock_addr = (struct sockaddr *) ((char *) sock_addr
                                         + SA_SIZE(sock_addr));

      if ((msg->rtm_addrs & RTA_GATEWAY) != 0)
        {
#ifdef __KAME__
          /* Remove embedded scope id-field added by KAME implementation. */
          if(sa_family == AF_INET6)
            {
              struct in6_addr *addr_6
                = &(((struct sockaddr_in6 *) sock_addr)->sin6_addr);

              if((IN6_IS_ADDR_LINKLOCAL (addr_6)
                    || IN6_IS_ADDR_MC_LINKLOCAL(addr_6))
                   && ((struct sockaddr_in6 *)sock_addr)->sin6_scope_id == 0)
                {
                  addr_6->s6_addr[2] = 0;
                  addr_6->s6_addr[3] = 0;
                }
            }
#endif  /* __KAME__ */
          route_info->gateway_present = 1;
          bsd_conv_addr_to_name (sock_addr, route_info->gateway,
                                 sizeof (route_info->gateway), resolve_names);
	}

      sock_addr = (struct sockaddr *) ((char *) sock_addr
                                       + SA_SIZE(sock_addr));
      if ((msg->rtm_addrs & RTA_NETMASK) != 0)
        {
          char buffer[sizeof(struct sockaddr_in6)];

	  memset (buffer, 0, sizeof(struct sockaddr_in6));
          strncpy (buffer, (char *) sock_addr, sock_addr->sa_len);
          ((struct sockaddr *) buffer)->sa_family = sa_family;

	  bsd_conv_addr_to_name ((struct sockaddr *) buffer,
                                 route_info->dest_mask,
                                 sizeof(route_info->dest_mask), 0);
        }

    next:
      msg = rt_msg_next (msg, &nread);
    }

  return list;
}

size_t
bsd_sysctl (void ** buffer, size_t *const size)
{
  int mib[6];
  int status;
  size_t length;

  mib[0] = CTL_NET;
  mib[1] = PF_ROUTE;
  mib[2] = 0;
  mib[3] = 0;
  mib[4] = NET_RT_DUMP;
  mib[5] = 0;

  status = sysctl (mib, 6, NULL, &length, NULL, 0);
  if (status == -1 || length == 0)
    error (EXIT_FAILURE, errno, "sysctl");

  *buffer = xmalloc (length);
  *size = length;

  status = sysctl (mib, 6, *buffer, &length, NULL, 0);
  if (status == -1)
    error (EXIT_FAILURE, errno, "sysctl");

  return length;
}

struct rt_msghdr *
rt_msg_next (const struct rt_msghdr *rtm, size_t *const length)
{
  struct rt_msghdr * next
      = (struct rt_msghdr *) ((char *) rtm + rtm->rtm_msglen);

  *length -= rtm->rtm_msglen;
  if (*length <= 0)
    return NULL;
  else if (*length < next->rtm_msglen)
    return NULL;

  return next;
}
