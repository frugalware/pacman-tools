/*
 *  chkperm.c
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
#include <string.h>
#include <stdlib.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

int parseMaintainer(xmlDoc *doc, xmlNode *cur, char *login)
{
	xmlChar *key;
	char *ptr;
	cur = cur->xmlChildrenNode;
	int ret=0;

	while (cur != NULL)
	{
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"email")))
		{
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			ptr = (char*)key;
			while(*ptr && *ptr != '@')
				ptr++;
			*ptr='\0';
			if ((!xmlStrcmp(key, (const xmlChar *)login)))
				ret++;
			xmlFree(key);
		}
		cur = cur->next;
	}
	return(ret);
}

int parseTeam(xmlDoc *doc, xmlNode *cur, char *group, char *login)
{
	xmlChar *key;
	cur = cur->xmlChildrenNode;
	int ret=0, ok=0;

	while (cur != NULL)
	{
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"name")))
		{
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			xmlFree(key);
		}
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"maintainer")))
			ok += parseMaintainer(doc, cur, login);
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"group")))
		{
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if (ok && (!xmlStrcmp(key, (const xmlChar *)group)))
				ret++;
			xmlFree(key);
		}
		cur = cur->next;
	}
	return(ret);
}

int parseDoc(char *docname, char *group, char *login)
{
	xmlDocPtr doc;
	xmlNodePtr cur;
	int ret=0;

	doc = xmlParseFile(docname);
	
	if(doc == NULL)
	{
		fprintf(stderr, "document not parsed successfully\n");
		return(ret);
	}
	
	cur = xmlDocGetRootElement(doc);
	
	if(cur == NULL)
	{
		fprintf(stderr, "empty document\n");
		xmlFreeDoc(doc);
		return(ret);
	}
	
	if(xmlStrcmp(cur->name, (const xmlChar *)"teams"))
	{
		fprintf(stderr, "document of the wrong type, root node != teams");
		xmlFreeDoc(doc);
		return(ret);
	}
	
	cur = cur->xmlChildrenNode;
	while(cur != NULL)
	{
		if((!xmlStrcmp(cur->name, (const xmlChar *)"team")))
			ret += parseTeam(doc, cur, group, login);

		cur = cur->next;
	}

	xmlFreeDoc(doc);
	return(ret);
}

int main(int argc, char **argv)
{
	char *login, *ptr;

	if(argc <= 2)
	{
		printf("Usage: %s /path/to/teams.xml <group>\n", argv[0]);
		return(1);
	}

	login = strdup(getenv("HOME"));
	while((ptr = strchr(login, '/')) != NULL)
		login=ptr+1;

	if((ptr = strstr(argv[2], "-extra")) != NULL)
		*ptr = '\0';

	return(!parseDoc(argv[1], argv[2], login));
}
