/*
 *  dbg.c
 *
 *  Copyright 2008 Yura Siamashka <yurand2@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtk/gtk.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "geany.h"		/* for the GeanyApp data type */
#include "keybindings.h"
#include "support.h"		/* for the _() translation macro (see also po/POTFILES.in) */
#include "document.h"
#include "editor.h"
#include "filetypes.h"
#include "ui_utils.h"
#include "project.h"
#include "utils.h"

#include "plugindata.h"		/* this defines the plugin API */
#include "pluginmacros.h"	/* some useful macros to avoid typing geany_data so often */

/* These items are set by Geany before init() is called. */
PluginFields *plugin_fields;
GeanyData *geany_data;
GeanyFunctions *geany_functions;

static GtkWidget *keyb1;


/* Check that Geany supports plugin API version 78 or later, and check
 * for binary compatibility. */
PLUGIN_VERSION_CHECK(78);
/* All plugins must set name, description, version and author. */
PLUGIN_SET_INFO(_("External Debugger"), _("Run external debugger."), VERSION,
		_("Yura Siamshka <yurand2@gmail.com>"));

/* Keybinding(s) */
enum
{
	KB_DBG_START,
	KB_COUNT
};

PLUGIN_KEY_GROUP(dbg_chars, KB_COUNT);

GtkWidget *create_Interactive(void);

static gchar *g_debuggers[] = { "kdbg", "ddd", "insight" };

static gchar *
locate_debugger()
{
	guint i;
	gchar *path;

	for (i = 0; i < sizeof(g_debuggers) / sizeof(g_debuggers[0]); i++)
	{
		path = g_find_program_in_path(g_debuggers[i]);
		if (path)
			return path;
	}
	return NULL;
}



static gchar *
locate_executeable()
{
	// Possible sources
	// 1) From project properties
	// 2) from autotools projects
	FILE *f;
	gchar *exec = NULL;
	gchar sign[4] = { 0 };

	gchar elf[] = { 0x7f, 0x45, 0x4c, 0x46 };
	gchar pe[] = { 0x4d, 0x5a };

	GeanyProject *project = geany->app->project;
	if (project && project->run_cmd)
	{
		exec = g_strdup(project->run_cmd);
	}
	else
	{
		// IMPLEMENT ME
	}

	// check ELF, PE signature
	f = fopen(exec, "rb");
	if (!f)
	{
		g_free(exec);
		return NULL;
	}
	fread(sign, 4, 1, f);
	fclose(f);

	if (memcmp(sign, elf, sizeof(elf)) == 0 || memcmp(sign, pe, sizeof(pe)) == 0)
	{
		return exec;
	}
	g_free(exec);
	return NULL;
}

static void
kb_dbg(G_GNUC_UNUSED guint key_id)
{
	gchar *debugger;
	gchar *executeable;
	gchar *command;

	debugger = locate_debugger();
	if (!debugger)
		return;

	executeable = locate_executeable();

	if (executeable)
	{
		command = g_strdup_printf("\"%s\" \"%s\"", debugger, executeable);
		g_spawn_command_line_sync(command, NULL, NULL, NULL, NULL);
		g_free(command);
		g_free(executeable);
	}
	g_free(debugger);
}

void
plugin_init(G_GNUC_UNUSED GeanyData * data)
{
	gchar *kb_label1 = _("Launch external debugger");

	keyb1 = gtk_menu_item_new();

	p_keybindings->set_item(plugin_key_group, KB_DBG_START, kb_dbg,
				0, 0, kb_label1, kb_label1, keyb1);
}

GtkWidget *
plugin_configure(G_GNUC_UNUSED GtkWidget * parent)
{
	return NULL;
}

void
plugin_cleanup(void)
{
	keyb1 = NULL;
}
