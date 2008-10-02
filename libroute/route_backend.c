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

#include "route.h"

#ifdef __FreeBSD__
#include "route_bsd.h"
#endif  /* __FreeBSD__ */

#ifdef __linux__
#include "route_linux.h"
#endif /* __linux__ */

const route_backend_t *route_backend = NULL;

void
route_backend_init (void)
{
#ifdef __FreeBSD__
  route_backend = &bsd_backend;
#endif /* __FreeBSD__ */

#ifdef __linux__
  route_backend = &linux_backend;
#endif /* __linux__ */
}
