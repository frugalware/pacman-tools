#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <alpm.h>
#include <glib.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

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

GList *isopkgs=NULL;
GList *volumes=NULL;

char *fst_root=NULL;
char *fst_ver=NULL;

// for 650Mb = 1024*650 = 681574400 bytes with about 11Mb to spare
// FIXME: correct this after the kernel&initrd FIXME is done
#define CD_SIZE 665600
#define DVD_SIZE 4590208

// FIXME: cmdline switch, config file or something else for these
#define ARCH "i686"
#define MEDIA "dvd"
#define VOLUME 2

char *getarch()
{
	struct utsname name;

	uname (&name);
	return(strdup(name.machine));
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

int parseVolume(xmlDoc *doc, xmlNode *cur)
{
	xmlChar *key;
	cur = cur->xmlChildrenNode;
	volume_t *volume;

	if((volume = (volume_t *)malloc(sizeof(volume_t)))==NULL)
	{
		fprintf(stderr, "out of memory!\n");
		return(1);
	}
	memset(volume, 0, sizeof(volume_t));
	while (cur != NULL)
	{
		key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"arch")))
			volume->arch = strdup((char*)key);
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"media")))
			volume->media = strdup((char*)key);
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"serial")))
			volume->serial = atoi((char*)key);
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"size")))
			volume->size = atoi((char*)key);
		xmlFree(key);
		cur = cur->next;
	}
	if(!volume->arch)
	{
		volume->arch = getarch();
	}
	if(!volume->media)
	{
		fprintf(stderr, "media directive for a volume is required!\n");
		return(1);
	}
	if(!volume->serial && strcmp(volume->media, "net"))
	{
		fprintf(stderr, "media serial for a cd/dvd is required!\n");
		return(1);
	}
	if(!volume->size)
	{
		if(strcmp(volume->media, "cd"))
			volume->size=CD_SIZE;
		else if(strcmp(volume->media, "dvd"))
			volume->size=DVD_SIZE;
	}
	volumes = g_list_append(volumes, volume);
	return(0);
}

int parseVolumes(char *docname)
{
	xmlDocPtr doc;
	xmlNodePtr cur;
	xmlChar *key;

	doc = xmlParseFile(docname);

	if(doc == NULL)
	{
		fprintf(stderr, "document not parsed successfully\n");
		return(1);
	}

	cur = xmlDocGetRootElement(doc);

	if(cur == NULL)
	{
		fprintf(stderr, "empty document\n");
		xmlFreeDoc(doc);
		return(1);
	}

	if(xmlStrcmp(cur->name, (const xmlChar *)"volumes"))
	{
		fprintf(stderr, "document of the wrong type, root node != volumes");
		xmlFreeDoc(doc);
		return(1);
	}

	cur = cur->xmlChildrenNode;
	while(cur != NULL)
	{
		key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		if((!xmlStrcmp(cur->name, (const xmlChar *)"fst_root")))
			fst_root = strdup((char*)key);
		else if((!xmlStrcmp(cur->name, (const xmlChar *)"version")))
			fst_ver = strdup((char*)key);
		else if((!xmlStrcmp(cur->name, (const xmlChar *)"volume")))
			if(parseVolume(doc, cur))
				return(1);
		xmlFree(key);
		cur = cur->next;
	}
	if(!fst_root)
	{
		fprintf(stderr, "missing fst_root directive\n");
		return(1);
	}
	if(!fst_ver)
		fst_ver = get_timestamp();
	if(!volumes)
	{
		fprintf(stderr, "at least one volume is required\n");
		return(1);
	}

	xmlFreeDoc(doc);
	return(0);
}

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

char *detect_kernel()
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
		if(strstr(ent->d_name, "vmlinuz-") && strstr(ent->d_name, ARCH))
		{
			closedir(dir);
			return(strdup(ent->d_name+strlen("vmlinuz-")));
		}
	}
	closedir(dir);
	return(NULL);
}

int mkiso()
{
	PM_LIST *i, *sorted;
	int total=0, volume=1;
	char *fname=get_filename(fst_ver, ARCH, MEDIA, VOLUME);
	char *label=get_label(fst_ver, ARCH, MEDIA, VOLUME);
	char *flist, *cmdline, *ptr;
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
	ptr = detect_kernel();
	iso_add(fp, "boot/vmlinuz-%s-%s", ptr, ARCH);
	free(ptr);
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
		long int size = (long int)alpm_pkg_getinfo(pkg, PM_PKG_SIZE)/1024;
		char *ptr;
		// FIXME: 20971520 is ~20mb for the initrd&kernel
		if (total+size > CD_SIZE - 20480)
		{
			total=0;
			volume++;
		}
		total += size;
		if(volume==VOLUME)
		{
			if(detect_priority(pkg)>50)
				ptr = strdup("frugalware-%s/%s-%s-%s%s");
			else
				ptr = strdup("extra/frugalware-%s/%s-%s-%s%s");
			iso_add(fp, ptr,
			ARCH,
			(char*)alpm_pkg_getinfo(pkg, PM_PKG_NAME),
			(char*)alpm_pkg_getinfo(pkg, PM_PKG_VERSION),
			(char*)alpm_pkg_getinfo(pkg, PM_PKG_ARCH),
			PM_EXT_PKG);
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
	system(cmdline);

	free(fname);
	free(label);
	unlink(flist);
	free(flist);
	free(cmdline);
	chdir(cwd);
	return(0);
}

void cb_log(unsigned short level, char *msg)
{
	printf("%s\n", msg);
}

PM_DB *db_register(char *treename)
{
	PM_DB *db;
	char *ptr;

	if(!(db = alpm_db_register(treename)))
	{
		fprintf(stderr, "could not register '%s' database (%s)\n", treename, alpm_strerror(pm_errno));
		return(NULL);
	}
	if(!strcmp(treename, "frugalware-current"))
		ptr = g_strdup_printf("%s/frugalware-%s/%s.fdb", fst_root, ARCH, treename);
	else
		ptr = g_strdup_printf("%s/extra/frugalware-%s/%s.fdb", fst_root, ARCH, treename);
	if(alpm_db_update(db, ptr) == -1)
	{
		fprintf(stderr, "failed to update %s (%s)\n", treename, alpm_strerror(pm_errno));
		return(NULL);
	}
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

int main(int argc, char **argv)
{
	PM_DB *db_local, *db_fwcurr, *db_fwextra;
	PM_LIST *i, *junk;
	char tmproot[] = "/tmp/mkiso_XXXXXX";
	char *xmlfile = strdup("volumes.xml");
	int j;

	if(argc >= 2)
	{
		free(xmlfile);
		xmlfile = strdup(argv[1]);
	}

	if(parseVolumes(xmlfile))
		return(1);
	mkdtemp(tmproot);

	if(alpm_initialize(tmproot) == -1)
		fprintf(stderr, "failed to initilize alpm library (%s)\n", alpm_strerror(pm_errno));
	alpm_set_option(PM_OPT_LOGCB, (long)cb_log);
	alpm_set_option(PM_OPT_LOGMASK, (long)-1);
	if((db_local = alpm_db_register("local"))==NULL)
		fprintf(stderr, "could not register 'local' database (%s)\n", alpm_strerror(pm_errno));
	db_fwcurr = db_register("frugalware-current");
	db_fwextra = db_register("extra-current");
	if(alpm_trans_init(PM_TRANS_TYPE_SYNC, PM_TRANS_FLAG_NOCONFLICTS, NULL, NULL, NULL) == -1)
		fprintf(stderr, "failed to init transaction (%s)\n", alpm_strerror(pm_errno));

	if(strcmp(MEDIA, "net"))
	{
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
	for(i=alpm_db_getpkgcache(db_fwextra); i; i=alpm_list_next(i))
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
	}

	isopkgs = g_list_sort(isopkgs, sort_isopkgs);
	add_targets();

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

	mkiso();
	alpm_trans_release();
	alpm_release();
	rmrf(tmproot);
	free(fst_root);
	free(fst_ver);
	return(0);
}
