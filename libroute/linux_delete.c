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

#include <errno.h>
#include <error.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>

#include "route.h"
#include "route_linux.h"

static uint32_t seqn = 0;

ssize_t linux_send_delroute (const int sockfd, const int format,
			     const void *const dest_addr,
			     const size_t dest_addr_size,
			     const unsigned char dest_len);

void
linux_delete (const int format,
              const void *const dest_addr,
              const size_t dest_addr_size,
              const unsigned char dest_len)
{
  int sockfd;
  size_t nsent;

  sockfd = socket (PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
  if (sockfd == -1)
    error (EXIT_FAILURE, errno, "socket");

  nsent = linux_send_delroute (sockfd, format,
                               dest_addr, dest_addr_size, dest_len);

  close (sockfd);
}

ssize_t
linux_send_delroute (const int sockfd, const int format,
		     const void *const dest_addr,
                     const size_t dest_addr_size,
		     const unsigned char dest_addr_len)
{
  int i;
  unsigned int size = 0;
  unsigned short int attr_types[] = { RTA_DST };
  ssize_t nsent;
  rtmroute_t msg;
  void *data;
  struct rtattr *attr;

  memset ((void *) &msg, 0, sizeof (msg));

  msg.nlmsghdr.nlmsg_len = NLMSG_LENGTH (sizeof (msg.rtmsg));
  msg.nlmsghdr.nlmsg_type = RTM_DELROUTE;
  msg.nlmsghdr.nlmsg_flags = NLM_F_REQUEST;
  msg.nlmsghdr.nlmsg_seq = seqn;
  msg.nlmsghdr.nlmsg_pid = 0;

  msg.rtmsg.rtm_family = format;
  msg.rtmsg.rtm_dst_len = dest_addr_len;
  msg.rtmsg.rtm_table = RT_TABLE_MAIN;
  msg.rtmsg.rtm_scope = RT_SCOPE_NOWHERE;

  attr = (struct rtattr *) RTM_RTA (&msg.rtmsg);
  size = RTM_PAYLOAD (&msg.nlmsghdr);
  for (i = 0; i < sizeof (attr_types) / sizeof (attr_types[0]); i++)
    {
      attr->rta_type = attr_types[i];
      switch (attr_types[i])
	{
	case RTA_DST:
	  attr->rta_len = RTA_LENGTH (dest_addr_size);
	  data = RTA_DATA (attr);
	  memmove (data, dest_addr, dest_addr_size);
	  break;

	default:
	  break;
	}

      msg.nlmsghdr.nlmsg_len = NLMSG_ALIGN (msg.nlmsghdr.nlmsg_len)
	+ RTA_ALIGN (attr->rta_len);
      attr = RTA_NEXT (attr, size);
    }

  seqn++;
  nsent = send (sockfd, (void *) &msg, msg.nlmsghdr.nlmsg_len, 0);
  if (nsent == -1)
    error (EXIT_FAILURE, errno, "send");

  return nsent;
}
