/*
 *  boot.c
 *
 *  Copyright (c) 2006 by Miklos Vajna <vmiklos@frugalware.org>
 *  parts are from gzip, Copyright (C) 1992-1993 by Jean-loup Gailly
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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>

/* Macros for getting two-byte and four-byte header values */
#define SH(p) ((unsigned short)(unsigned char)((p)[0]) | ((unsigned short)(unsigned char)((p)[1]) << 8))
#define LG(p) ((unsigned long)(SH(p)) | ((unsigned long)(SH((p)+2)) << 16))

int gunzip_size(char *path)
{
	int fd;
	unsigned char buf[8];

	fd = open(path, O_RDONLY);
	lseek(fd, (off_t)(-8), SEEK_END);
	read(fd, (char*)buf, sizeof(buf));
	return(LG(buf+4));
}

int boot_size(char *root, char *kernel, char *initrd)
{
	struct stat buf;
	int ret;
	char path[PATH_MAX+1];

	snprintf(path, PATH_MAX, "%s/%s", root, kernel);
	if(stat(path, &buf))
		return(0);
	ret = buf.st_size/1024;
	snprintf(path, PATH_MAX, "%s/%s", root, initrd);
	if(stat(path, &buf))
		return(0);
	ret += buf.st_size/1024;
	return(ret);
}

/*int main()
{
	printf("%d\n",
		boot_size("/home/vmiklos/darcs/frugalware-current", "boot/vmlinuz-2.6.16-fw5-i686", "boot/initrd-i686.img.gz"));
}*/
