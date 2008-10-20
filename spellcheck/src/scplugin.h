/*
 *      scplugin.h - this file is part of Spellcheck, a Geany plugin
 *
 *      Copyright 2008 Enrico Tröger <enrico(dot)troeger(at)uvena(dot)de>
 *      Copyright 2008 Nick Treleaven <nick(dot)treleaven(at)btinternet(dot)com>
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
 *
 * $Id$
 */


#ifndef SC_PLUGIN_H
#define SC_PLUGIN_H 1



typedef struct
{
	gchar *config_file;
	gchar *default_language;
	gboolean use_msgwin;
	gboolean check_while_typing;
	gboolean show_toolbar_item;
	gulong signal_id;
	GPtrArray *dicts;
	GtkWidget *menu_item;
	GtkWidget *edit_menu;
	GtkWidget *edit_menu_sep;
	GtkWidget *edit_menu_sub;
	GtkToolItem *toolbar_button;
} SpellCheck;


#define MAX_MENU_SUGGESTIONS 10

typedef struct
{
	gint pos;
	GeanyDocument *doc;
	/* static array to keep suggestions for use as callback user data for the editing menu items */
	gchar *suggs[MAX_MENU_SUGGESTIONS];
	/* static storage for the misspelled word under the cursor when using the editing menu */
	gchar *word;
} SpellClickInfo;


extern SpellCheck *sc;
extern SpellClickInfo clickinfo;
extern GeanyPlugin		*geany_plugin;
extern GeanyData		*geany_data;
extern GeanyFunctions	*geany_functions;

#endif