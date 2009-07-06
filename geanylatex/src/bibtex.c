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

#define glatex_set_status(entry_number, flag) \
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

void glatex_insert_bibtex_entry(G_GNUC_UNUSED GtkMenuItem * menuitem,
						 G_GNUC_UNUSED gpointer gdata)
{
	int doctype = GPOINTER_TO_INT(gdata);
	gboolean fields[GLATEX_BIBTEX_N_ENTRIES];
	gchar *output = g_strdup("@");
	int i;

	for (i = 0; i < GLATEX_BIBTEX_N_ENTRIES; i++)
	{
		glatex_set_status(i, FALSE);
	}

	switch(doctype) {
	case GLATEX_BIBTEX_ARTICLE:
		glatex_set_status(GLATEX_BIBTEX_AUTHOR, TRUE);
		glatex_set_status(GLATEX_BIBTEX_TITLE, TRUE);
		glatex_set_status(GLATEX_BIBTEX_JOURNAL, TRUE);
		glatex_set_status(GLATEX_BIBTEX_YEAR, TRUE);
		break;
	case GLATEX_BIBTEX_BOOK:
		glatex_set_status(GLATEX_BIBTEX_AUTHOR, TRUE);
		glatex_set_status(GLATEX_BIBTEX_EDITOR, TRUE);
		glatex_set_status(GLATEX_BIBTEX_TITLE, TRUE);
		glatex_set_status(GLATEX_BIBTEX_PUBLISHER, TRUE);
		glatex_set_status(GLATEX_BIBTEX_YEAR, TRUE);
		break;
	case GLATEX_BIBTEX_BOOKLET:
		glatex_set_status(GLATEX_BIBTEX_TITLE, TRUE);
		break;
	case GLATEX_BIBTEX_CONFERENCE:
	case GLATEX_BIBTEX_INCOLLECTION:
	case GLATEX_BIBTEX_INPROCEEDINGS:
		glatex_set_status(GLATEX_BIBTEX_AUTHOR, TRUE);
		glatex_set_status(GLATEX_BIBTEX_TITLE, TRUE);
		glatex_set_status(GLATEX_BIBTEX_BOOKTITLE, TRUE);
		glatex_set_status(GLATEX_BIBTEX_YEAR, TRUE);
		break;
	case GLATEX_BIBTEX_INBOOK:
		glatex_set_status(GLATEX_BIBTEX_AUTHOR, TRUE);
		glatex_set_status(GLATEX_BIBTEX_EDITOR, TRUE);
		glatex_set_status(GLATEX_BIBTEX_TITLE, TRUE);
		glatex_set_status(GLATEX_BIBTEX_CHAPTER, TRUE);
		glatex_set_status(GLATEX_BIBTEX_PAGES, TRUE);
		glatex_set_status(GLATEX_BIBTEX_PUBLISHER, TRUE);
		glatex_set_status(GLATEX_BIBTEX_YEAR, TRUE);
		break;
	case GLATEX_BIBTEX_MANUAL:
		glatex_set_status(GLATEX_BIBTEX_TITLE, TRUE);
		break;
	case GLATEX_BIBTEX_MASTERSTHESIS:
	case GLATEX_BIBTEX_PHDTHESIS:
		glatex_set_status(GLATEX_BIBTEX_AUTHOR, TRUE);
		glatex_set_status(GLATEX_BIBTEX_TITLE, TRUE);
		glatex_set_status(GLATEX_BIBTEX_SCHOOL, TRUE);
		glatex_set_status(GLATEX_BIBTEX_YEAR, TRUE);
		break;
	case GLATEX_BIBTEX_MISC:
		for (i = 0; i < GLATEX_BIBTEX_N_ENTRIES; i++)
		{
			glatex_set_status(i, TRUE);
		}
	case GLATEX_BIBTEX_TECHREPORT:
		glatex_set_status(GLATEX_BIBTEX_AUTHOR, TRUE);
		glatex_set_status(GLATEX_BIBTEX_TITLE, TRUE);
		glatex_set_status(GLATEX_BIBTEX_INSTITUTION, TRUE);
		glatex_set_status(GLATEX_BIBTEX_YEAR, TRUE);
		break;
	case GLATEX_BIBTEX_UNPUBLISHED:
		glatex_set_status(GLATEX_BIBTEX_AUTHOR, TRUE);
		glatex_set_status(GLATEX_BIBTEX_TITLE, TRUE);
		glatex_set_status(GLATEX_BIBTEX_NOTE, TRUE);
		break;
	case GLATEX_BIBTEX_PROCEEDINGS:
		glatex_set_status(GLATEX_BIBTEX_TITLE, TRUE);
		glatex_set_status(GLATEX_BIBTEX_YEAR, TRUE);
		break;
	default:
		for (i = 0; i < GLATEX_BIBTEX_N_ENTRIES; i++)
		{
			glatex_set_status(i, TRUE);
		}
	}

	output = g_strconcat(output, glatex_label_types[doctype], "{ \n",NULL);
	for (i = 0; i < GLATEX_BIBTEX_N_ENTRIES; i++)
	{
		if (fields[i] == TRUE)
		{
			output = g_strconcat(output, glatex_label_entry_keywords[i], " = {},\n", NULL);
		}
	}

	output = g_strconcat(output, "}\n", NULL);
	glatex_insert_string(output, FALSE);

	g_free(output);
}

