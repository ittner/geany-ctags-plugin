/*
 *      bibtex.c
 *
 *      Copyright 2008-2009 Frank Lanitz <frank(at)frank(dot)uvena(dot)de>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

#include "bibtex.h"
#include "reftex.h"

#define set_status(entry_number, flag) \
	fields[entry_number] = flag;


#if 0
static int get_entry_pos(char *str)
{
	int i;

	if(str != NULL) {
		for (i = 0; i < N_ENTRIES; i++) {
			if (utils_str_casecmp(str, label_entry[i]) == 0)
				return i;
		}
	}
	return -1;
}
#endif

void insert_bibtex_entry(G_GNUC_UNUSED GtkMenuItem * menuitem,
						 G_GNUC_UNUSED gpointer gdata)
{
	int doctype = GPOINTER_TO_INT(gdata);
	gboolean fields[N_ENTRIES];
	gchar *output = g_strup("@");
	int i;

	for (i = 0; i < N_ENTRIES; i++)
	{
		set_status(i, FALSE);
	}

	switch(doctype) {
	case ARTICLE:
		set_status(AUTHOR, TRUE);
		set_status(TITLE, TRUE);
		set_status(JOURNAL, TRUE);
		set_status(YEAR, TRUE);
		break;
	case BOOK:
		set_status(AUTHOR, TRUE);
		set_status(EDITOR, TRUE);
		set_status(TITLE, TRUE);
		set_status(PUBLISHER, TRUE);
		set_status(YEAR, TRUE);
		break;
	case BOOKLET:
		set_status(TITLE, TRUE);
		break;
	case CONFERENCE:
	case INCOLLECTION:
	case INPROCEEDINGS:
		set_status(AUTHOR, TRUE);
		set_status(TITLE, TRUE);
		set_status(BOOKTITLE, TRUE);
		set_status(YEAR, TRUE);
		break;
	case INBOOK:
		set_status(AUTHOR, TRUE);
		set_status(EDITOR, TRUE);
		set_status(TITLE, TRUE);
		set_status(CHAPTER, TRUE);
		set_status(PAGES, TRUE);
		set_status(PUBLISHER, TRUE);
		set_status(YEAR, TRUE);
		break;
	case MANUAL:
		set_status(TITLE, TRUE);
		break;
	case MASTERSTHESIS:
	case PHDTHESIS:
		set_status(AUTHOR, TRUE);
		set_status(TITLE, TRUE);
		set_status(SCHOOL, TRUE);
		set_status(YEAR, TRUE);
		break;
	case MISC:
		for (i = 0; i < N_ENTRIES; i++)
		{
			set_status(i, TRUE);
		}
	case TECHREPORT:
		set_status(AUTHOR, TRUE);
		set_status(TITLE, TRUE);
		set_status(INSTITUTION, TRUE);
		set_status(YEAR, TRUE);
		break;
	case UNPUBLISHED:
		set_status(AUTHOR, TRUE);
		set_status(TITLE, TRUE);
		set_status(NOTE, TRUE);
		break;
	case PROCEEDINGS:
		set_status(TITLE, TRUE);
		set_status(YEAR, TRUE);
		break;
	default:
		for (i = 0; i < N_ENTRIES; i++)
		{
			set_status(i, TRUE);
		}
	}

	output = g_strconcat(output, label_types[doctype], "{ \n",NULL);
	for (i = 0; i < N_ENTRIES; i++)
	{
		if (fields[i] == TRUE)
		{
			output = g_strconcat(output, label_entry_keywords[i], "= {},\n", NULL);
		}
	}

	output = g_strconcat(output, "}\n", NULL);
	insert_string(output);

	g_free(output);
}
