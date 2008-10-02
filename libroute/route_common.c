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
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "route.h"
#include "xalloc.h"

/*
 * Supports AF_INET & AF_INET6 addresses.
 */
void
conv_inet_addr_to_name (const int format,
                        const void *const addr, const size_t addr_size,
                        char *const buffer, const size_t buffer_size,
                        const short int resolve_names)
{
  const char *status;
  uint32_t network;
  struct hostent *ht;
  struct netent *nt;

  status = inet_ntop (format, addr, buffer, buffer_size);
  if (status == NULL)
    error (EXIT_FAILURE, errno, "inet_ntop");

  if (resolve_names != 0)
    {
      ht = gethostbyaddr ((char *) addr, addr_size, format);
      if (ht != NULL)
	{
	  strncpy (buffer, ht->h_name, buffer_size - 1);
	  buffer[buffer_size - 1] = '\0';
	}
      else
	{
          /* FIXME: IPv4 specific. */
          network = *(uint32_t *) addr;
          network = ntohl (network);
          nt = getnetbyaddr (network, AF_INET);
          if (nt != NULL)
	    {
	      strncpy (buffer, nt->n_name, buffer_size - 1);
	      buffer[buffer_size - 1] = '\0';
	    }
	}
    }
}

/*
 * Supports *BSDs' AF_LINK addresses.
 */
void
conv_link_addr_to_name (const struct sockaddr *const sock,
                        const socklen_t sock_size,
                        char *const buffer, const size_t buffer_size,
                        const short int resolve_names)
{
  int status;

  status = getnameinfo (sock, sock_size, buffer, buffer_size, NULL, 0,
                        NI_NUMERICHOST);
  if (status != 0)
    error (EXIT_FAILURE, errno, "getnameinfo");
}

void
conv_name_to_addr (const int format, const char *const name,
                   void *const buffer, const size_t buffer_size,
                   const short int resolve_names)
{
  int status;
  size_t addr_size;
  uint32_t network;
  struct hostent *ht;
  struct netent *nt;

  switch (format)
    {
    case AF_INET:
      addr_size = sizeof (struct in_addr);
      break;
    }

  if (buffer_size < addr_size)
    error (EXIT_FAILURE, 0, "insufficient buffer size");

  /* FIXME: IPv4 specific. */
  if (resolve_names != 0)
    {
      /* First get rid of the idiosyncrasies. */
      if (strcmp ("default", name) == 0 || strcmp ("*", name) == 0)
        {
          status = inet_pton (format, "0.0.0.0", buffer);
          if (status < 0)
            error (EXIT_FAILURE, errno, "inet_pton");
          else if (status == 0)
            error (EXIT_FAILURE, 0, "invalid address (%s)", "0.0.0.0");
        }
      /* Next try to resolve host or network name.
         gethostbyname does not seem to mind if it is fed network addresses
         instead of names. */
      else
        {
          /* FIXME: use gethostbyname2 if it is present in GNULib. */
          ht = gethostbyname (name);
          if (ht == NULL)
            {
              /* FIXME: IPv4 specific. */
              nt = getnetbyname (name);
              if (nt == NULL)
                error (EXIT_FAILURE, 0, "invalid name (%s)", name);

              network = (uint32_t) nt->n_net;
              network = htonl (network);
              memmove (buffer, (void *) &network, sizeof (network));
            }
          /* We have already checked the buffer size.  No harm in doing it
             twice.  Better safe than sorry. */
          else if (ht->h_length > buffer_size)
            error (EXIT_FAILURE, 0, "insufficient buffer size");
          else
            memmove (buffer, (void *) ht->h_addr, ht->h_length);
        }
    }
  else
    {
      status = inet_pton (format, name, buffer);
      if (status < 0)
        error (EXIT_FAILURE, errno, "inet_pton");
      else if (status == 0)
        error (EXIT_FAILURE, 0, "invalid address (%s)", "0.0.0.0");
    }
}

void
convert_netmask (const int format, const unsigned char length,
		 char *const buffer, const size_t buffer_size)
{
  uint32_t afinet_mask = 0;

  switch (format)
    {
    case AF_INET:
      afinet_mask =
	((1 << length) - 1) << (sizeof (afinet_mask) * 8 - length);
      afinet_mask = htonl (afinet_mask);
      inet_ntop (format, (void *) &afinet_mask, buffer, buffer_size);
      break;

    default:
      buffer[0] = '\0';
      return;
    }
}

void
route_info_init (route_info_t * const route_info,
		 const short int resolve_names)
{
  if (route_info == NULL)
    return;

  memset (route_info, 0, sizeof (*route_info));

  if (resolve_names == 0)
    {
      strcpy (route_info->dest, "0.0.0.0");
      strcpy (route_info->src, "0.0.0.0");
      strcpy (route_info->gateway, "0.0.0.0");
      strcpy (route_info->pref_src, "0.0.0.0");
    }
  else
    {
      strcpy (route_info->dest, "default");
      strcpy (route_info->src, "*");
      strcpy (route_info->gateway, "*");
      strcpy (route_info->pref_src, "*");
    }
}

route_info_t *
route_info_append (route_info_t *const tail)
{
  route_info_t *const node = (route_info_t *) xmalloc (sizeof (route_info_t));

  if (tail != NULL)
    tail->next = node;
  return node;
}
