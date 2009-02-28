/*
 *      latexenvironments.c
 *
 *      Copyright 2009 Frank Lanitz <frank(at)frank(dot)uvena(dot)de>
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

#include "latexenvironments.h"

CategoryName glatex_environment_cat_names[] = {
    { ENVIRONMENT_CAT_DUMMY, N_("Environments"), TRUE},
    { 0, NULL, FALSE}
};


SubMenuTemplate glatex_environment_array[] = {
    {0, "document", "document"},
    {0, "frame", "frame"},
    {0, "itemize", "itemize"},
    {0, NULL, NULL},
};


void glatex_insert_environment(gchar *environment)
{
	GeanyDocument *doc = NULL;

	doc = document_get_current();

	if (doc != NULL && environment != NULL)
	{
       	gint pos = sci_get_current_position(doc->editor->sci);
       	gint len = strlen(environment);
       	gchar *tmp = NULL;

       	tmp = g_strconcat("\\begin{", environment, "}\n\n\\end(",
		    environment, "}\n", NULL);

       	sci_insert_text(doc->editor->sci, pos, tmp);
       	sci_set_current_position(doc->editor->sci, pos + len + 9, TRUE);

        g_free(tmp);
	}
}
