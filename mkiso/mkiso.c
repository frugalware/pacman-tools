#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alpm.h>
#include <glib.h>

typedef struct __isopkg_t
{
	PM_PKG *pkg;
	int priority;
} isopkg_t;

GList *isopkgs=NULL;

int strrcmp(const char *haystack, const char *needle)
{
	if(strlen(haystack) < strlen(needle))
		return(1);
	return(strcmp(haystack + strlen(haystack) - strlen(needle), needle));
}

int detect_priority(PM_PKG *pkg)
{
	PM_LIST *i = alpm_pkg_getinfo(pkg, PM_PKG_GROUPS);
	char *grp = alpm_list_getdata(i);

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
			return(40);
		else
			return(20);
	}
	fprintf(stderr, "possible invalid group '%s' for package '%s'\n", grp, (char*)alpm_pkg_getinfo(pkg, PM_PKG_NAME));
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
		char *pkgname = alpm_pkg_getinfo(isopkg->pkg, PM_PKG_NAME);
		if(alpm_trans_addtarget(pkgname))
		{
			fprintf(stderr, "failed to add target '%s' (%s)\n", pkgname, alpm_strerror(pm_errno));
			return(1);
		}
	}
	return(0);
}

int main()
{
	PM_DB *db_local, *db_fwcurr;
	PM_LIST *sorted, *i, *junk;

	if(alpm_initialize("/home/vmiklos/darcs/pacman-tools/mkiso/t") == -1)
		fprintf(stderr, "failed to initilize alpm library (%s)\n", alpm_strerror(pm_errno));
	if((db_local = alpm_db_register("local"))==NULL)
		fprintf(stderr, "could not register 'local' database (%s)\n", alpm_strerror(pm_errno));
	if((db_fwcurr = alpm_db_register("frugalware-current"))==NULL)
		fprintf(stderr, "could not register 'frugalware-current' database (%s)\n", alpm_strerror(pm_errno));
	if(alpm_trans_init(PM_TRANS_TYPE_SYNC, PM_TRANS_FLAG_NOCONFLICTS, NULL, NULL, NULL) == -1)
		fprintf(stderr, "failed to init transaction (%s)\n", alpm_strerror(pm_errno));

	for(i=alpm_db_getpkgcache(db_fwcurr); i; i=alpm_list_next(i))
	{
		PM_PKG *pkg=alpm_list_getdata(i);
		isopkg_t *isopkg;

		if((isopkg = (isopkg_t *)malloc(sizeof(isopkg_t)))==NULL)
		{
			fprintf(stderr, "out of memory!\n");
			return(1);
		}
		isopkg->pkg = pkg;
		isopkg->priority = detect_priority(pkg);
		isopkgs = g_list_append(isopkgs, isopkg);
	}

	isopkgs = g_list_sort(isopkgs, sort_isopkgs);
	add_targets();

	if(alpm_trans_prepare(&junk) == -1)
		fprintf(stderr, "failed to prepare transaction (%s)\n", alpm_strerror(pm_errno));

	sorted = alpm_trans_getinfo(PM_TRANS_PACKAGES);
	for(i = alpm_list_first(sorted); i; i = alpm_list_next(i))
	{
		PM_SYNCPKG *sync = alpm_list_getdata(i);
		PM_PKG *pkg = alpm_sync_getinfo(sync, PM_SYNC_PKG);
		printf("%s-%s-%s%s %ld\n",
			(char*)alpm_pkg_getinfo(pkg, PM_PKG_NAME),
			(char*)alpm_pkg_getinfo(pkg, PM_PKG_VERSION),
			(char*)alpm_pkg_getinfo(pkg, PM_PKG_ARCH),
			PM_EXT_PKG,
			(long int)alpm_pkg_getinfo(pkg, PM_PKG_SIZE));
	}
	alpm_trans_release();
	return(0);
}
