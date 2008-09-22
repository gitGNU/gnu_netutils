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

#include <net/if.h>
#include <sys/socket.h>

#include "route.h"
#include "route_linux.h"

static uint32_t seqn = 0;

const route_info_t * linux_parse_msg (const rtmroute_t * msg, size_t size,
		                      const short int resolve_names);
size_t linux_recv_newroute (int sockfd, rtmroute_t * msg);
ssize_t linux_send_getroute (int sockfd);

const route_info_t *
linux_show (const short int resolve_names)
{
  int sockfd;
  const route_info_t *list;
  size_t nread;
  rtmroute_t msg;

  sockfd = socket (PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
  if (sockfd == -1)
    error (EXIT_FAILURE, errno, "socket");

  linux_send_getroute (sockfd);
  nread = linux_recv_newroute (sockfd, &msg);
  list = linux_parse_msg (&msg, nread, resolve_names);
  close (sockfd);
  return list;
}

const route_info_t *
linux_parse_msg (const rtmroute_t * msg, size_t nread,
		 const short int resolve_names)
{
  unsigned int size;
  const route_info_t *list = NULL;
  route_info_t *route_info = NULL;
  struct rta_cacheinfo *cacheinfo;
  struct rtattr *attr;

  while (NLMSG_OK (&msg->nlmsghdr, nread) != 0)
    {
      /* FIXME: IPv4 specific. */
      if ((msg->rtmsg.rtm_family != AF_INET)
	  || (msg->rtmsg.rtm_table != RT_TABLE_MAIN))
	goto next;

      route_info = route_info_append (route_info);
      if (list == NULL)
        list = route_info;
      route_info_init (route_info, resolve_names);

      route_info->dest_len = msg->rtmsg.rtm_dst_len;
      convert_netmask (msg->rtmsg.rtm_family, msg->rtmsg.rtm_dst_len,
		       route_info->dest_mask, sizeof (route_info->dest_mask));

      attr = (struct rtattr *) RTM_RTA (&msg->rtmsg);
      size = RTM_PAYLOAD (&msg->nlmsghdr);
      while (RTA_OK (attr, size) != 0)
	{
	  switch (attr->rta_type)
	    {
	    case RTA_CACHEINFO:
	      cacheinfo = (struct rta_cacheinfo *) RTA_DATA (attr);
	      route_info->ref = cacheinfo->rta_clntref;
	      route_info->use = cacheinfo->rta_used;
	      break;

            case RTA_DST:
              route_info->dest_present = 1;
              conv_inet_addr_to_name (msg->rtmsg.rtm_family,
                                      RTA_DATA (attr), RTA_PAYLOAD (attr),
                                      route_info->dest,
                                      sizeof (route_info->dest),
                                      resolve_names);
	      break;

	    case RTA_SRC:
	      route_info->src_present = 1;
	      conv_inet_addr_to_name (msg->rtmsg.rtm_family,
                                      RTA_DATA (attr), RTA_PAYLOAD (attr),
                                      route_info->src,
                                      sizeof (route_info->src),
                                      resolve_names);
              break;

	    case RTA_OIF:
              if_indextoname (*(int *) RTA_DATA (attr), route_info->iface);
	      break;

	    case RTA_GATEWAY:
	      route_info->gateway_present = 1;
	      conv_inet_addr_to_name (msg->rtmsg.rtm_family,
                                      RTA_DATA (attr), RTA_PAYLOAD (attr),
                                      route_info->gateway,
                                      sizeof (route_info->gateway),
                                      resolve_names);
	      break;

	    case RTA_PRIORITY:
	      route_info->metric = *(int *) RTA_DATA (attr);
	      break;

	    case RTA_PREFSRC:
	      route_info->pref_src_present = 1;
	      conv_inet_addr_to_name (msg->rtmsg.rtm_family,
                                      RTA_DATA (attr), RTA_PAYLOAD (attr),
                                      route_info->pref_src,
                                      sizeof (route_info->pref_src),
                                      resolve_names);
	      break;

	    default:
	      break;
	    }

	  attr = RTA_NEXT (attr, size);
	}

    next:
      msg = (rtmroute_t *) NLMSG_NEXT (&msg->nlmsghdr, nread);
    }

  return list;
}

size_t
linux_recv_newroute (int sockfd, rtmroute_t * msg)
{
  size_t nread = 0;
  size_t size = 0;

  if (msg == NULL)
    return -1;

  nread = recv (sockfd, (void *) msg, sizeof (*msg), 0);

  size = nread;
  while (NLMSG_OK (&msg->nlmsghdr, size) != 0)
    {
      if (msg->nlmsghdr.nlmsg_type == NLMSG_ERROR)
	error (EXIT_FAILURE, 0,
	       "netlink message truncated and can not be parsed");
      else if (msg->nlmsghdr.nlmsg_pid != getpid ())
	error (0, 0, "netlink message has invalid PID");

      if (msg->nlmsghdr.nlmsg_type == NLMSG_DONE)
	break;

      if ((msg->nlmsghdr.nlmsg_flags & NLM_F_MULTI) == 0)
	break;

      msg = (rtmroute_t *) NLMSG_NEXT (&msg->nlmsghdr, size);
    }

  return nread;
}

ssize_t
linux_send_getroute (int sockfd)
{
  rtmroute_t msg;
  ssize_t nsent;

  memset ((void *) &msg, 0, sizeof (msg));
  msg.nlmsghdr.nlmsg_len = NLMSG_LENGTH (sizeof (msg.rtmsg));
  msg.nlmsghdr.nlmsg_type = RTM_GETROUTE;
  msg.nlmsghdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
  msg.nlmsghdr.nlmsg_seq = seqn;
  msg.nlmsghdr.nlmsg_pid = 0;

  seqn++;
  nsent = send (sockfd, (void *) &msg, msg.nlmsghdr.nlmsg_len, 0);
  if (nsent == -1)
    error (EXIT_FAILURE, errno, "send");

  return nsent;
}
