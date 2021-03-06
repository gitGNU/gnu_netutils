/*
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <getopt.h>
#include <stdlib.h>

#include "talk.h"

void usage (void);

/*
 * talk:	A visual form of write. Using sockets, a two way
 *		connection is set up between the two people talking.
 *		With the aid of curses, the screen is split into two
 *		windows, and each users text is added to the window,
 *		one character at a time...
 *
 *		Written by Kipp Hickman
 *
 *		Modified to run under 4.1a by Clem Cole and Peter Moore
 *		Modified to run between hosts by Peter Moore, 8/19/82
 *		Modified to run under 4.1c by Peter Moore 3/17/83
 */

static const char *short_options = "hV";
static struct option long_options[] = {
  {"help", no_argument, 0, 'h'},
  {"version", no_argument, 0, 'V'},
  {0}
};

char *program_name;

int
main (int argc, char *argv[])
{
  int c;

  program_name = argv[0];

  while ((c = getopt_long (argc, argv, short_options, long_options, NULL))
	 != EOF)
    {
      switch (c)
	{
	case 'V':
	  printf ("talk (%s %s)\n", PACKAGE_NAME, PACKAGE_VERSION);
	  exit (0);

	case 'h':
	default:
	  usage ();
	  exit (0);
	}
    }

  get_names (argc, argv);
  init_display ();
  open_ctl ();
  open_sockt ();
  start_msgs ();
  if (!check_local ())
    invite_remote ();
  end_msgs ();
  set_edit_chars ();
  talk ();
}


static const char usage_str[] =
  "Usage: talk [OPTIONS...] USER\n"
  "\n"
  "Options are:\n"
  "       --help              Display usage instructions\n"
  "       --version           Display program version\n";

void
usage (void)
{
  printf ("%s\n" "Send bug reports to <%s>\n", usage_str, PACKAGE_BUGREPORT);
}
