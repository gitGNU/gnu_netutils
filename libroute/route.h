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

#ifndef _ROUTE_H
#define _ROUTE_H

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <netdb.h>
#include <stddef.h>
#include <stdio.h>

#include <sys/socket.h>
#include <net/if.h>

struct route_info
{
  uint8_t dest_present;
  char dest[NI_MAXHOST];
  char dest_mask[NI_MAXHOST];
  uint8_t dest_len;
  uint8_t src_present;
  char src[NI_MAXHOST];
  char flag_str[6];
  int flag_bits;
  char oif_name[IFNAMSIZ];
  int oif_index;
  uint8_t gateway_present;
  char gateway[NI_MAXHOST];
  uint32_t metric;
  uint8_t pref_src_present;
  char pref_src[NI_MAXHOST];
  uint32_t ref;
  uint32_t use;
  struct route_info *next;
};
typedef struct route_info route_info_t;

/* Backend interface. */
struct route_backend
{
  void (*add) (const int format,
               const void *const dest_addr,
               const size_t dest_addr_size,
               const unsigned char dest_addr_len,
               const void *const gw_addr,
               const size_t gw_addr_size,
               const unsigned int iface);

  void (*append) (const int format,
                  const void *const dest_addr,
                  const size_t dest_addr_size,
                  const unsigned char dest_addr_len,
                  const void *const gw_addr,
                  const size_t gw_addr_size,
                  const unsigned int iface);

  void (*change) (const int format,
                  const void *const dest_addr,
                  const size_t dest_addr_size,
                  const unsigned char dest_addr_len,
                  const void *const gw_addr,
                  const size_t gw_addr_size,
                  const unsigned int iface);

  void (*delete) (const int format,
                  const void *const dest_addr,
                  const size_t dest_addr_size,
                  const unsigned char dest_len);

  void (*prepend) (const int format,
                   const void *const dest_addr,
                   const size_t dest_addr_size,
                   const unsigned char dest_addr_len,
                   const void *const gw_addr,
                   const size_t gw_addr_size,
                   const unsigned int iface);

  void (*replace) (const int format,
                   const void *const dest_addr,
                   const size_t dest_addr_size,
                   const unsigned char dest_addr_len,
                   const void *const gw_addr,
                   const size_t gw_addr_size,
                   const unsigned int iface);

  const route_info_t * (*show) (const sa_family_t sa_family,
                                const short int resolve_names);
};
typedef struct route_backend route_backend_t;

/* route_backend.c */
extern const route_backend_t *route_backend;
extern void route_backend_init (void);

/* route_common.c */
extern void conv_inet_addr_to_name (const int format,
                                    const void *const addr,
                                    const size_t addr_size,
                                    char *const buffer,
                                    const size_t buffer_size,
                                    const short int resolve_names);
extern void conv_link_addr_to_name (const struct sockaddr *const sock,
                                    const socklen_t sock_size,
                                    char *const buffer,
                                    const size_t buffer_size,
				    const short int resolve_names);
extern void conv_name_to_addr (const int format,
                               const char *const name,
                               void *const buffer, const size_t buffer_size,
                               const short int resolve_names);
extern void convert_netmask (const int format, const unsigned char length,
                             char *const buffer, const size_t buffer_size);
extern void route_info_init (route_info_t * const route_info,
			     const short int resolve_names);
extern route_info_t *route_info_append (route_info_t *const tail);

#endif /* _ROUTE_H */
