/*
 *  xml.c
 *
 *  Copyright (c) 2006, 2008 by Miklos Vajna <vmiklos@frugalware.org>
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
#include <sys/utsname.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <glib.h>
#include <pacman.h>

#include "mkiso.h"

extern GList *volumes;
extern char *fst_root;
extern char *fst_ver;
extern char *fst_codename;
extern char *out_dir;
extern char *lang;

static char *get_arch()
{
	struct utsname name;

	uname (&name);
	return(strdup(name.machine));
}

static char *get_version()
{
	FILE *fp;
	char *ptr, line[PATH_MAX+1], *cmdline = g_strdup_printf("git --git-dir %s/.git describe", fst_root);

	fp = popen(cmdline, "r");
	if(!fp)
		sprintf(line, "git");
	else
	{
		fgets(line, PATH_MAX, fp);
		fclose(fp);
		line[strlen(line)-1] = '\0';
		ptr = line;
		while((ptr = strchr(ptr, '-')))
		{
			*ptr = '.';
			ptr++;
		}
	}
	free(cmdline);

	return(strdup(line));
}

static int parseVolume(xmlDoc *doc, xmlNode *cur)
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
		volume->arch = get_arch();
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
		if(!strcmp(volume->media, "cd"))
			volume->size=CD_SIZE;
		else if(!strcmp(volume->media, "dvd"))
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
		else if((!xmlStrcmp(cur->name, (const xmlChar *)"codename")))
			fst_codename = strdup((char*)key);
		else if((!xmlStrcmp(cur->name, (const xmlChar *)"out_dir")))
			out_dir = strdup((char*)key);
		else if((!xmlStrcmp(cur->name, (const xmlChar *)"lang")))
			lang = strdup((char*)key);
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
		fst_ver = get_version();
	if(!fst_codename)
		fst_codename = strdup("-current");
	if(!volumes)
	{
		fprintf(stderr, "at least one volume is required\n");
		return(1);
	}

	xmlFreeDoc(doc);
	return(0);
}
