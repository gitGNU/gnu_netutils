# Copyright (C) 2005, 2007 Free Software Foundation, Inc.
# This file is part of Netutils.
#
# Netutils is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# Netutils is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Netutils; see the file COPYING.  If not, write
# to the Free Software Foundation, Inc., 51 Franklin Street,
# Fifth Floor, Boston, MA 02110-1301 USA.

bin_PROGRAMS = @talk_BUILD@

EXTRA_PROGRAMS = talk

talk_SOURCES = ctl.c ctl_transact.c display.c get_addrs.c get_names.c init_disp.c invite.c io.c look_up.c msgs.c talk.c

noinst_HEADERS = talk.h talk_ctl.h

man_MANS = talk.1

INCLUDES = -I$(top_srcdir)/lib -I../lib -I$(top_srcdir)/libinetutils $(NCURSES_INCLUDE)

LDADD = $(LIBCURSES) -L../libinetutils -linetutils -L../lib -lgnu

EXTRA_DIST = $(man_MANS)
