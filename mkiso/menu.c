/*
 *  menu.c
 *
 *  Copyright (c) 2006, 2007, 2008, 2009, 2010, 2011 by Miklos Vajna <vmiklos@frugalware.org>
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
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <glib.h>
#include <pacman.h>

#include "mkiso.h"
#include "boot.h"

extern char *fst_root;
extern char *fst_ver;
extern char *fst_codename;

char *mkmenu(volume_t *volume, int isolinux)
{
	char *flist = strdup("/tmp/mkiso_XXXXXX");
	char *kernel = detect_kernel(volume->arch);
	FILE *fp;

	mkstemp(flist);
	if(!(fp = fopen(flist, "w")))
		return(NULL);

	if (!isolinux) {
		struct stat buf;
		fprintf(fp, "default=0\n"
				"timeout=10\n");
		if(!stat("/boot/grub/message", &buf))
			fprintf(fp, "gfxmenu /boot/grub/message\n\n");

		fprintf(fp, "title Frugalware %s (%s) - %s\n",
				fst_ver, fst_codename, kernel);
		fprintf(fp, "\tkernel /boot/vmlinuz-%s quiet\n", kernel);
		fprintf(fp, "\tinitrd /boot/initrd-%s.img.gz\n",
				volume->arch);

		fprintf(fp, "title Frugalware %s (%s) - %s (vga fb)\n",
				fst_ver, fst_codename, kernel);
		fprintf(fp, "\tkernel /boot/vmlinuz-%s quiet vga=791\n", kernel);
		fprintf(fp, "\tinitrd /boot/initrd-%s.img.gz\n",
				volume->arch);

		fprintf(fp, "title Frugalware %s (%s) - %s (gui)\n",
				fst_ver, fst_codename, kernel);
		fprintf(fp, "\tkernel /boot/vmlinuz-%s quiet\n", kernel);
		fprintf(fp, "\tinitrd /boot/initrd-%s-gui.img.gz\n",
				volume->arch);

	} else {
		fprintf(fp, "timeout 100\n");
		fprintf(fp, "ui vesamenu.c32\n");
		fprintf(fp, "menu title Frugalware Linux\n\n");

		fprintf(fp, "menu background splash.png\n");
		fprintf(fp, "menu vshift 10\n");
		fprintf(fp, "menu margin 0\n\n");

		fprintf(fp, "label frugalware\n");
		fprintf(fp, "\tmenu label Frugalware %s (%s) - %s\n",
			fst_ver, fst_codename, kernel);
		fprintf(fp, "\tlinux /boot/vmlinuz\n");
		fprintf(fp, "\tinitrd /boot/initrd\n");

		fprintf(fp, "label frugalwarevgafb\n");
		fprintf(fp, "\tmenu label Frugalware %s (%s) - %s (vga fb)\n",
			fst_ver, fst_codename, kernel);
		fprintf(fp, "\tlinux /boot/vmlinuz\n");
		fprintf(fp, "\tinitrd /boot/initrd\n");
		fprintf(fp, "\tappend vga=791\n");

		fprintf(fp, "label frugalwaregui\n");
		fprintf(fp, "\tmenu label Frugalware %s (%s) - %s (gui)\n",
			fst_ver, fst_codename, kernel);
		fprintf(fp, "\tlinux /boot/vmlinuz\n");
		fprintf(fp, "\tinitrd /boot/initrd-gui\n");

	}
	fclose(fp);
	free(kernel);
	return(flist);
}

char *mkbootmsg(volume_t *volume)
{
	char *flist = strdup("/tmp/mkiso_XXXXXX");
	FILE *fp;
	char *kernel = detect_kernel(volume->arch);

	mkstemp(flist);
	if(!(fp = fopen(flist, "w")))
		return(NULL);

	fprintf(fp, "Frugalware %s (%s) - %s\n\n", fst_ver, fst_codename, kernel);
	fprintf(fp, "To boot the kernel, just hit enter, or use 'install'.\n\n");
	fprintf(fp, "If the system fails to boot at all (the typical symptom is a white screen which\n"
			"doesn't go away), use 'install video=ofonly'.\n\n");
	fclose(fp);
	free(kernel);
	return(flist);
}

char *mkconf(volume_t *volume)
{
	char *flist = strdup("/tmp/mkiso_XXXXXX");
	FILE *fp;
	char *kernel = detect_kernel(volume->arch);
	char *ptr = g_strdup_printf("%s/boot/initrd-%s.img.gz", fst_root, volume->arch);

	mkstemp(flist);
	if(!(fp = fopen(flist, "w")))
		return(NULL);

	fprintf(fp, "default=install\n"
		"root=/dev/ram\n"
		"message=boot/yaboot/boot.msg\n");
	fprintf(fp, "image=boot/vmlinux-%s\n", kernel);
	fprintf(fp, "label=install\n");
	fprintf(fp, "\tinitrd=boot/initrd-%s.img.gz\n", volume->arch);
	fprintf(fp, "\tinitrd-size=%d\n", gunzip_size(ptr)/1024);
	fprintf(fp, "\tread-write\n");

	fclose(fp);
	free(ptr);
	free(kernel);
	return(flist);
}
