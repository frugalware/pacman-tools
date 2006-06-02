/*
 *  mkiso.h
 *
 *  Copyright (c) 2006 by Miklos Vajna <vmiklos@frugalware.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 *  USA.
 */

// for 650Mb = 1024*650 = 681574400 bytes with about 11Mb to spare
// FIXME: correct this after the kernel&initrd FIXME is done
#define CD_SIZE 665600
#define DVD_SIZE 4590208

typedef struct __volume_t
{
	char *arch;
	char *media;
	int serial;
	int size;
} volume_t;

typedef struct __isopkg_t
{
	PM_PKG *pkg;
	int priority;
} isopkg_t;
