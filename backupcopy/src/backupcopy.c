/*
 *      backupcopy.c
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
#include <errno.h>
#include <time.h>
#include <string.h>

#ifdef HAVE_LOCALE_H
# include <locale.h>
#endif

#include "geany.h"
#include "support.h"
#include "document.h"
#include "utils.h"

#include "plugindata.h"
#include "pluginmacros.h"

#include <glib/gstdio.h>

PluginFields	*plugin_fields;
GeanyData		*geany_data;
GeanyFunctions	*geany_functions;


PLUGIN_VERSION_CHECK(75)

PLUGIN_SET_INFO(_("Backup Copy"), _("Creates a backup of the current file when saving"),
	"0.2", "Enrico Tröger")


static gchar *config_file;
static gchar *backup_dir; /* path to an existing directory in locale encoding */
static gchar *time_fmt;
static gint dir_levels;


/* Ensures utf8_dir exists and is writable and
 * set backup_dir to the locale encoded form of utf8_dir */
static gboolean set_backup_dir(const gchar *utf8_dir)
{
	gchar *tmp;

	if (! NZV(utf8_dir))
		return FALSE;

	tmp = p_utils->get_locale_from_utf8(utf8_dir);

	if (! g_path_is_absolute(tmp) ||
		! g_file_test(tmp, G_FILE_TEST_EXISTS) ||
		! g_file_test(tmp, G_FILE_TEST_IS_DIR))
	{
		g_free(tmp);
		return FALSE;
	}
	/** TODO add utils_is_file_writeable() to the plugin API and make use of it **/

	setptr(backup_dir, tmp);

	return TRUE;
}


static gchar *skip_root(gchar *filename)
{
	/* first skip the root (e.g. c:\ on windows) */
	const gchar *dir = g_path_skip_root(filename);

	/* if this has failed, use the filename again */
	if (dir == NULL)
		dir = filename;
	/* check again for leading / or \ */
	while (*dir == G_DIR_SEPARATOR)
		dir++;

	return (gchar *) dir;
}


static gchar *create_dir_parts(const gchar *filename)
{
	gint cnt_dir_parts = 0;
	gchar *cp;
	gchar *dirname;
	gchar last_char = 0;
	gint error;
	gchar *result;
	gchar *target_dir;

	if (dir_levels == 0)
		return g_strdup("");

	dirname = g_path_get_dirname(filename);

	cp = dirname;
	/* walk to the end of the string */
	while (*cp != '\0')
		cp++;

	/* walk backwards to find directory parts */
	while (cp > dirname)
	{
		if (*cp == G_DIR_SEPARATOR && last_char != G_DIR_SEPARATOR)
			cnt_dir_parts++;

		if (cnt_dir_parts == dir_levels)
			break;

		last_char = *cp;
		cp--;
	}

	result = skip_root(cp); /* skip leading slash/backslash and c:\ */
	target_dir = g_build_filename(backup_dir, result, NULL);

	error = p_utils->mkdir(target_dir, TRUE);
	if (error != 0)
	{
		p_ui->set_statusbar(FALSE, _("Backup Copy: Directory could not be created (%s)."),
			g_strerror(error));

		result = g_strdup(""); /* return an empty string in case of an error */
	}
	else
		result = g_strdup(result);

	g_free(dirname);
	g_free(target_dir);

	return result;
}


static void on_document_save(GObject *obj, GeanyDocument *doc, gpointer user_data)
{
	FILE *src, *dst;
	gchar *locale_filename_src;
	gchar *locale_filename_dst;
	gchar *basename_src;
	gchar *dir_parts_src;
	gchar stamp[512];
	time_t t = time(NULL);
	struct tm *now = localtime(&t);

	locale_filename_src = p_utils->get_locale_from_utf8(doc->file_name);

	if ((src = g_fopen(locale_filename_src, "r")) == NULL)
	{
		/* it's unlikely that this happens */
		p_ui->set_statusbar(FALSE, _("Backup Copy: File could not be read (%s)."),
			g_strerror(errno));
		g_free(locale_filename_src);
		return;
	}

	strftime(stamp, sizeof(stamp), time_fmt, now);
	basename_src = g_path_get_basename(locale_filename_src);
	dir_parts_src = create_dir_parts(locale_filename_src);
	locale_filename_dst = g_strconcat(
		backup_dir, G_DIR_SEPARATOR_S,
		dir_parts_src, G_DIR_SEPARATOR_S,
		basename_src, ".", stamp, NULL);
	g_free(basename_src);
	g_free(dir_parts_src);

	if ((dst = g_fopen(locale_filename_dst, "wb")) == NULL)
	{
		p_ui->set_statusbar(FALSE, _("Backup Copy: File could not be saved (%s)."),
			g_strerror(errno));
		g_free(locale_filename_src);
		g_free(locale_filename_dst);
		fclose(src);
		return;
	}

	while (fgets(stamp, sizeof(stamp), src) != NULL)
	{
		fputs(stamp, dst);
	}

	fclose(src);
	fclose(dst);
	g_free(locale_filename_src);
	g_free(locale_filename_dst);
}


PluginCallback plugin_callbacks[] =
{
    { "document-save", (GCallback) &on_document_save, FALSE, NULL },
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


void plugin_init(GeanyData *data)
{
	GKeyFile *config = g_key_file_new();
	gchar *tmp;
	config_file = g_strconcat(geany->app->configdir, G_DIR_SEPARATOR_S, "plugins", G_DIR_SEPARATOR_S,
		"backupcopy", G_DIR_SEPARATOR_S, "backupcopy.conf", NULL);

	backup_dir = NULL;
	time_fmt = NULL;
	dir_levels = 0;

	g_key_file_load_from_file(config, config_file, G_KEY_FILE_NONE, NULL);
	dir_levels = p_utils->get_setting_integer(config, "backupcopy", "dir_levels", 0);
	time_fmt = p_utils->get_setting_string(config, "backupcopy", "time_fmt", "%Y-%m-%d-%H-%M-%S");
	tmp = p_utils->get_setting_string(config, "backupcopy", "backup_dir", g_get_tmp_dir());
	set_backup_dir(tmp);

	locale_init();

	g_key_file_free(config);
	g_free(tmp);
}


static void on_dir_button_clicked(GtkButton *button, gpointer item)
{
	/** TODO add win32_show_pref_file_dialog to the plugin API and use it **/
//~ #ifdef G_OS_WIN32
	//~ win32_show_pref_file_dialog(item);
//~ #else

	GtkWidget *dialog;
	gchar *text;

	/* initialize the dialog */
	dialog = gtk_file_chooser_dialog_new(_("Select Directory"), NULL,
					GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

	text = p_utils->get_locale_from_utf8(gtk_entry_get_text(GTK_ENTRY(item)));
	if (NZV(text))
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), text);

	/* run it */
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		gchar *utf8_filename, *tmp;

		tmp = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		utf8_filename = p_utils->get_utf8_from_locale(tmp);

		gtk_entry_set_text(GTK_ENTRY(item), utf8_filename);

		g_free(utf8_filename);
		g_free(tmp);
	}

	gtk_widget_destroy(dialog);
}


static void on_configure_response(GtkDialog *dialog, gint response, gpointer user_data)
{
	if (response == GTK_RESPONSE_OK || response == GTK_RESPONSE_APPLY)
	{
		GKeyFile *config = g_key_file_new();
		gchar *data;
		const gchar *text_dir, *text_time;
		gchar *config_dir = g_path_get_dirname(config_file);

		text_dir = gtk_entry_get_text(GTK_ENTRY(g_object_get_data(G_OBJECT(dialog), "entry_dir")));
		text_time = gtk_entry_get_text(GTK_ENTRY(g_object_get_data(G_OBJECT(dialog), "entry_time")));
		dir_levels = gtk_spin_button_get_value_as_int(
			GTK_SPIN_BUTTON(g_object_get_data(G_OBJECT(dialog), "spin_dir_levels")));

		if (NZV(text_dir) && set_backup_dir(text_dir))
		{
			g_key_file_load_from_file(config, config_file, G_KEY_FILE_NONE, NULL);
			g_key_file_set_integer(config, "backupcopy", "dir_levels", dir_levels);
			g_key_file_set_string(config, "backupcopy", "backup_dir", text_dir);
			g_key_file_set_string(config, "backupcopy", "time_fmt", text_time);
			setptr(time_fmt, g_strdup(text_time));

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
		}
		else
		{
			p_dialogs->show_msgbox(GTK_MESSAGE_ERROR,
					_("Backup directory does not exist or is not writable."));
			g_free(config_dir);
			g_key_file_free(config);
		}
		g_free(config_dir);
		g_key_file_free(config);
	}
}


GtkWidget *plugin_configure(GtkDialog *dialog)
{
	GtkWidget *label, *vbox, *hbox, *entry_dir, *entry_time, *button, *image, *spin_dir_levels;

	vbox = gtk_vbox_new(FALSE, 6);

	label = gtk_label_new(_("Directory to save backup files in:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	entry_dir = gtk_entry_new();
	if (NZV(backup_dir))
		gtk_entry_set_text(GTK_ENTRY(entry_dir), backup_dir);

	button = gtk_button_new();
	g_signal_connect((gpointer) button, "clicked", G_CALLBACK(on_dir_button_clicked), entry_dir);

	image = gtk_image_new_from_stock("gtk-open", GTK_ICON_SIZE_BUTTON);
	gtk_container_add(GTK_CONTAINER(button), image);

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start_defaults(GTK_BOX(hbox), entry_dir);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	label = gtk_label_new(_("Date/Time format for backup files (\"man strftime\" for details):"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 7);

	entry_time = gtk_entry_new();
	if (NZV(time_fmt))
		gtk_entry_set_text(GTK_ENTRY(entry_time), time_fmt);
	gtk_box_pack_start(GTK_BOX(vbox), entry_time, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 6);

	label = gtk_label_new(_("Directory levels to include in the backup destination:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	spin_dir_levels = gtk_spin_button_new_with_range(0, 20, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_dir_levels), dir_levels);
	gtk_box_pack_start(GTK_BOX(hbox), spin_dir_levels, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 7);

	g_object_set_data(G_OBJECT(dialog), "entry_dir", entry_dir);
	g_object_set_data(G_OBJECT(dialog), "entry_time", entry_time);
	g_object_set_data(G_OBJECT(dialog), "spin_dir_levels", spin_dir_levels);
	g_signal_connect(dialog, "response", G_CALLBACK(on_configure_response), NULL);

	gtk_widget_show_all(vbox);

	return vbox;
}


void plugin_cleanup(void)
{
	g_free(backup_dir);
	g_free(config_file);
}
