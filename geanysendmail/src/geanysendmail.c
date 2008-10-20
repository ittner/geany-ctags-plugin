/*
 *      geanysendmail.c
 *
 *      Copyright 2007, 2008 Frank Lanitz <frank(at)frank(dot)uvena(dot)de>
 *      Copyright 2007 Enrico Tröger <enrico(dot)troeger(at)uvena(dot)de>
 *      Copyright 2007, 2008 Nick Treleaven <nick(dot)treleaven(at)btinternet(dot)com>
 *      Copyright 2008 Timothy Boronczyk <tboronczyk(at)gmail(dot)com>
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
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

/* A little plugin to send a document as attachment using the preferred mail client */

#include "geany.h"
#include "ui_utils.h"
#include "support.h"
#include "plugindata.h"
#include "document.h"
#include "filetypes.h"
#include "utils.h"
#include "keybindings.h"
#include "icon.h"
#include "pluginmacros.h"

#ifdef HAVE_LOCALE_H
# include <locale.h>
#endif

GeanyPlugin		*geany_plugin;
GeanyData		*geany_data;
GeanyFunctions	*geany_functions;

PLUGIN_VERSION_CHECK(100)

PLUGIN_SET_INFO(_("GeanySendMail"), _("A little plugin to send the current \
file as attachment by user's favorite mailer"), "0.4", "Frank Lanitz <frank@frank.uvena.de>")

/* Keybinding(s) */
enum
{
	SENDMAIL_KB,
	COUNT_KB
};

PLUGIN_KEY_GROUP(sendmail, COUNT_KB)

static gchar *config_file = NULL;
static gchar *mailer = NULL;
static gchar *address = NULL;
gboolean icon_in_toolbar = FALSE;
gboolean use_address_dialog = FALSE;
/* Needed global to remove from toolbar again */
GtkWidget *mailbutton = NULL;
static GtkWidget *main_menu_item = NULL;


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


/* Callback for sending file as attachment */
static void
send_as_attachment(G_GNUC_UNUSED GtkMenuItem *menuitem, G_GNUC_UNUSED gpointer gdata)
{
	GeanyDocument *doc;
	gchar	*locale_filename = NULL;
	gchar	*command = NULL;
	GError	*error = NULL;
	GString	*cmd_str = NULL;
	GtkWidget	*dialog = NULL;
	GtkWidget 	*label = NULL;
	GtkWidget 	*entry = NULL;
	GtkWidget	*vbox = NULL;
	GKeyFile 	*config = g_key_file_new();
	gchar 		*config_dir = g_path_get_dirname(config_file);
	gchar 		*data;


	doc = p_document->get_current();


	if (doc->file_name == NULL)
	{
		p_dialogs->show_save_as();
	}
	else
	{
		p_document->save_file(doc, FALSE);
	}

    if (doc->file_name != NULL)
	{
		if (mailer)
		{
			locale_filename = p_utils->get_locale_from_utf8(doc->file_name);
			cmd_str = g_string_new(mailer);

			if ((use_address_dialog == TRUE) && (g_strrstr(mailer, "%r") != NULL))
			{
				gint tmp;

 				dialog = gtk_dialog_new_with_buttons(_("Recipient's Address"),
 					GTK_WINDOW(geany->main_widgets->window), GTK_DIALOG_DESTROY_WITH_PARENT,
 					GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
 				vbox = p_ui->dialog_vbox_new(GTK_DIALOG(dialog));
 				gtk_widget_set_name(dialog, "GeanyDialog");
 				gtk_box_set_spacing(GTK_BOX(vbox), 10);

 				label = gtk_label_new(_("Enter the recipient's e-mail address:"));
 				gtk_widget_show(label);
 				gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
 				entry = gtk_entry_new();
 				gtk_widget_show(entry);
 				if (address != NULL)
 					gtk_entry_set_text(GTK_ENTRY(entry), address);

 				gtk_container_add(GTK_CONTAINER(vbox), label);
 				gtk_container_add(GTK_CONTAINER(vbox), entry);
 				gtk_widget_show(vbox);

 				tmp = gtk_dialog_run(GTK_DIALOG(dialog));

 				if (tmp == GTK_RESPONSE_ACCEPT)
 				{
					g_key_file_load_from_file(config, config_file, G_KEY_FILE_NONE, NULL);

					g_free(address);
 					address = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));

 					g_key_file_set_string(config, "tools", "address", address);
 				}

				if (! g_file_test(config_dir, G_FILE_TEST_IS_DIR) && p_utils->mkdir(config_dir, TRUE) != 0)
 				{
 					p_dialogs->show_msgbox(GTK_MESSAGE_ERROR,
 						_("Plugin configuration directory could not be created."));
 				}
 				else
 				{
 					// write config to file
 					data = g_key_file_to_data(config, NULL, NULL);
 					p_utils->write_file(config_file, data);
					g_free(data);
					g_key_file_free(config);
 					g_free(config_dir);
 				}
 			}

			if (! p_utils->string_replace_all(cmd_str, "%f", locale_filename))
				p_ui->set_statusbar(FALSE, _("Filename placeholder not found. The executed command might have failed."));

			if (use_address_dialog == TRUE && address != NULL)
			{
				if (! p_utils->string_replace_all(cmd_str, "%r", address))
 					p_ui->set_statusbar(FALSE, _("Recipient address placeholder not found. The executed command might have failed."));
			}
			else
				/* Removes %r if option was not activ but was included into command */
				p_utils->string_replace_all(cmd_str, "%r", NULL);

			p_utils->string_replace_all(cmd_str, "%b", g_path_get_basename(locale_filename));

			command = g_string_free(cmd_str, FALSE);
			g_spawn_command_line_async(command, &error);
			if (error != NULL)
			{
				p_ui->set_statusbar(FALSE, _("Could not execute mailer. Please check your configuration."));
				g_error_free(error);
			}

			g_free(locale_filename);
			g_free(command);

			if (dialog != NULL)
				gtk_widget_destroy(dialog);
		}
		else
		{
			p_ui->set_statusbar(FALSE, _("Please define a mail client first."));
		}
	}
	else
	{
		p_ui->set_statusbar(FALSE, _("File has to be saved before sending."));
	}
}

static void key_send_as_attachment(G_GNUC_UNUSED guint key_id)
{
	send_as_attachment(NULL, NULL);
}

#if GTK_CHECK_VERSION(2, 12, 0)
#define ICON_LOOKUP_MODE GTK_ICON_LOOKUP_GENERIC_FALLBACK
#else
#define ICON_LOOKUP_MODE GTK_ICON_LOOKUP_USE_BUILTIN
#endif

void show_icon()
{
	GdkPixbuf *mailbutton_pb = NULL;
	GtkWidget *icon = NULL;
	GtkIconSize size = geany_data->toolbar_prefs->icon_size;

	mailbutton_pb = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
					"mail-message-new", size, ICON_LOOKUP_MODE, NULL);

	/* Fallback if icon is not part of theme */
	if (mailbutton_pb == NULL)
	{
		mailbutton_pb = gdk_pixbuf_new_from_inline(-1, mail_pixbuf, FALSE, NULL);
	}

	icon = gtk_image_new_from_pixbuf(mailbutton_pb);
	g_object_unref(mailbutton_pb);

	mailbutton = (GtkWidget*) gtk_tool_button_new (icon, _("Mail"));
	p_plugin->add_toolbar_item(geany_plugin, GTK_TOOL_ITEM(mailbutton));
	p_ui->add_document_sensitive(mailbutton);
	g_signal_connect (G_OBJECT(mailbutton), "clicked", G_CALLBACK(send_as_attachment), NULL);
	gtk_widget_show_all (mailbutton);
}

void cleanup_icon()
{
	if (mailbutton != NULL)
	{
		gtk_container_remove(GTK_CONTAINER (geany->main_widgets->toolbar), mailbutton);
	}
}


static struct
{
	GtkWidget *entry;
	GtkWidget *checkbox_icon_to_toolbar;
	GtkWidget *checkbox_use_addressdialog;
}
pref_widgets;

static void
on_configure_response(GtkDialog *dialog, gint response, gpointer user_data)
{
	if (response == GTK_RESPONSE_OK || response == GTK_RESPONSE_APPLY)
	{
		GKeyFile 	*config = g_key_file_new();
		gchar 		*config_dir = g_path_get_dirname(config_file);

		g_free(mailer);
		mailer = g_strdup(gtk_entry_get_text(GTK_ENTRY(pref_widgets.entry)));

		if (icon_in_toolbar == FALSE && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pref_widgets.checkbox_icon_to_toolbar)) == TRUE)
		{
			icon_in_toolbar = TRUE;
			show_icon();
		}
		else if (icon_in_toolbar == TRUE && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pref_widgets.checkbox_icon_to_toolbar)) == FALSE)
		{
			cleanup_icon();
			icon_in_toolbar = FALSE;
		}
		else
		{
			icon_in_toolbar = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pref_widgets.checkbox_icon_to_toolbar));
		}

		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pref_widgets.checkbox_use_addressdialog)) == TRUE)
			use_address_dialog = TRUE;
		else
			use_address_dialog = FALSE;

		g_key_file_load_from_file(config, config_file, G_KEY_FILE_NONE, NULL);
		g_key_file_set_string(config, "tools", "mailer", mailer);
		g_key_file_set_boolean(config, "tools", "address_usage", use_address_dialog);
		g_key_file_set_boolean(config, "icon", "show_icon", icon_in_toolbar);

		if (! g_file_test(config_dir, G_FILE_TEST_IS_DIR) && p_utils->mkdir(config_dir, TRUE) != 0)
		{
			p_dialogs->show_msgbox(GTK_MESSAGE_ERROR,
				_("Plugin configuration directory could not be created."));
		}
		else
		{
			// write config to file
			gchar *data = g_key_file_to_data(config, NULL, NULL);
			p_utils->write_file(config_file, data);
			g_free(data);
		}
		g_key_file_free(config);
		g_free(config_dir);
	}
}

GtkWidget *plugin_configure(GtkDialog *dialog)
{
	GtkWidget	*label1, *label2, *vbox;
	GtkTooltips *tooltip = NULL;

	tooltip = gtk_tooltips_new();

	vbox = gtk_vbox_new(FALSE, 6);

	/* add a label and a text entry to the dialog */
	label1 = gtk_label_new(_("Path and options for the mail client:"));
	gtk_widget_show(label1);
	gtk_misc_set_alignment(GTK_MISC(label1), 0, 0.5);
	pref_widgets.entry = gtk_entry_new();
	gtk_widget_show(pref_widgets.entry);
	if (mailer != NULL)
		gtk_entry_set_text(GTK_ENTRY(pref_widgets.entry), mailer);

	label2 = gtk_label_new(_("Note: \n\t\%f will be replaced by your file."\
		"\n\t\%r will be replaced by recipient's email address."\
		"\n\t\%b will be replaced by basename of a file"\
		"\n\tExamples:"\
		"\n\tsylpheed --attach \"\%f\" --compose \"\%r\""\
		"\n\tmutt -s \"Sending \'\%b\'\" -a \"\%f\" \"\%r\""));
	gtk_label_set_selectable(GTK_LABEL(label2), TRUE);
	gtk_widget_show(label2);
	gtk_misc_set_alignment(GTK_MISC(label2), 0, 0.5);

	pref_widgets.checkbox_icon_to_toolbar = gtk_check_button_new_with_label(_("Showing icon in toolbar"));
	gtk_tooltips_set_tip(tooltip, pref_widgets.checkbox_icon_to_toolbar,
			     _
			     ("Shows a icon in the toolbar to send file more easy."),
			     NULL);
	gtk_button_set_focus_on_click(GTK_BUTTON(pref_widgets.checkbox_icon_to_toolbar), FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pref_widgets.checkbox_icon_to_toolbar), icon_in_toolbar);
	gtk_widget_show(pref_widgets.checkbox_icon_to_toolbar);

	pref_widgets.checkbox_use_addressdialog = gtk_check_button_new_with_label(_("Using dialog for entering email address of recipients"));
	/*gtk_tooltips_set_tip(tooltip, checkbox_use_addressdialog,
			     _
			     ("Shows a icon in the toolbar to send file more easy."),
			     NULL);*/
	gtk_button_set_focus_on_click(GTK_BUTTON(pref_widgets.checkbox_use_addressdialog), FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pref_widgets.checkbox_use_addressdialog), use_address_dialog);
	gtk_widget_show(pref_widgets.checkbox_use_addressdialog);

	gtk_container_add(GTK_CONTAINER(vbox), label1);
	gtk_container_add(GTK_CONTAINER(vbox), pref_widgets.entry);
	gtk_container_add(GTK_CONTAINER(vbox), label2);
	gtk_box_pack_start(GTK_BOX(vbox), pref_widgets.checkbox_icon_to_toolbar, TRUE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), pref_widgets.checkbox_use_addressdialog, TRUE, FALSE, 2);


	gtk_widget_show(vbox);

	g_signal_connect(dialog, "response", G_CALLBACK(on_configure_response), NULL);
	return vbox;
}

/* Called by Geany to initialize the plugin */
void plugin_init(GeanyData G_GNUC_UNUSED *data)
{
	GtkTooltips *tooltips = NULL;

	GKeyFile *config = g_key_file_new();

	gchar *kb_label = _("Send file by mail");

	GtkWidget *menu_mail = NULL;

	locale_init();

	config_file = g_strconcat(geany->app->configdir, G_DIR_SEPARATOR_S, "plugins", G_DIR_SEPARATOR_S,
		"geanysendmail", G_DIR_SEPARATOR_S, "mail.conf", NULL);

	// Initialising options from config file
	g_key_file_load_from_file(config, config_file, G_KEY_FILE_NONE, NULL);
	mailer = g_key_file_get_string(config, "tools", "mailer", NULL);
	address = g_key_file_get_string(config, "tools", "address", NULL);
	use_address_dialog = g_key_file_get_boolean(config, "tools", "address_usage", NULL);
	icon_in_toolbar = g_key_file_get_boolean(config, "icon", "show_icon", NULL);

	g_key_file_free(config);

	tooltips = gtk_tooltips_new();

	if (icon_in_toolbar == TRUE)
	{
		show_icon();
	}

	// Build up menu entry
	menu_mail = gtk_menu_item_new_with_mnemonic(_("_Mail document"));
	gtk_container_add(GTK_CONTAINER(geany->main_widgets->tools_menu), menu_mail);
	gtk_tooltips_set_tip(tooltips, menu_mail,
			     _("Sends the opened file as unzipped attachment by any mailer from your $PATH"), NULL);
	g_signal_connect(G_OBJECT(menu_mail), "activate", G_CALLBACK(send_as_attachment), NULL);

	/* setup keybindings */
	p_keybindings->set_item(plugin_key_group, SENDMAIL_KB, key_send_as_attachment,
		0, 0, "send_file_as_attachment", kb_label, menu_mail);

	gtk_widget_show_all(menu_mail);
	p_ui->add_document_sensitive(menu_mail);
	main_menu_item = menu_mail;
}


void plugin_cleanup()
{
	gtk_widget_destroy(main_menu_item);
	cleanup_icon();
	g_free(mailer);
	g_free(address);
	g_free(config_file);
}
