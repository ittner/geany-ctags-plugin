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

#if HAVE_LOCALE_H
# include <locale.h>
#endif

#include "geany.h"
#include "support.h"
#include "document.h"
#include "filetypes.h"

#include "plugindata.h"
#include "pluginmacros.h"


PluginFields	*plugin_fields;
GeanyData		*geany_data;


PLUGIN_VERSION_CHECK(51)

PLUGIN_INFO(_("Instant Save"), _("Save instantly new files without an explicit Save As dialog."),
	"0.1", "Enrico Tröger")


static gchar *config_file;
static gint default_ft_uid; /* this is the filetype uid, not a filetype id */


static void on_document_new(GObject *obj, gint idx, gpointer user_data)
{
    if (doc_list[idx].file_name ==  NULL)
    {
		gchar *new_filename;
		gint fd;

		fd = g_file_open_tmp("gis_XXXXXX", &new_filename, NULL);
		if (fd != -1)
			close(fd); /* close the returned file descriptor as we only need the filename */

		doc_list[idx].file_name = new_filename;

		if (FILETYPE_ID(doc_list[idx].file_type) == GEANY_FILETYPES_ALL)
			p_document->set_filetype(idx, p_filetypes->get_from_uid(default_ft_uid));

		/* force saving the file to enable all the related actions(tab name, filetype, etc.) */
		p_document->save_file(idx, TRUE);
    }
}


GeanyCallback geany_callbacks[] =
{
    { "document-new", (GCallback) &on_document_new, FALSE, NULL },
    { NULL, NULL, FALSE, NULL }
};


static void locale_init(void)
{
#ifdef ENABLE_NLS
	gchar *locale_dir = NULL;

#if HAVE_LOCALE_H
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


void init(GeanyData *data)
{
	GKeyFile *config = g_key_file_new();
	config_file = g_strconcat(app->configdir, G_DIR_SEPARATOR_S, "plugins", G_DIR_SEPARATOR_S,
		"instantsave", G_DIR_SEPARATOR_S, "instantsave.conf", NULL);

	g_key_file_load_from_file(config, config_file, G_KEY_FILE_NONE, NULL);
	default_ft_uid = p_utils->get_setting_integer(config, "instantsave", "default_ft",
		geany_data->filetypes[GEANY_FILETYPES_ALL]->uid);

	locale_init();

	g_key_file_free(config);
}


void configure(GtkWidget *parent)
{
	GtkWidget *dialog, *label, *vbox, *combo;
	gint i;
	filetype *ft;

	dialog = gtk_dialog_new_with_buttons(_("Instant Save"),
		GTK_WINDOW(parent), GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
	vbox = p_ui->dialog_vbox_new(GTK_DIALOG(dialog));
	gtk_widget_set_name(dialog, "GeanyDialog");
	gtk_box_set_spacing(GTK_BOX(vbox), 6);

	label = gtk_label_new(_("Filetype to use for newly opened files:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	combo = gtk_combo_box_new_text();
	for (i = 0; i < GEANY_MAX_FILE_TYPES; i++)
	{
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), geany_data->filetypes[i]->name);
	}
	ft = p_filetypes->get_from_uid(default_ft_uid);
	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), FILETYPE_ID(ft));
	gtk_combo_box_set_wrap_width(GTK_COMBO_BOX(combo), 3);
	gtk_box_pack_start(GTK_BOX(vbox), combo, FALSE, FALSE, 0);

	gtk_widget_show_all(vbox);

	/* run the dialog and check for the response code */
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		GKeyFile *config = g_key_file_new();
		gchar *data;
		gchar *config_dir = g_path_get_dirname(config_file);
		gint selected_ft_id;

		selected_ft_id = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
		default_ft_uid = geany_data->filetypes[selected_ft_id]->uid;

		g_key_file_load_from_file(config, config_file, G_KEY_FILE_NONE, NULL);
		g_key_file_set_integer(config, "instantsave", "default_ft", default_ft_uid);

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
	gtk_widget_destroy(dialog);
}


void cleanup(void)
{
	g_free(config_file);
}
