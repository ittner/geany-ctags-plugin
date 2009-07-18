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
	gint i;
	gint doctype = GPOINTER_TO_INT(gdata);
	GPtrArray *entry = glatex_bibtex_init_empty_entry();

	switch(doctype) {
	case GLATEX_BIBTEX_ARTICLE:
		g_ptr_array_index(entry, GLATEX_BIBTEX_AUTHOR) = g_strdup("\0");
		g_ptr_array_index(entry, GLATEX_BIBTEX_TITLE) = g_strdup("\0");
		g_ptr_array_index(entry, GLATEX_BIBTEX_JOURNAL) = g_strdup("\0");
		g_ptr_array_index(entry, GLATEX_BIBTEX_YEAR) = g_strdup("\0");
		break;
	case GLATEX_BIBTEX_BOOK:
		g_ptr_array_index(entry, GLATEX_BIBTEX_AUTHOR) = g_strdup("\0");
		g_ptr_array_index(entry, GLATEX_BIBTEX_EDITOR) = g_strdup("\0");
		g_ptr_array_index(entry, GLATEX_BIBTEX_TITLE) = g_strdup("\0");
		g_ptr_array_index(entry, GLATEX_BIBTEX_PUBLISHER) = g_strdup("\0");
		g_ptr_array_index(entry, GLATEX_BIBTEX_YEAR) = g_strdup("\0");
		break;
	case GLATEX_BIBTEX_BOOKLET:
		g_ptr_array_index(entry, GLATEX_BIBTEX_TITLE) = g_strdup("\0");
		break;
	case GLATEX_BIBTEX_CONFERENCE:
	case GLATEX_BIBTEX_INCOLLECTION:
	case GLATEX_BIBTEX_INPROCEEDINGS:
		g_ptr_array_index(entry, GLATEX_BIBTEX_AUTHOR) = g_strdup("\0");
		g_ptr_array_index(entry, GLATEX_BIBTEX_TITLE) = g_strdup("\0");
		g_ptr_array_index(entry, GLATEX_BIBTEX_BOOKTITLE) = g_strdup("\0");
		g_ptr_array_index(entry, GLATEX_BIBTEX_YEAR) = g_strdup("\0");
		break;
	case GLATEX_BIBTEX_INBOOK:
		g_ptr_array_index(entry, GLATEX_BIBTEX_AUTHOR) = g_strdup("\0");
		g_ptr_array_index(entry, GLATEX_BIBTEX_EDITOR) = g_strdup("\0");
		g_ptr_array_index(entry, GLATEX_BIBTEX_TITLE) = g_strdup("\0");
		g_ptr_array_index(entry, GLATEX_BIBTEX_CHAPTER) = g_strdup("\0");
		g_ptr_array_index(entry, GLATEX_BIBTEX_PAGES) = g_strdup("\0");
		g_ptr_array_index(entry, GLATEX_BIBTEX_PUBLISHER) = g_strdup("\0");
		g_ptr_array_index(entry, GLATEX_BIBTEX_YEAR) = g_strdup("\0");
		break;
	case GLATEX_BIBTEX_MANUAL:
		g_ptr_array_index(entry, GLATEX_BIBTEX_TITLE) = g_strdup("\0");
		break;
	case GLATEX_BIBTEX_MASTERSTHESIS:
	case GLATEX_BIBTEX_PHDTHESIS:
		g_ptr_array_index(entry, GLATEX_BIBTEX_AUTHOR) = g_strdup("\0");
		g_ptr_array_index(entry, GLATEX_BIBTEX_TITLE) = g_strdup("\0");
		g_ptr_array_index(entry, GLATEX_BIBTEX_SCHOOL) = g_strdup("\0");
		g_ptr_array_index(entry, GLATEX_BIBTEX_YEAR) = g_strdup("\0");
		break;
	case GLATEX_BIBTEX_MISC:
		for (i = 0; i < GLATEX_BIBTEX_N_ENTRIES; i++)
		{
			g_ptr_array_index(entry, i) = g_strdup("\0");
		}
	case GLATEX_BIBTEX_TECHREPORT:
		g_ptr_array_index(entry, GLATEX_BIBTEX_AUTHOR) = g_strdup("\0");
		g_ptr_array_index(entry, GLATEX_BIBTEX_TITLE) = g_strdup("\0");
		g_ptr_array_index(entry, GLATEX_BIBTEX_INSTITUTION) = g_strdup("\0");
		g_ptr_array_index(entry, GLATEX_BIBTEX_YEAR) = g_strdup("\0");
		break;
	case GLATEX_BIBTEX_UNPUBLISHED:
		g_ptr_array_index(entry, GLATEX_BIBTEX_AUTHOR) = g_strdup("\0");
		g_ptr_array_index(entry, GLATEX_BIBTEX_TITLE) = g_strdup("\0");
		g_ptr_array_index(entry, GLATEX_BIBTEX_NOTE) = g_strdup("\0");
		break;
	case GLATEX_BIBTEX_PROCEEDINGS:
		g_ptr_array_index(entry, GLATEX_BIBTEX_TITLE) = g_strdup("\0");
		g_ptr_array_index(entry, GLATEX_BIBTEX_YEAR) = g_strdup("\0");
		break;
	default:
		for (i = 0; i < GLATEX_BIBTEX_N_ENTRIES; i++)
		{
			g_ptr_array_index(entry, i) = g_strdup("\0");
		}
	}

	glatex_bibtex_write_entry(entry, doctype);

	g_ptr_array_free(entry, TRUE);
}

/* Creating and initialising a array for a bibTeX entry with NULL pointers*/
GPtrArray *glatex_bibtex_init_empty_entry()
{
	gint i;
	GPtrArray *entry = g_ptr_array_new();
	g_ptr_array_set_size(entry, GLATEX_BIBTEX_N_ENTRIES);
	return entry;
}


void glatex_bibtex_write_entry(GPtrArray *entry, gint doctype)
{
	gint i;
	gchar *output = NULL;

	/* Adding the doctype to entry */
	output = g_strconcat("@", glatex_label_types[doctype], "{ \n",NULL);

	/* Adding the keywords and values to entry */
	for (i = 0; i < GLATEX_BIBTEX_N_ENTRIES; i++)
	{
		/* Check whether a field has been marked for being used */
		if (g_ptr_array_index (entry, i) != NULL)
		{
			/* Check whether a field was only set for being used ... */
			if (utils_str_equal(g_ptr_array_index (entry, i), "\0"))
			{
				output = g_strconcat(output,
					glatex_label_entry_keywords[i], " = {},\n", NULL);
			}
			/* ... or has some real value inside. */
			else
			{
				output = g_strconcat(output,
					glatex_label_entry_keywords[i], " = {", g_ptr_array_index(entry, i),
					"},\n", NULL);
			}
		}
	}

	output = g_strconcat(output, "}\n", NULL);
	glatex_insert_string(output, FALSE);

}

