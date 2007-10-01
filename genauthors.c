/*
 *  genauthors.c
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

void parseAuthor(xmlDoc *doc, xmlNode *cur)
{
	xmlChar *key;
	cur = cur->xmlChildrenNode;

	while (cur != NULL)
	{
		key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"name")))
			printf("%s ", key);
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"nick")))
			printf("(%s) ", key);
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"email")))
			printf("<%s>\n", key);
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"role")))
			printf("\t* %s\n", key);
		xmlFree(key);
		cur = cur->next;
	}
}

int parseAuthors(char *docname)
{
	xmlDocPtr doc;
	xmlNodePtr cur;

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

	if(xmlStrcmp(cur->name, (const xmlChar *)"authors"))
	{
		fprintf(stderr, "document of the wrong type, root node != authors");
		xmlFreeDoc(doc);
		return(1);
	}

	cur = cur->xmlChildrenNode;
	while(cur != NULL)
	{
		if((!xmlStrcmp(cur->name, (const xmlChar *)"author")))
			parseAuthor(doc, cur);
		cur = cur->next;
	}

	xmlFreeDoc(doc);
	return(0);
}

int main(int argc, char **argv)
{
	int ret=0;

	if(argc <= 1)
	{
		system("man genauthors");
		return(1);
	}

	ret += parseAuthors(argv[1]);
	return(ret);
}
