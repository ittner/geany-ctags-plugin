/*
 *      instantsave.c
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

#include <config.h>
#include <unistd.h>

#ifdef HAVE_LOCALE_H
# include <locale.h>
#endif

#include "geany.h"
#include "support.h"
#include "document.h"
#include "filetypes.h"
#include "utils.h"

#include "plugindata.h"
#include "pluginmacros.h"


PluginFields	*plugin_fields;
GeanyData		*geany_data;
GeanyFunctions	*geany_functions;


PLUGIN_VERSION_CHECK(71)

PLUGIN_SET_INFO(_("Instant Save"), _("Save instantly new files without an explicit Save As dialog."),
	"0.2", "Enrico Tröger")


static gchar *config_file;
static gchar *default_ft;


static void on_document_new(GObject *obj, GeanyDocument *doc, gpointer user_data)
{
    if (doc->file_name ==  NULL)
    {
		gchar *new_filename;
		gint fd;
		GeanyFiletype *ft = p_filetypes->lookup_by_name(default_ft);

		fd = g_file_open_tmp("gis_XXXXXX", &new_filename, NULL);
		if (fd != -1)
			close(fd); /* close the returned file descriptor as we only need the filename */

		if (ft != NULL)
			/* add the filetype's default extension to the new filename */
			setptr(new_filename, g_strconcat(new_filename, ".", ft->extension, NULL));

		doc->file_name = new_filename;

		if (FILETYPE_ID(doc->file_type) == GEANY_FILETYPES_NONE)
			p_document->set_filetype(doc, p_filetypes->lookup_by_name(default_ft));

		/* force saving the file to enable all the related actions(tab name, filetype, etc.) */
		p_document->save_file(doc, TRUE);
    }
}


PluginCallback plugin_callbacks[] =
{
    { "document-new", (GCallback) &on_document_new, FALSE, NULL },
    { NULL, NULL, FALSE, NULL }
};


static void locale_init(void)
{
#ifdef ENABLE_NLS
	gchar *locale_dir = NULL;

#ifdef HAVE_LOCALE_H
	setlocale(LC_ALL, "");
#endif

#ifdef G_OS_WIN32
	gchar *install_dir = g_win32_get_package_installation_directory("geany", NULL);
	/* e.g. C:\Program Files\geany\lib\locale */
	locale_dir = g_strconcat(install_dir, "\\lib\\locale", NULL);
	g_free(install_dir);
#else
	locale_dir = g_strdup(LOCALEDIR);
#endif

	bindtextdomain(GETTEXT_PACKAGE, locale_dir);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);
	g_free(locale_dir);
#endif
}


static void on_configure_response(GtkDialog *dialog, gint response, gpointer user_data)
{
	if (response == GTK_RESPONSE_OK || response == GTK_RESPONSE_APPLY)
	{
		GKeyFile *config = g_key_file_new();
		gchar *data;
		gchar *config_dir = g_path_get_dirname(config_file);

		g_free(default_ft);
		default_ft = gtk_combo_box_get_active_text(GTK_COMBO_BOX(
			g_object_get_data(G_OBJECT(dialog), "combo")));

		g_key_file_load_from_file(config, config_file, G_KEY_FILE_NONE, NULL);
		g_key_file_set_string(config, "instantsave", "default_ft", default_ft);

		if (! g_file_test(config_dir, G_FILE_TEST_IS_DIR) && p_utils->mkdir(config_dir, TRUE) != 0)
		{
			p_dialogs->show_msgbox(GTK_MESSAGE_ERROR,
				_("Plugin configuration directory could not be created."));
		}
		else
		{
			/* write config to file */
			data = g_key_file_to_data(config, NULL, NULL);
			p_utils->write_file(config_file, data);
			g_free(data);
		}

		g_free(config_dir);
		g_key_file_free(config);
	}
}


void plugin_init(GeanyData *data)
{
	GKeyFile *config = g_key_file_new();
	config_file = g_strconcat(app->configdir, G_DIR_SEPARATOR_S, "plugins", G_DIR_SEPARATOR_S,
		"instantsave", G_DIR_SEPARATOR_S, "instantsave.conf", NULL);

	g_key_file_load_from_file(config, config_file, G_KEY_FILE_NONE, NULL);
	default_ft = p_utils->get_setting_string(config, "instantsave", "default_ft",
		filetypes[GEANY_FILETYPES_NONE]->name);

	locale_init();

	g_key_file_free(config);
}


GtkWidget *plugin_configure(GtkDialog *dialog)
{
	GtkWidget *label, *vbox, *combo;
	guint i;

	vbox = gtk_vbox_new(FALSE, 6);

	label = gtk_label_new(_("Filetype to use for newly opened files:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	combo = gtk_combo_box_new_text();
	for (i = 0; i < filetypes_array->len; i++)
	{
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), filetypes[i]->name);

		if (p_utils->str_equal(filetypes[i]->name, default_ft))
			gtk_combo_box_set_active(GTK_COMBO_BOX(combo), i);
	}
	gtk_combo_box_set_wrap_width(GTK_COMBO_BOX(combo), 3);
	gtk_box_pack_start(GTK_BOX(vbox), combo, FALSE, FALSE, 0);

	gtk_widget_show_all(vbox);

	g_object_set_data(G_OBJECT(dialog), "combo", combo);
	g_signal_connect(dialog, "response", G_CALLBACK(on_configure_response), NULL);

	return vbox;
}


void plugin_cleanup(void)
{
	g_free(config_file);
	g_free(default_ft);
}
