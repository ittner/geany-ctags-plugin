/*
 *      bibtex.c
 *
 *      Copyright 2008 Frank Lanitz <frank(at)frank(dot)uvena(dot)de>
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

#define set_status(entry_number, flag) \
	fields[entry_number] = flag;


gchar *label_types[] = {
	N_("Article"),
	N_("Book"),
	N_("Booklet"),
	N_("Conference"),
	N_("Inbook"),
	N_("Incollection"),
	N_("Inproceedings"),
	N_("Manual"),
	N_("Mastersthesis"),
	N_("Misc"),
	N_("PhdThesis"),
	N_("Proceedings"),
	N_("Techreport"),
	N_("Unpublished")};


const gchar *label_entry[] = {
	N_("Address"),
	N_("Annote"),
	N_("Author"),
	N_("Booktitle"),
	N_("Chapter"),
	N_("Crossref"),
	N_("Edition"),
	N_("Editor"),
	N_("E-print"),
	N_("HowPublished"),
	N_("Institution"),
	N_("Journal"),
	N_("Key"),
	N_("Month"),
	N_("Note"),
	N_("Number"),
	N_("Organization"),
	N_("Pages"),
	N_("Publisher"),
	N_("School"),
	N_("Series"),
	N_("Title"),
	N_("Type"),
	N_("URL"),
	N_("Volume"),
	N_("Year")};

const gchar *label_entry_keywords[] = {
	("Address"),
	("Annote"),
	("Author"),
	("Booktitle"),
	("Chapter"),
	("Crossref"),
	("Edition"),
	("Editor"),
	("E-print"),
	("HowPublished"),
	("Institution"),
	("Journal"),
	("Key"),
	("Month"),
	("Note"),
	("Number"),
	("Organization"),
	("Pages"),
	("Publisher"),
	("School"),
	("Series"),
	("Title"),
	("Type"),
	("URL"),
	("Volume"),
	("Year")};

const gchar *tooltips[] = {
	N_("Address of publisher"),
	N_("Annotation for annotated bibliography styles"),
	N_("Name(s) of the author(s), separated by 'and' if more than one"),
	N_("Title of the book, if only part of it is being cited"),
	N_("Chapter number"),
	N_("Citation key of the cross-referenced entry"),
	N_("Edition of the book (such as \"first\" or \"second\")"),
	N_("Name(s) of the editor(s), separated by 'and' if more than one"),
	N_("Specification of electronic publication"),
	N_("Publishing method if the method is nonstandard"),
	N_("Institution that was involved in the publishing"),
	N_("Journal or magazine in which the work was published"),
	N_("Hidden field used for specifying or overriding the alphabetical order of entries"),
	N_("Month of publication or creation if unpublished"),
	N_("Miscellaneous extra information"),
	N_("Number of journal, magazine, or tech-report"),
	N_("Sponsor of the conference"),
	N_("Page numbers separated by commas or double-hyphens"),
	N_("Name of publisher"),
	N_("School where thesis was written"),
	N_("Series of books in which the book was published"),
	N_("Title of the work"),
	N_("Type of technical report"),
	N_("Internet address"),
	N_("Number of the volume"),
	N_("Year of publication or creation if unpublished")};


static int get_entry_pos(char *str)
{
	int i;
	if(str != NULL) {
		for (i = 0; i < N_ENTRIES; i++) {
			if (p_utils->str_casecmp(str, label_entry[i]) == 0)
				return i;
		}
	}
	return -1;
}


void insert_bibtex_entry(G_GNUC_UNUSED GtkMenuItem * menuitem, G_GNUC_UNUSED gpointer gdata)
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
	case PHDTHESIS:
		set_status(TITLE, TRUE);
		set_status(YEAR, TRUE);
		break;
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
