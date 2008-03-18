/*
 *  mkiso.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>

#include <pacman.h>
#include <glib.h>

#include "mkiso.h"
#include "xml.h"
#include "boot.h"
#include "menu.h"

GList *isopkgs=NULL;
GList *volumes=NULL;
GList *isogrps=NULL;

char *fst_root=NULL;
char *fst_ver=NULL;
char *fst_codename=NULL;
char *out_dir=NULL;
char *lang=NULL;

int strrcmp(const char *haystack, const char *needle)
{
	if(strlen(haystack) < strlen(needle))
		return(1);
	return(strcmp(haystack + strlen(haystack) - strlen(needle), needle));
}

int detect_priority(PM_PKG *pkg)
{
	PM_LIST *i = pacman_pkg_getinfo(pkg, PM_PKG_GROUPS);
	char *grp = pacman_list_getdata(i);
	char *name = pacman_pkg_getinfo(pkg, PM_PKG_NAME);

	if(strrcmp(grp, "-extra"))
	{
		if(!strcmp(grp, "base") || !strcmp(grp, "apps") || !strcmp(grp, "lib") || !strcmp(grp, "multimedia") || !strcmp(grp, "network") || !strcmp(grp, "devel"))
			return(80);
		else
			return(60);
	}
	else
	{
		if(!strcmp(grp, "locale-extra"))
		{
			if(lang)
			{
				char *ptr = g_strdup_printf("-%s", lang);
				if(!strrcmp(name, ptr))
				{
					free(ptr);
					return(40);
				}
				else
				{
					free(ptr);
					return(20);
				}
			}
			else
				return(40);
		}
		else
			return(20);
	}
	PRINTF("possible invalid group '%s' for package '%s'\n", grp, (char*)pacman_pkg_getinfo(pkg, PM_PKG_NAME));
	return(0);
}

int sort_isopkgs(gconstpointer a, gconstpointer b)
{
	const isopkg_t *pa = a;
	const isopkg_t *pb = b;
	return ((pa->priority < pb->priority) ? 1 : -1);
}

int add_targets()
{
	int i;
	for (i=0; i<g_list_length(isopkgs); i++)
	{
		isopkg_t *isopkg = g_list_nth_data(isopkgs, i);
		char *pkgname = pacman_pkg_getinfo(isopkg->pkg, PM_PKG_NAME);
		if(pacman_trans_addtarget(pkgname))
		{
			PRINTF("failed to add target '%s' (%s)\n", pkgname, pacman_strerror(pm_errno));
			return(1);
		}
	}
	return(0);
}

char *get_filename(char *version, char *arch, char *media, int volume)
{
	char *ptr=NULL;

	if(out_dir)
		ptr = g_strdup_printf("%s/", out_dir);
	else
		ptr = strdup("");
	if(volume)
		return(g_strdup_printf("%sfrugalware-%s-%s-%s%d.iso", ptr, version, arch, media, volume));
	else
		return(g_strdup_printf("%sfrugalware-%s-%s-%s.iso", ptr, version, arch, media));
	free(ptr);
}

char *get_label(char *version, char *arch, char *media, int volume)
{
	if(volume)
		return(g_strdup_printf("Frugalware %s-%s Install %s #%d", version, arch, media, volume));
	else
		return(g_strdup_printf("Frugalware %s-%s Install %s", version, arch, media));
}

int iso_add(int dryrun, FILE *fp, char *fmt, ...)
{
	va_list args;

	char str[PATH_MAX];

	va_start(args, fmt);
	vsnprintf(str, PATH_MAX, fmt, args);
	va_end(args);

	if(!fp)
		return(1);
	if(!dryrun)
		fprintf(fp, "%s=%s\n", str, str);
	else
		printf("%s\n", str);
	return(0);
}

char *detect_kernel(char *arch)
{
	DIR *dir;
	struct dirent *ent;
	char *ptr = g_strdup_printf("%s/boot", fst_root);

	dir = opendir(ptr);
	free(ptr);
	if (!dir)
		return(NULL);
	while ((ent = readdir(dir)))
	{
		if(!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
			continue;
		if(strstr(ent->d_name, "vmlinuz-") && strstr(ent->d_name, arch))
		{
			closedir(dir);
			return(strdup(ent->d_name+strlen("vmlinuz-")));
		}
	}
	closedir(dir);
	return(NULL);
}

int isogrp_strin(char *str)
{
	int i;

	for(i=0;i<g_list_length(isogrps);i++)
	{
		isogrp_t *grp = g_list_nth_data(isogrps, i);
		if(!strcmp(grp->name, str))
			return(1);
	}
	return(0);
}

int isogrp_add(PM_PKG *pkg)
{
	int i;
	PM_LIST *grps = pacman_pkg_getinfo(pkg, PM_PKG_GROUPS);
	char *grp = pacman_list_getdata(grps);

	if(isogrp_strin(grp))
	{
		for(i=0;i<g_list_length(isogrps);i++)
		{
			isogrp_t *isogrp = g_list_nth_data(isogrps, i);
			if(!strcmp(isogrp->name, grp))
			{
				isogrp->size += (long int)pacman_pkg_getinfo(pkg, PM_PKG_SIZE)/1024;
				isogrp->usize += (long int)pacman_pkg_getinfo(pkg, PM_PKG_USIZE)/1024;
				break;
			}
		}
	}
	else
	{
		isogrp_t *isogrp;
		if((isogrp = (isogrp_t *)malloc(sizeof(isogrp_t)))==NULL)
		{
			PRINTF("out of memory!\n");
			return(1);
		}
		isogrp->name = strdup(grp);
		isogrp->size = (long int)pacman_pkg_getinfo(pkg, PM_PKG_SIZE)/1024;
		isogrp->usize = (long int)pacman_pkg_getinfo(pkg, PM_PKG_USIZE)/1024;
		isogrps = g_list_append(isogrps, isogrp);
	}
	return(0);
}

void isogrp_stat(FILE *fp)
{
	int i;
	int sum=0, usum=0;

	for(i=0;i<g_list_length(isogrps);i++)
	{
		isogrp_t *grp = g_list_nth_data(isogrps, i);
		sum += (int)grp->size/1024;
		usum += (int)grp->usize/1024;
		fprintf(fp, "%-16s %4dMB %4dMB\n", grp->name, (int)grp->size/1024, (int)grp->usize/1024);
		free(grp->name);
	}
	fprintf(fp, "%-16s %4dMB %4dMB\n", "total", sum, usum);
}

int mkiso(volume_t *volume, int countonly, int stable, int dryrun)
{
	PM_LIST *i, *sorted;
	int total=0, myvolume=1, bootsize;
	char *fname=get_filename(fst_ver, volume->arch, volume->media, volume->serial);
	char *label=get_label(fst_ver, volume->arch, volume->media, volume->serial);
	char *flist, *cmdline, *ptr, *kptr, *iptr, *menu;
	char cwd[PATH_MAX] = "";
	FILE *fp;

	flist = strdup("/tmp/mkiso_XXXXXX");
	mkstemp(flist);

	if((fp=fopen(flist, "w"))==NULL)
		return(1);
	// initial filelist
	iso_add(dryrun, fp, "AUTHORS");
	iso_add(dryrun, fp, "ChangeLog.txt");
	iso_add(dryrun, fp, "LICENSE");
	iso_add(dryrun, fp, "docs");
	ptr = detect_kernel(volume->arch);
	kptr = g_strdup_printf("boot/vmlinuz-%s", ptr);
	free(ptr);
	iso_add(dryrun, fp, kptr);
	iptr = g_strdup_printf("boot/initrd-%s.img.gz", volume->arch);
	iso_add(dryrun, fp, iptr);
	// how many space is needed for the kernel & initrd?
	bootsize = boot_size(fst_root, kptr, iptr);
	free(kptr);
	free(iptr);
	iso_add(dryrun, fp, "boot/grub/message");
	iso_add(dryrun, fp, "boot/grub/stage2_eltorito");
	menu = mkmenu(volume);
	fprintf(fp, "boot/grub/menu.lst=%s\n", menu);
	// first volume of !net medias
	if(volume->serial==1)
	{
		if(stable)
			iso_add(dryrun, fp, "frugalware-%s/frugalware.fdb", volume->arch);
		else
			iso_add(dryrun, fp, "frugalware-%s/frugalware-current.fdb", volume->arch);
	}

	sorted = pacman_trans_getinfo(PM_TRANS_PACKAGES);
	for(i = pacman_list_first(sorted); i; i = pacman_list_next(i))
	{
		PM_SYNCPKG *sync = pacman_list_getdata(i);
		PM_PKG *pkg = pacman_sync_getinfo(sync, PM_SYNC_PKG);
		long int size = (long int)pacman_pkg_getinfo(pkg, PM_PKG_SIZE)/1024;
		char *ptr;
		if (total+size > volume->size - bootsize)
		{
			total=0;
			myvolume++;
		}
		total += size;
		if(myvolume==volume->serial)
		{
			ptr = strdup("frugalware-%s/%s-%s-%s%s");
			iso_add(dryrun, fp, ptr,
			volume->arch,
			(char*)pacman_pkg_getinfo(pkg, PM_PKG_NAME),
			(char*)pacman_pkg_getinfo(pkg, PM_PKG_VERSION),
			(char*)pacman_pkg_getinfo(pkg, PM_PKG_ARCH),
			PM_EXT_PKG);
			isogrp_add(pkg);
		}
	}
	fclose(fp);

	getcwd(cwd, PATH_MAX);
	chdir(fst_root);

	cmdline = g_strdup_printf("mkisofs -o %s "
		"-R -J -V \"Frugalware Install\" "
		"-A \"%s\" "
		"-graft-points "
		"-path-list %s "
		"-hide-rr-moved "
		"-b boot/grub/stage2_eltorito "
		"-v -d -N -no-emul-boot -boot-load-size 4 -boot-info-table",
		fname, label, flist);
	ptr = g_strdup_printf("%s.lst", fname);
	if((fp=fopen(ptr, "w"))==NULL)
		return(1);
	free(ptr);
	isogrp_stat(fp);
	fclose(fp);
	if(!countonly && !dryrun)
		system(cmdline);
	else if(!dryrun)
		PRINTF("expected volume number: %d\n", myvolume);

	free(fname);
	free(label);
	unlink(menu);
	free(menu);
	unlink(flist);
	free(flist);
	free(cmdline);
	chdir(cwd);
	return(0);
}

void cb_log(unsigned short level, char *msg)
{
	PRINTF("%s\n", msg);
}

PM_DB *db_register(volume_t *volume, char *treename)
{
	PM_DB *db;
	char *ptr;

	PRINTF("registering database %s...", treename);
	if(!(db = pacman_db_register(treename)))
	{
		PRINTF("could not register '%s' database (%s)\n", treename, pacman_strerror(pm_errno));
		return(NULL);
	}
	ptr = g_strdup_printf("file://%s/frugalware-%s", fst_root, volume->arch);
	pacman_db_setserver(db, ptr);
	free(ptr);
	if(pacman_db_update(0, db) == -1)
	{
		PRINTF("failed to update %s (%s)\n", treename, pacman_strerror(pm_errno));
		return(NULL);
	}
	PRINTF(" done.\n");
	return(db);
}

/* does the same thing as 'rm -rf' */
int rmrf(char *path)
{
	int errflag = 0;
	struct dirent *dp;
	DIR *dirp;
	char name[PATH_MAX];

	if(!unlink(path))
		return(0);
	else
	{
		if(errno == ENOENT)
			return(0);
		else if(errno == EPERM)
		{
			/* fallthrough */
		}
		else if(errno == EISDIR)
		{
			/* fallthrough */
		}
		else if(errno == ENOTDIR)
			return(1);
		else
			/* not a directory */
			return(1);

		if((dirp = opendir(path)) == (DIR *)-1)
			return(1);
		for(dp = readdir(dirp); dp != NULL; dp = readdir(dirp))
			if(dp->d_ino)
			{
				sprintf(name, "%s/%s", path, dp->d_name);
				if(strcmp(dp->d_name, "..") && strcmp(dp->d_name, "."))
					errflag += rmrf(name);
			}
		closedir(dirp);
		if(rmdir(path))
			errflag++;
		return(errflag);
	}
	return(0);
}

int prepare(volume_t *volume, char *tmproot, int countonly, int stable, int dryrun)
{
	PM_LIST *i, *junk;
	PM_DB *db_local, *db_sync;

	if(pacman_initialize(tmproot) == -1)
		PRINTF("failed to initilize pacman library (%s)\n", pacman_strerror(pm_errno));
	pacman_set_option(PM_OPT_LOGCB, (long)cb_log);
	pacman_set_option(PM_OPT_LOGMASK, (long)PM_LOG_WARNING);
	if((db_local = pacman_db_register("local"))==NULL)
		PRINTF("could not register 'local' database (%s)\n", pacman_strerror(pm_errno));
	if(stable)
		db_sync = db_register(volume, "frugalware");
	else
		db_sync = db_register(volume, "frugalware-current");

	if(pacman_trans_init(PM_TRANS_TYPE_SYNC, PM_TRANS_FLAG_NOCONFLICTS, NULL, NULL, NULL) == -1)
		PRINTF("failed to init transaction (%s)\n", pacman_strerror(pm_errno));

	if(strcmp(volume->media, "net"))
	{
		for(i=pacman_db_getpkgcache(db_sync); i; i=pacman_list_next(i))
		{
			PM_PKG *pkg=pacman_list_getdata(i);
			isopkg_t *isopkg;

			if((isopkg = (isopkg_t *)malloc(sizeof(isopkg_t)))==NULL)
			{
				PRINTF("out of memory!\n");
				return(1);
			}
			isopkg->pkg = pkg;
			isopkg->priority = detect_priority(pkg);
			isopkgs = g_list_append(isopkgs, isopkg);
		}
	}

	isopkgs = g_list_sort(isopkgs, sort_isopkgs);
	add_targets();

	PRINTF("preparing the transaction...");
	if(pacman_trans_prepare(&junk) == -1)
	{
		PRINTF("failed to prepare transaction (%s)\n", pacman_strerror(pm_errno));
		for(i = pacman_list_first(junk); i; i = pacman_list_next(i))
		{
			PM_DEPMISS *miss = pacman_list_getdata(i);

			printf(":: %s: %s %s", (char*)pacman_dep_getinfo(miss, PM_DEP_TARGET),
				(int)pacman_dep_getinfo(miss, PM_DEP_TYPE) == PM_DEP_TYPE_DEPEND ? "requires" : "is required by",
				(char*)pacman_dep_getinfo(miss, PM_DEP_NAME));
			switch((int)pacman_dep_getinfo(miss, PM_DEP_MOD))
			{
				case PM_DEP_MOD_EQ: printf("=%s\n", (char*)pacman_dep_getinfo(miss, PM_DEP_VERSION)); break;
				case PM_DEP_MOD_GE: printf(">=%s\n", (char*)pacman_dep_getinfo(miss, PM_DEP_VERSION)); break;
				case PM_DEP_MOD_LE: printf("<=%s\n", (char*)pacman_dep_getinfo(miss, PM_DEP_VERSION)); break;
				default: printf("\n"); break;
			}
		}
		pacman_list_free(junk);
		pacman_trans_release();
		return(1);
	}
	PRINTF(" done.\n");

	mkiso(volume, countonly, stable, dryrun);
	pacman_trans_release();
	pacman_release();
	g_list_free(isopkgs);
	isopkgs=NULL;
	g_list_free(isogrps);
	isogrps=NULL;
	return(0);
}

int main(int argc, char **argv)
{
	char tmproot[] = "/tmp/mkiso_XXXXXX";
	char *xmlfile = strdup("volumes.xml");
	int i, countonly=0, stable=0, dryrun=0;
	char *ptr;
	int opt;
	int option_index;
	static struct option opts[] =
	{
		{"help",        no_argument,       0, 'h'},
		{"count",       no_argument,       0, 'c'},
		{"dry-run",     no_argument,       0, 'n'},
		{"stable",      no_argument,       0, 's'},
		{"file",        required_argument, 0, 'f'},
		{0, 0, 0, 0}
	};

	if(argc >= 2)
	{
		while((opt = getopt_long(argc, argv, "hcsf:n", opts, &option_index)))
		{
			if(opt < 0)
				break;
			switch(opt)
			{
				case 'h':
					printf("usage: %s [ options ]\n", argv[0]);
					printf("       -c | --count   count the possible number of images only\n");
					printf("       -f | --file    use some other source instead of volumes.xml\n");
					printf("       -h | --help    this help\n");
					printf("       -n | --dry-run do not generate an iso, just print a filelist\n");
					printf("       -s | --stable  indicate that the source repo is a -stable one\n");
					free(xmlfile);
					return(0);
				break;
				case 'c': countonly=1; break;
				case 'n': dryrun=1; break;
				case 's': stable=1; break;
				case 'f':
					  free(xmlfile);
					  xmlfile = strdup(optarg);
				break;
			}
		}
	}

	if(parseVolumes(xmlfile))
		return(1);
	mkdtemp(tmproot);
	ptr = g_strdup_printf("%s/tmp", tmproot);
	mkdir(ptr, 0700);
	free(ptr);
	ptr = g_strdup_printf("%s/var", tmproot);
	mkdir(ptr, 0700);
	free(ptr);
	ptr = g_strdup_printf("%s/var/log", tmproot);
	mkdir(ptr, 0700);
	free(ptr);

	for(i=0;i<g_list_length(volumes);i++)
		if(prepare(g_list_nth_data(volumes, i), tmproot, countonly, stable, dryrun))
			break;

	PRINTF("cleaning up...");
	rmrf(tmproot);
	PRINTF(" done.\n");
	free(fst_root);
	free(fst_ver);
	free(fst_codename);
	free(out_dir);
	free(lang);
	return(0);
}
