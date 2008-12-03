 /*
 *      geanylatex.h
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

/* LaTeX plugin */
/* This plugin improves the work with LaTeX and Geany.*/

#ifndef GEANYLATEX_H
#define GEANYLATEX_H



#include "geany.h"
#include "support.h"
#include "plugindata.h"
#include "document.h"
#include "editor.h"
#include "filetypes.h"
#include "templates.h"
#include "utils.h"
#include "ui_utils.h"
#include "keybindings.h"
#include "prefs.h"
#include "pluginmacros.h"
#include "datatypes.h"
#include "letters.h"
#include "latexencodings.h"
#include "bibtex.h"

#ifdef HAVE_LOCALE_H
# include <locale.h>
#endif


typedef void (*SubMenuCallback) (G_GNUC_UNUSED GtkMenuItem * menuitem, G_GNUC_UNUSED gpointer gdata);

extern GeanyPlugin	*geany_plugin;
extern GeanyData	*geany_data;
extern GeanyFunctions	*geany_functions;


#define TEMPLATE_LATEX "\
\\documentclass[{CLASSOPTION}]{{DOCUMENTCLASS}}\n\
{ENCODING}\
{TITLE}\
{AUTHOR}\
{DATE}\
\\begin{document}\n\
\n\
\\end{document}\n"

#define TEMPLATE_LATEX_LETTER "\
\\documentclass[{CLASSOPTION}]{{DOCUMENTCLASS}}\n\
{ENCODING}\
\\stdaddress{}\n\
\\place{}\n\
{DATE}\
{TITLE}\
{AUTHOR}\
\\begin{document}\n\
\\begin{letter}{}\n\
\\opening{}\n\
\\end{document}\n"

#define create_sub_menu(base_menu, menu, item, title) \
		(menu) = gtk_menu_new(); \
		(item) = gtk_menu_item_new_with_mnemonic((title)); \
		gtk_menu_item_set_submenu(GTK_MENU_ITEM((item)), (menu)); \
		gtk_container_add(GTK_CONTAINER(base_menu), (item)); \
		gtk_widget_show((item));

#define MAX_MENU_ENTRIES 20

extern void insert_string(gchar *string);

#endif
