/*
 *      gui.h - this file is part of Spellcheck, a Geany plugin
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


#ifndef SC_GUI_H
#define SC_GUI_H 1



void gui_update_editor_menu_cb(GObject *obj, const gchar *word, gint pos,
						   GeanyDocument *doc, gpointer user_data);

gboolean gui_key_release_cb(GtkWidget *widget, GdkEventKey *ev, gpointer user_data);

void gui_kb_run_activate_cb(guint key_id);

void gui_kb_toggle_typing_activate_cb(guint key_id);

void gui_create_edit_menu(void);

GtkWidget *gui_create_menu(GtkWidget *sp_item);

void gui_update_editor_menu_cb(GObject *obj, const gchar *word, gint pos,
								  GeanyDocument *doc, gpointer user_data);

void gui_toolbar_update(void);

#endif
