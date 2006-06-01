#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <alpm.h>
#include <glib.h>

typedef struct __isopkg_t
{
	PM_PKG *pkg;
	int priority;
} isopkg_t;

GList *isopkgs=NULL;

// for 650Mb = 1024*1024*650 = 681574400 bytes with about 11Mb to spare
#define CD_SIZE 670000000

// FIXME: cmdline switch, config file or something else for these
#define ARCH "i686"
#define MEDIA "cd"
#define VOLUME 1
#define KERNEL "2.6.16-fw5"

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

char *get_timestamp()
{
	time_t t;
	struct tm *tm;
	char buf[9];

	t = time(NULL);
	tm = localtime(&t);

	sprintf(buf, "20%02d%02d%02d", tm->tm_year-100, tm->tm_mon+1, tm->tm_mday);
	return(strdup(buf));
}

char *get_filename(char *version, char *arch, char *media, int volume)
{
	if(volume)
		return(g_strdup_printf("frugalware-%s-%s-%s%d.iso", version, arch, media, volume));
	else
		return(g_strdup_printf("frugalware-%s-%s-%s.iso", version, arch, media));
}

char *get_label(char *version, char *arch, char *media, int volume)
{
	if(volume)
		return(g_strdup_printf("Frugalware %s-%s Install %s #%d", version, arch, media, volume));
	else
		return(g_strdup_printf("Frugalware %s-%s Install %s", version, arch, media));
}

int iso_add(FILE *fp, char *fmt, ...)
{
	va_list args;

	char str[PATH_MAX];

	va_start(args, fmt);
	vsnprintf(str, PATH_MAX, fmt, args);
	va_end(args);

	if(!fp)
		return(1);
	fprintf(fp, "%s=%s\n", str, str);
	return(0);
}

int mkiso()
{
	PM_LIST *i, *sorted;
	int total=0, volume=1;
	char *version=get_timestamp();
	char *fname=get_filename(version, ARCH, MEDIA, VOLUME);
	char *label=get_label(version, ARCH, MEDIA, VOLUME);
	char *flist;
	char *cmdline;
	char cwd[PATH_MAX] = "";
	FILE *fp;

	flist = strdup("/tmp/mkiso_XXXXXX");
	mkstemp(flist);

	if((fp=fopen(flist, "w"))==NULL)
		return(1);
	// initial filelist
	iso_add(fp, "AUTHORS");
	iso_add(fp, "ChangeLog.txt");
	iso_add(fp, "LICENSE");
	iso_add(fp, "docs");
	iso_add(fp, "boot/vmlinuz-%s-%s", KERNEL, ARCH);
	iso_add(fp, "boot/initrd-%s.img.gz", ARCH);
	// FIXME: generate the menu.lst automatically
	iso_add(fp, "boot/grub");
	if(VOLUME==1)
	{
		if(!strcmp(MEDIA, "cd") || !strcmp(MEDIA, "dvd"))
			iso_add(fp, "frugalware-%s/frugalware-current.fdb");
		if(!strcmp(MEDIA, "dvd"))
			iso_add(fp, "extra/frugalware-%s/extra-current.fdb");
	}

	sorted = alpm_trans_getinfo(PM_TRANS_PACKAGES);
	for(i = alpm_list_first(sorted); i; i = alpm_list_next(i))
	{
		PM_SYNCPKG *sync = alpm_list_getdata(i);
		PM_PKG *pkg = alpm_sync_getinfo(sync, PM_SYNC_PKG);
		long int size = (long int)alpm_pkg_getinfo(pkg, PM_PKG_SIZE);
		// FIXME: 20971520 is ~20mb for the initrd&kernel
		if (total+size > CD_SIZE - 20971520)
		{
			total=0;
			volume++;
		}
		total += size;
		if(volume==VOLUME)
			iso_add(fp, "frugalware-%s/%s-%s-%s%s",
			ARCH,
			(char*)alpm_pkg_getinfo(pkg, PM_PKG_NAME),
			(char*)alpm_pkg_getinfo(pkg, PM_PKG_VERSION),
			(char*)alpm_pkg_getinfo(pkg, PM_PKG_ARCH),
			PM_EXT_PKG);
	}
	fclose(fp);

	getcwd(cwd, PATH_MAX);
	chdir("/home/ftp/pub/frugalware/frugalware-current");

	cmdline = g_strdup_printf("mkisofs -o %s "
		"-R -J -V \"Frugalware Install\" "
		"-A \"%s\" "
		"-graft-points "
		"-path-list %s "
		"-hide-rr-moved "
		"-b boot/grub/stage2_eltorito "
		"-v -d -N -no-emul-boot -boot-load-size 4 -boot-info-table",
		fname, label, flist);
	system(cmdline);

	free(version);
	free(fname);
	free(label);
	unlink(flist);
	free(flist);
	free(cmdline);
	chdir(cwd);
	return(0);
}

int main()
{
	PM_DB *db_local, *db_fwcurr;
	PM_LIST *i, *junk;

	if(alpm_initialize("/home/vmiklos/darcs/pacman-tools/mkiso/t") == -1)
		fprintf(stderr, "failed to initilize alpm library (%s)\n", alpm_strerror(pm_errno));
	if((db_local = alpm_db_register("local"))==NULL)
		fprintf(stderr, "could not register 'local' database (%s)\n", alpm_strerror(pm_errno));
	if((db_fwcurr = alpm_db_register("frugalware-current"))==NULL)
		fprintf(stderr, "could not register 'frugalware-current' database (%s)\n", alpm_strerror(pm_errno));
	if(alpm_trans_init(PM_TRANS_TYPE_SYNC, PM_TRANS_FLAG_NOCONFLICTS, NULL, NULL, NULL) == -1)
		fprintf(stderr, "failed to init transaction (%s)\n", alpm_strerror(pm_errno));

	if(strcmp(MEDIA, "net"))
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

	printf("sorting dependencies...");
	fflush(stdout);
	if(alpm_trans_prepare(&junk) == -1)
	{
		fprintf(stderr, "failed to prepare transaction (%s)\n", alpm_strerror(pm_errno));
		for(i = alpm_list_first(junk); i; i = alpm_list_next(i))
		{
			PM_DEPMISS *miss = alpm_list_getdata(i);

			printf(":: %s: %s %s", (char*)alpm_dep_getinfo(miss, PM_DEP_TARGET),
				(int)alpm_dep_getinfo(miss, PM_DEP_TYPE) == PM_DEP_TYPE_DEPEND ? "requires" : "is required by",
				(char*)alpm_dep_getinfo(miss, PM_DEP_NAME));
			switch((int)alpm_dep_getinfo(miss, PM_DEP_MOD))
			{
				case PM_DEP_MOD_EQ: printf("=%s\n", (char*)alpm_dep_getinfo(miss, PM_DEP_VERSION)); break;
				case PM_DEP_MOD_GE: printf(">=%s\n", (char*)alpm_dep_getinfo(miss, PM_DEP_VERSION)); break;
				case PM_DEP_MOD_LE: printf("<=%s\n", (char*)alpm_dep_getinfo(miss, PM_DEP_VERSION)); break;
				default: printf("\n"); break;
			}
		}
		alpm_list_free(junk);
		alpm_trans_release();
		return(1);
	}
	printf(" done.\n");

	mkiso();
	alpm_trans_release();
	return(0);
}
