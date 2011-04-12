/*
 *      geany-ctags -- ctags/etags plugin for Geany
 *      Copyright 2011 Alexandre Erwin Ittner <alexandre@ittner.com.br>
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

#include <glib.h>
#include <glib/gprintf.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "geany.h"
#include "support.h"

#ifdef HAVE_LOCALE_H
# include <locale.h>
#endif

#include "ui_utils.h"
#include "document.h"
#include "editor.h"
#include "keybindings.h"
#include "plugindata.h"
#include "geanyfunctions.h"

#include "readtags.h"

GeanyPlugin		*geany_plugin;
GeanyData		*geany_data;
GeanyFunctions	*geany_functions;



PLUGIN_VERSION_CHECK(130);

PLUGIN_SET_INFO(_("ctags"), _("Browse symbols in ctags/etags files"),
	VERSION, "Alexandre Erwin Ittner <alexandre@ittner.com.br>");

/* Keybindings */
enum { KB_CTRL_BRACKET, KB_MAX };
PLUGIN_KEY_GROUP(ctags, KB_MAX)

static GtkWidget *menu_item_jump_to_tag = NULL;

/* A array of all file names understood as tag files */
static gchar **tag_file_names = NULL;

/*
 * Update the array 'tag_file_names' to reflect the given comma-separated
 * list of valid names for tag files. If NULL, just frees the list.
 */
static void update_tag_file_names(const gchar *names)
{
	if (tag_file_names != NULL)
		g_strfreev(tag_file_names);
	if (names != NULL)
		tag_file_names = g_strsplit_set(names, " \t\n\r;,", 0);
	else
		tag_file_names = NULL;
}


/*
 * Returns the full path to the nearest tagfile from the given document.
 * First this function tries to find a tag file in the same directory as the
 * document. If not found, it goes to the every parent directory until it
 * finds a file or reaches '/'. If no file is found, returns NULL.
 *
 * The return value must be freed by the caller.
 */

static gchar *find_tag_file_name(const GeanyDocument *doc)
{
	if (!DOC_VALID(doc))
		return NULL;	/* Error? */

	gchar *dir = NULL;
	if (doc->real_path != NULL)
		dir = g_path_get_dirname(doc->real_path);
	else	/* File not saved yet. Try current dir. */
		dir = g_get_current_dir();

	if (dir == NULL)
		return NULL;

	gint i = strlen(dir) - 1;
	while (i >= 0)
	{
		gint j;
		for (j = 0; tag_file_names[j]; j++)
		{
			if (*tag_file_names[j] == '\0')	/* empty string */
				continue;
			gchar *path = g_build_filename(dir, tag_file_names[j], NULL);
			if (g_file_test(path, G_FILE_TEST_EXISTS))
			{
				g_free(dir);
				return path;
			}
			g_free(path);
		}
		while (i >= 0 && !G_IS_DIR_SEPARATOR(dir[i]))
			i--;
		dir[i] = '\0';
	}
	g_free(dir);
	return NULL;
}


/*
 * Receive a ctags search pattern and returns a copy without the leading
 * and trailing symbols (ie, get "/^something$/" and returns a new string
 * with "something".  The caller must g_free the result.
 */

static gchar *g_strdup_pattern(const char *pattern)
{
	if (pattern == NULL)
		return NULL;
	if (*pattern == '/')
	{
		pattern++;
		if (*pattern == '^')
			pattern++;
	}
	if (*pattern == '\0')
		return NULL;
	gchar *new_pattern = g_strdup(pattern);
	gint len = strlen(new_pattern);
	if (len > 1 && new_pattern[len-2] == '/')
	{
		new_pattern[len-2] = '\0';
		len--;
	}
	if (len > 1 && new_pattern[len-2] == '$')
	{
		new_pattern[len-2] = '\0';
		len--;
	}
	return new_pattern;
}


static void print_entry_msgwin(tagEntry *entry)
{
	/* TODO: click in the message jump to file/line */
	gchar *p = g_strdup_pattern(entry->address.pattern);
	if (entry->address.lineNumber > 0)
		msgwin_status_add(_("'%s' found in %s, line %ld: %s"),
			entry->name, entry->file, entry->address.lineNumber, p);
	else
		msgwin_status_add(_("'%s' found in %s: %s"),
			entry->name, entry->file, p);
	if (p != NULL)
		g_free(p);
}


static void jump_to_declaration(GeanyDocument *doc, gchar *text)
{
	struct Sci_TextToFind ttf;
	ttf.chrg.cpMin = 0;
	ttf.chrg.cpMax = sci_get_length(doc->editor->sci);
	ttf.lpstrText = (gchar *) text;
	gint pos = sci_find_text(doc->editor->sci, SCFIND_MATCHCASE, &ttf);
	if (pos > -1)
	{
		/* TODO: mark position for back/forward */
		gint line = sci_get_line_from_position(doc->editor->sci, pos);
		sci_goto_line(doc->editor->sci, line, TRUE);
	}
	else
		msgwin_status_add(_("Declaration not found. Tag file outdated"));
}


static void find_tag(GeanyDocument *doc, char *tag)
{
	char *fname = find_tag_file_name(doc);
	if (fname == NULL)
	{
		msgwin_status_add(_("No tag file was found."));
		return;
	}

	tagFileInfo info;
	tagFile *tagfile = tagsOpen(fname, &info);
	if (tagfile == NULL)
	{
		msgwin_status_add(_("Failed to open tag file %s."), fname);
		g_free(fname);
		return;
	}

	msgwin_status_add(_("Using tagfile %s."), fname);

	tagEntry entry;
	gint total_tags = 0;
	gchar *first_file = NULL;
	gchar *first_pattern = NULL;
	gint result = TagFailure;

	do
	{
		if (total_tags == 0)
		{
			result = tagsFind(tagfile, &entry, tag, TAG_OBSERVECASE);
			if (result == TagSuccess)
			{
				first_file = g_strdup(entry.file);
				first_pattern = g_strdup_pattern(entry.address.pattern);
			}
		}
		else
			result = tagsFindNext(tagfile, &entry);
		if (result == TagSuccess)
		{
			print_entry_msgwin(&entry);
			total_tags++;
		}
	}
	while (result == TagSuccess);

	if (total_tags == 0)
		msgwin_status_add(_("Declaration '%s' not found"), tag);
	else if (total_tags == 1)	/* One tag found. Jump to that file */
	{
		/* Tagfiles may use absolute paths. */
		if (!g_path_is_absolute(first_file))
		{
			gchar *dir = g_path_get_dirname(fname);
			gchar *new_file = g_build_filename(dir, first_file, NULL);
			g_free(dir);
			g_free(first_file);
			first_file = new_file;
		}
		/* Open the document or show if it is already open */
		GeanyDocument *ndoc = document_open_file(first_file, FALSE, NULL, NULL);
		if (ndoc != NULL && ndoc->editor && ndoc->editor->sci)
			jump_to_declaration(ndoc, first_pattern);
		else
			msgwin_status_add(_("Can not open document '%s'"), first_file);
	}

	if (first_file != NULL)
		g_free(first_file);
	if (first_pattern != NULL)
		g_free(first_pattern);

	tagsClose(tagfile);
	g_free(fname);
}


static void find_tag_cb(GtkMenuItem *menuitem, gpointer gdata)
{
	GeanyDocument *doc = document_get_current();
	if (doc == NULL)
		return;
	gchar *text = sci_get_selection_contents(doc->editor->sci);
	gboolean ok = FALSE;
	if (text != NULL)
	{
		if (strlen(text) > 0)
		{
			/* TODO: Check for valid tags (ie, no spaces?) */
			ok = TRUE;
			find_tag(doc, text);
		}
		g_free(text);
	}
	if (ok == FALSE)
		msgwin_status_add(_("Select some text to search for declarations"));
}


/* For the keyboard shortcut */
static void find_tag_kb(guint key_id)
{
	find_tag_cb(NULL, NULL);
}


void plugin_init(GeanyData *data)
{
	main_locale_init(LOCALEDIR, GETTEXT_PACKAGE);
	menu_item_jump_to_tag = gtk_menu_item_new_with_mnemonic(_("Find ctag declaration"));
	gtk_widget_show(menu_item_jump_to_tag);
	gtk_container_add(GTK_CONTAINER(geany->main_widgets->tools_menu),
		menu_item_jump_to_tag);
	g_signal_connect(menu_item_jump_to_tag, "activate",
		G_CALLBACK(find_tag_cb), NULL);

	ui_add_document_sensitive(menu_item_jump_to_tag);

	/* Set keyboard shortcut. Defaults to ^], as in vim */
	keybindings_set_item(plugin_key_group, KB_CTRL_BRACKET, find_tag_kb,
		GDK_bracketright, GDK_CONTROL_MASK, "find_ctag_declaration",
		_("Find ctag declaration"), menu_item_jump_to_tag);

	/* TODO: make this configurable */
	update_tag_file_names("tags");
}


void plugin_cleanup(void)
{
	gtk_widget_destroy(menu_item_jump_to_tag);
	update_tag_file_names(NULL);
}


GtkWidget *plugin_configure(GtkDialog *dialog)
{
	return gtk_label_new(_("Work in progress. There is no config dialog yet."));
}
