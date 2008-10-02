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

#include <config.h>

#include <argp.h>
#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/types.h>

#include "libinetutils.h"
#include "progname.h"
#include "route.h"
#include "xalloc.h"

enum route_command
{
  ROUTE_COMMAND_ADD,
  ROUTE_COMMAND_APPEND,
  ROUTE_COMMAND_CHANGE,
  ROUTE_COMMAND_DELETE,
  ROUTE_COMMAND_PREPEND,
  ROUTE_COMMAND_REPLACE,
  ROUTE_COMMAND_SHOW,
  ROUTE_COMMAND_INVALID
};
typedef enum route_command route_command_t;

struct route_options
{
  route_command_t route_command;
  void *route_dest_addr;
  size_t route_dest_addr_size;
  unsigned char route_dest_len;
  void *route_gw_addr;
  size_t route_gw_addr_size;
  sa_family_t route_sa_family;
  short int route_resolve_names;
  unsigned int route_iface;
};
typedef struct route_options route_options_t;

static const route_info_t *list = NULL;
static route_options_t options;
static void (*show_header_action) (void) = NULL;
static void (*show_info_action) (const route_info_t * const route_info) = NULL;

void cleanup (void);
void route_options_init (route_options_t *const options);
void show_ip_header (void);
void show_netstat_header (void);
void show_route_header (void);
void show_ip_info (const route_info_t * const route_info);
void show_netstat_info (const route_info_t * const route_info);
void show_route_info (const route_info_t * const route_info);

ARGP_PROGRAM_DATA ("route", "2008", "Debarshi Ray");

const char args_doc[] = "[COMMAND [ARG...]]";
const char doc[] = "Show or manipulate the network routing tables."
  "\vOptions marked with (root only) are available only to " "superuser.";

/* Define keys for long options that do not have short counterparts. */
enum
{
  ARG_DEV = 256,
  ARG_HOST,
  ARG_IP,
  ARG_NET,
  ARG_NETMASK,
  ARG_REJECT
};

static struct argp_option argp_options[] = {
#define GRP 0
  {NULL, 0, NULL, 0, "Commands controlling request types:", GRP},
  {"add", ROUTE_COMMAND_ADD, NULL, OPTION_DOC, "Add a new route (root only)",
   GRP + 1},
  {"append", ROUTE_COMMAND_APPEND, NULL, OPTION_DOC, "Append a new route "
   "(root only)", GRP + 1},
  {"change", ROUTE_COMMAND_CHANGE, NULL, OPTION_DOC, "Change an existing "
   "route (root only)", GRP + 1},
  {"delete", ROUTE_COMMAND_DELETE, NULL, OPTION_DOC, "Delete a route (root "
   "only)", GRP + 1},
  {"prepend", ROUTE_COMMAND_PREPEND, NULL, OPTION_DOC, "Prepend a new route "
   "(root only)", GRP + 1},
  {"replace", ROUTE_COMMAND_REPLACE, NULL, OPTION_DOC, "Change an existing "
   "route or create a new one otherwise (root only)", GRP + 1},
  {"show", ROUTE_COMMAND_SHOW, NULL, OPTION_DOC, "Display the contents of "
   "the routing tables or selected route(s) (default)", GRP + 1},
  {"list", ROUTE_COMMAND_SHOW, NULL, OPTION_ALIAS | OPTION_DOC, "Display the "
   "contents of the routing tables or selected route(s) (default)", GRP + 1},
#undef GRP
#define GRP 10
  {NULL, 0, NULL, 0, "General options for commands:", GRP},
  {"dev", ARG_DEV, "INTERFACE", 0,
   "Force the route to be associated with the " "specified INTERFACE",
   GRP + 1},
  {"family", 'A', "AF", 0, "Use address Family AF (inet (default) or inet6)",
   GRP + 1},
  {"fib", 'F', NULL, 0, "Display Forwarding Information Base (default)",
   GRP + 1},
  {"gateway", 'g', "GW", 0, "Use GW as the gateway for a route", GRP + 1},
  {"gw", 'g', "GW", OPTION_ALIAS, "Use GW as the gateway for a route",
   GRP + 1},
  {"host", ARG_HOST, "DEST", 0, "Destination DEST is a host", GRP + 1},
  {"ip", ARG_IP, NULL, 0, "Use ip format for displaying", GRP + 1},
  {"net", ARG_NET, "DEST", 0, "Destination DEST is a network", GRP + 1},
  {"netmask", ARG_NETMASK, "MASK", 0, "Use MASK as the netmask for a network "
   "route", GRP + 1},
  {"numeric", 'n', NULL, 0, "Do not resolve host addresses", GRP + 1},
  {"netstat", 'e', NULL, 0, "Use netstat format for displaying", GRP + 1},
  {"extend", 'e', NULL, OPTION_ALIAS, "Use netstat format for displaying",
   GRP + 1},
  {"reject", ARG_REJECT, NULL, 0, "Add a blocking route", GRP + 1},
#undef GRP
  {NULL}
};

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  char *endptr;
  route_options_t *options = (route_options_t *) state->input;

  switch (key)
    {
    case ARG_DEV:
      options->route_iface = if_nametoindex (arg);
      if (options->route_iface == 0)
        error (EXIT_FAILURE, 0, "invalid device (%s)", arg);
      break;

    case 'A':
      if (strncmp("inet6", arg, strlen ("inet6")) == 0)
        options->route_sa_family = AF_INET6;
      else if (strncmp("inet", arg, strlen ("inet")) == 0)
        options->route_sa_family = AF_INET;
      break;

    /* FIXME: incomplete. */
    case 'F':
      break;

    case 'g':
      options->route_gw_addr_size = sizeof (struct in_addr);
      options->route_gw_addr = xmalloc (options->route_gw_addr_size);
      conv_name_to_addr (options->route_sa_family, arg,
                         options->route_gw_addr,
                         options->route_gw_addr_size,
                         options->route_resolve_names);
      break;

    case ARG_HOST:
      options->route_dest_addr_size = sizeof (struct in_addr);
      options->route_dest_addr = xmalloc (options->route_dest_addr_size);
      conv_name_to_addr (options->route_sa_family, arg,
                         options->route_dest_addr,
                         options->route_dest_addr_size,
                         options->route_resolve_names);
      options->route_dest_len = sizeof (struct in_addr) * 8;
      break;

    case ARG_IP:
      show_header_action = show_ip_header;
      show_info_action = show_ip_info;
      break;

    case ARG_NET:
      options->route_dest_addr_size = sizeof (struct in_addr);
      options->route_dest_addr = xmalloc (options->route_dest_addr_size);
      conv_name_to_addr (options->route_sa_family, arg,
                         options->route_dest_addr,
                         options->route_dest_addr_size,
                         options->route_resolve_names);
      break;

    case ARG_NETMASK:
      options->route_dest_len = strtol (arg, &endptr, 10);
      if (*endptr != '\0')
        argp_error (state, "invalid value (`%s' near `%s')", arg, endptr);
      break;

    case 'n':
      options->route_resolve_names = 0;
      break;

    case 'e':
      show_header_action = show_netstat_header;
      show_info_action = show_netstat_info;
      break;

    /* FIXME: incomplete. */
    case ARG_REJECT:
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }

  return 0;
}

static struct argp argp = {argp_options, parse_opt, args_doc, doc};

static int
parse_com (const int argc, char *argv[], route_options_t * const options)
{
  const char *str;
  int i;

  for (i = 1; (argc > 1) && (argp_options[i].name != NULL); i++)
    {
      if ((argp_options[i].flags & OPTION_DOC) == 0)
	continue;

      str = strstr (argp_options[i].name, argv[1]);
      if (str == argp_options[i].name)
        {
	  options->route_command = argp_options[i].key;
          return 0;
	}
    }

  return -1;
}

int
main (int argc, char *argv[])
{
  int status;
  const route_info_t *route_info = NULL;

  atexit (cleanup);
  set_program_name (argv[0]);

  /* Set default actions */
  route_options_init (&options);

  /* Parse command */
  status = parse_com (argc, argv, &options);
  if (status == 0)
    {
      argc--;
      /* Preserve argv[0]. */
      argv[1] = argv[0];
      argv++;
    }

  /* Parse command options and arguments */
  argp_parse (&argp, argc, argv, 0, NULL, (void *) &options);

  route_backend_init ();

  switch (options.route_command)
    {
    case ROUTE_COMMAND_ADD:
      (*route_backend->add) (AF_INET,
                             options.route_dest_addr,
                             options.route_dest_addr_size,
                             options.route_dest_len,
                             options.route_gw_addr,
                             options.route_gw_addr_size,
                             options.route_iface);
      break;

    case ROUTE_COMMAND_APPEND:
      (*route_backend->append) (AF_INET,
                                options.route_dest_addr,
                                options.route_dest_addr_size,
                                options.route_dest_len,
                                options.route_gw_addr,
                                options.route_gw_addr_size,
                                options.route_iface);
      break;

    case ROUTE_COMMAND_CHANGE:
      (*route_backend->change) (AF_INET,
                                options.route_dest_addr,
                                options.route_dest_addr_size,
                                options.route_dest_len,
                                options.route_gw_addr,
                                options.route_gw_addr_size,
                                options.route_iface);
      break;

    case ROUTE_COMMAND_DELETE:
      (*route_backend->delete) (AF_INET,
                                options.route_dest_addr,
                                options.route_dest_addr_size,
                                options.route_dest_len);
      break;

    case ROUTE_COMMAND_PREPEND:
      (*route_backend->prepend) (AF_INET,
                                 options.route_dest_addr,
                                 options.route_dest_addr_size,
                                 options.route_dest_len,
                                 options.route_gw_addr,
                                 options.route_gw_addr_size,
                                 options.route_iface);
      break;

    case ROUTE_COMMAND_REPLACE:
      (*route_backend->replace) (AF_INET,
                                 options.route_dest_addr,
                                 options.route_dest_addr_size,
                                 options.route_dest_len,
                                 options.route_gw_addr,
                                 options.route_gw_addr_size,
                                 options.route_iface);
      break;

    case ROUTE_COMMAND_SHOW:
      (*show_header_action) ();
      list = route_info = (*route_backend->show) (options.route_sa_family,
                                                  options.route_resolve_names);
      while (route_info != NULL)
        {
          (*show_info_action) (route_info);
          route_info = route_info->next;
        }
      break;

    default:
      break;
    }

  return 0;
}

void
cleanup (void)
{
  void *old;

  while (list != NULL)
    {
      old = (void *) list;
      list = list->next;
      free (old);
    }

  if (options.route_dest_addr != NULL)
    free (options.route_dest_addr);
  if (options.route_gw_addr != NULL)
    free (options.route_gw_addr);
}

void
route_options_init (route_options_t *const options)
{
  memset ((void *) options, 0, sizeof (*options));
  options->route_command = ROUTE_COMMAND_SHOW;
  options->route_resolve_names = 1;
  options->route_sa_family = AF_INET;
  show_header_action = show_route_header;
  show_info_action = show_route_info;
}

void
show_ip_header (void)
{
  /* No header for ip-like display. */
}

void
show_netstat_header (void)
{
  puts ("Kernel network routing table");
  printf ("%-15s %-17s %-15s %-5s %-5s\n",
	  "Destination",
          "Gateway",
          "Netmask",
          "Flags",
          "Iface");
}

void
show_route_header (void)
{
  puts ("Kernel network routing table");
  printf ("%-15s %-17s %-15s %-5s %-6s %-5s %-5s %-5s\n",
	  "Destination",
          "Gateway",
          "Netmask",
          "Flags",
          "Metric",
          "Ref",
          "Use",
          "Iface");
}

void
show_ip_info (const route_info_t * const route_info)
{
  if (route_info->dest_present != 0)
    printf ("%s/%" PRIu8 " ", route_info->dest, route_info->dest_len);
  else
    printf ("%s ", route_info->dest);

  if (route_info->gateway_present != 0)
    printf ("via %s ", route_info->gateway);

  printf ("dev %s ", route_info->oif_name);

  if (route_info->src_present != 0)
    printf ("from %s ", route_info->src);

  if (route_info->pref_src_present != 0)
    printf ("src %s ", route_info->pref_src);

  putchar ('\n');
}

void
show_netstat_info (const route_info_t * const route_info)
{
  printf ("%-15s %-17s %-15s %-5s %-5s\n",
          route_info->dest,
          route_info->gateway,
          route_info->dest_mask,
          route_info->flag_str,
          route_info->oif_name);
}

void
show_route_info (const route_info_t * const route_info)
{
  printf ("%-15s %-17s %-15s %-5s %-6"PRIu32" %-5"PRIu32" %-5"PRIu32" %-5s\n",
          route_info->dest,
          route_info->gateway,
	  route_info->dest_mask,
          route_info->flag_str,
          route_info->metric,
          route_info->ref,
	  route_info->use,
          route_info->oif_name);
}
