# Copyright (C) 2008 Free Software Foundation, Inc.
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

noinst_LTLIBRARIES = @libroute_BUILD@

EXTRA_LTLIBRARIES = libroute.la

libroute_la_SOURCES =	route_backend.c route_common.c route_$(KERNEL).c \
			$(KERNEL)_add.c $(KERNEL)_delete.c $(KERNEL)_show.c

EXTRA_libroute_la_SOURCES = 	bsd_add.c bsd_delete.c bsd_show.c linux_add.c \
				linux_delete.c linux_show.c route_bsd.c \
				route_linux.c

noinst_HEADERS = route.h route_bsd.h route_linux.h

INCLUDES =	-I$(top_srcdir)/lib -I$(top_srcdir)/libinetutils

libroute_la_LIBADD =	-L$(top_builddir)/lib -lgnu $(LIBNL)

