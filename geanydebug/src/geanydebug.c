/*
 * geanydebug.c - Integrated debugger plugin for the Geany IDE
 * Copyright 2008 Jeff Pohlmeyer <yetanothergeek(at)gmail(dot)com>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */



#include "geany.h"
#include "support.h"
#include "prefs.h"
#include "document.h"
#include "editor.h"
#include "utils.h"
#include "ui_utils.h"
#include "keybindings.h"
#include "project.h"
#include "msgwindow.h"

#include "plugindata.h"
#include "pluginmacros.h"

#include "gdb-io.h"
#include "gdb-ui.h"



#define unix_name "debugger"
//#define VERSION "0.0.1"



PLUGIN_VERSION_CHECK(78)
PLUGIN_SET_INFO(_("Debugger"), _("Integrated debugging with GDB."), VERSION, _("Jeff Pohlmeyer"))
     static GeanyData *geany_data;
     GeanyFunctions *geany_functions;

     static GtkNotebook *msgbook;
     static GtkWidget *compwin;
     static GtkWidget *frame;
     static gchar *config_file;


     static void show_compwin()
{
	gint page = gtk_notebook_page_num(msgbook, compwin);
	gtk_notebook_set_current_page(msgbook, page);
}



static void
info_message_cb(const gchar * msg)
{
	show_compwin();
	p_msgwindow->compiler_add(COLOR_BLACK, msg);
}


static void
warn_message_cb(const gchar * msg)
{
	show_compwin();
	p_msgwindow->compiler_add(COLOR_RED, msg);
}


#define NOTEBOOK GTK_NOTEBOOK(geany->main_widgets->notebook)
//#define DOCS ((document*)(doc_array->data))

//static gint
//doc_idx_to_tab_idx(gint idx)
//{
	// FIXME
//	return 0;
//      return (
//              (idx>=0) && ((guint)idx<doc_array->len) && DOCS[idx].is_valid
//      ) ? gtk_notebook_page_num(NOTEBOOK, GTK_WIDGET(DOCS[idx].sci)):-1;
//}


static void
goto_file_line_cb(const gchar * filename, const gchar * line, const gchar * reason)
{
	gint pos;
	gint page;
	GeanyDocument *doc;

	gint line_num = gdbio_atoi((gchar *) line) - 1;
	if (reason)
	{
		p_msgwindow->compiler_add(COLOR_BLUE, reason);
	}
	doc = p_document->open_file(filename, FALSE, NULL, NULL);
	if (!(doc && doc->is_valid))
	{
		return;
	}
	page = gtk_notebook_page_num(NOTEBOOK, GTK_WIDGET(doc->editor->sci));
	gtk_notebook_set_current_page(NOTEBOOK, page);
	pos = p_sci->get_position_from_line(doc->editor->sci, line_num);
	p_sci->ensure_line_is_visible(doc->editor->sci, line_num);
	while (gtk_events_pending())
	{
		gtk_main_iteration();
	}
	p_sci->set_current_position(doc->editor->sci, pos, TRUE);
	gtk_widget_grab_focus(GTK_WIDGET(doc->editor->sci));
	gtk_window_present(GTK_WINDOW(geany->main_widgets->window));
}


/*
static gchar *
get_current_word()
{
	const gchar *word_chars = NULL;
	gint pos, linenum, bol, bow, eow;
	gchar *text = NULL;
	gchar *rv = NULL;
	document *doc = p_document->get_current();
	if (!(doc && doc->is_valid))
	{
		return NULL;
	}
	pos = p_sci->get_current_position(doc->sci);
	linenum = p_sci->get_line_from_position(doc->sci, pos);
	bol = p_sci->get_position_from_line(doc->sci, linenum);
	bow = pos - bol;
	eow = pos - bol;
	text = p_sci->get_line(doc->sci, linenum);
	word_chars = GEANY_WORDCHARS;
	while ((bow > 0) && (strchr(word_chars, text[bow - 1]) != NULL))
	{
		bow--;
	}
	while (text[eow] && (strchr(word_chars, text[eow]) != NULL))
	{
		eow++;
	}
	text[eow] = '\0';
	rv = g_strdup(text + bow);
	g_free(text);
	return rv;
}*/

static gboolean word_check_left(gchar c)
{
	if (g_ascii_isalnum(c) || c == '_' || c == '.')
		return TRUE;
	return FALSE;
}

static gboolean
word_check_right(gchar c)
{
	if (g_ascii_isalnum(c) || c == '_')
		return TRUE;
	return FALSE;
}

static gchar *
get_current_word()
{
	gchar *txt;
	GeanyDocument *doc;

	gint pos;
	gint cstart, cend;
	gchar c;
	gint text_len;

	doc = p_document->get_current();
	g_return_val_if_fail(doc != NULL && doc->file_name != NULL, NULL);

	text_len = p_sci->get_selected_text_length(doc->editor->sci);
	if (text_len > 1)
	{
		txt = g_malloc(text_len + 1);
		p_sci->get_selected_text(doc->editor->sci, txt);
		return txt;
	}

	pos = p_sci->get_current_position(doc->editor->sci);
	if (pos > 0)
		pos--;

	cstart = pos;
	c = p_sci->get_char_at(doc->editor->sci, cstart);

	if (!word_check_left(c))
		return NULL;

	while (word_check_left(c))
	{
		cstart--;
		if (cstart >= 0)
			c = p_sci->get_char_at(doc->editor->sci, cstart);
		else
			break;
	}
	cstart++;

	cend = pos;
	c = p_sci->get_char_at(doc->editor->sci, cend);
	while (word_check_right(c) && cend < p_sci->get_length(doc->editor->sci))
	{
		cend++;
		c = p_sci->get_char_at(doc->editor->sci, cend);
	}

	if (cstart == cend)
		return NULL;
	txt = g_malloc0(cend - cstart + 1);

	p_sci->get_text_range(doc->editor->sci, cstart, cend, txt);
	return txt;
}




static LocationInfo *
location_query_cb()
{
	GeanyDocument *doc = p_document->get_current();
	if (!(doc && doc->is_valid))
	{
		return NULL;
	}
	if (doc->file_name)
	{
		LocationInfo *abi;
		gint line;
		abi = g_new0(LocationInfo, 1);
		line = p_sci->get_current_line(doc->editor->sci);
		abi->filename = g_strdup(doc->file_name);
		if (line >= 0)
		{
			abi->line_num = g_strdup_printf("%d", line + 1);
		}
		abi->symbol = get_current_word();
		return abi;
	}
	return NULL;
}



static void
update_settings_cb()
{
	GKeyFile *kf = g_key_file_new();
	g_key_file_set_string(kf, unix_name, "mono_font", gdbui_setup.options.mono_font);
	g_key_file_set_string(kf, unix_name, "term_cmd", gdbui_setup.options.term_cmd);
	g_key_file_set_boolean(kf, unix_name, "show_tooltips", gdbui_setup.options.show_tooltips);
	g_key_file_set_boolean(kf, unix_name, "show_icons", gdbui_setup.options.show_icons);
	if (p_utils->mkdir(gdbio_setup.temp_dir, TRUE) != 0)
	{
		p_dialogs->show_msgbox(GTK_MESSAGE_ERROR,
				       _("Plugin configuration directory could not be created."));
	}
	else
	{
		gchar *data = g_key_file_to_data(kf, NULL, NULL);
		p_utils->write_file(config_file, data);
		g_free(data);
	}
	g_key_file_free(kf);
	gtk_widget_destroy(GTK_BIN(frame)->child);
	gdbui_create_widgets(frame);
	gtk_widget_show_all(frame);
}




#define CLEAR() if (err) { g_error_free(err); err=NULL; }

#define GET_KEY_STR(k) { \
  gchar *tmp=g_key_file_get_string(kf,unix_name,#k"",&err); \
  if (tmp) { \
    if (*tmp && !err) { \
      g_free(gdbui_setup.options.k); \
      gdbui_setup.options.k=tmp; \
    } else { g_free(tmp); } \
  } \
  CLEAR(); \
}


#define GET_KEY_BOOL(k) { \
  gboolean tmp=g_key_file_get_boolean(kf,unix_name,#k"",&err); \
  if (err) { CLEAR() } else { gdbui_setup.options.k=tmp; } \
}

void
plugin_init(GeanyData * data)
{
	GKeyFile *kf = g_key_file_new();
	GError *err = NULL;
	geany_data = data;
	gdbui_setup.main_window = geany->main_widgets->window;

	gdbio_setup.temp_dir = g_build_filename(geany->app->configdir, "plugins", unix_name, NULL);
	gdbio_setup.tty_helper = g_build_filename(gdbio_setup.temp_dir, "ttyhelper", NULL);
	config_file = g_build_filename(gdbio_setup.temp_dir, "debugger.cfg", NULL);
	gdbui_opts_init();
	if (g_key_file_load_from_file(kf, config_file, G_KEY_FILE_NONE, NULL))
	{
		GET_KEY_STR(mono_font);
		GET_KEY_STR(term_cmd);
		GET_KEY_BOOL(show_tooltips);
		GET_KEY_BOOL(show_icons);
	}
	g_key_file_free(kf);

	gdbui_setup.warn_func = warn_message_cb;
	gdbui_setup.info_func = info_message_cb;
	gdbui_setup.opts_func = update_settings_cb;
	gdbui_setup.location_query = location_query_cb;
	gdbui_setup.line_func = goto_file_line_cb;


	msgbook = GTK_NOTEBOOK(p_support->lookup_widget(geany->main_widgets->window, "notebook_info"));
	compwin = gtk_widget_get_parent(p_support->lookup_widget(geany->main_widgets->window, "treeview5"));
	frame = gtk_frame_new(NULL);
	gtk_notebook_append_page(GTK_NOTEBOOK(geany->main_widgets->sidebar_notebook), frame,
				 gtk_label_new("Debug"));
	gdbui_set_tips(GTK_TOOLTIPS(p_support->lookup_widget(geany->main_widgets->window, "tooltips")));
	gdbui_create_widgets(frame);
	gtk_widget_show_all(frame);
}


void
plugin_cleanup()
{
	gdbio_exit();
	update_settings_cb();

	g_free(gdbio_setup.temp_dir);
	g_free(gdbio_setup.tty_helper);

	gtk_widget_destroy(frame);
	gdbui_opts_done();
}


GtkWidget *
plugin_configure(G_GNUC_UNUSED GtkWidget * parent)
{
	gdbui_opts_dlg();
	return NULL;
}