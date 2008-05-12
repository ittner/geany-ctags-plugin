/*
 *      geanysendmail.c
 *
 *      Copyright 2007,2008 Frank Lanitz <frank(at)frank(dot)uvena(dot)de>
 * 		Copyright 2007 Enrico Tröger <enrico.troeger@uvena.de>
 *		Copyright 2007 Nick Treleaven <nick.treleaven@btinternet.com>
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
#include "support.h"
#include "plugindata.h"
#include "document.h"
#include "filetypes.h"
#include "pluginmacros.h"
#include "utils.h"
#include "keybindings.h"
#include "icon.h"

PluginFields	*plugin_fields;
GeanyData		*geany_data;

VERSION_CHECK(49)

PLUGIN_INFO(_("GeanySendMail"), _("A little plugin to send the current \
file as attachment by user's favorite mailer"), "0.4dev", "Frank Lanitz <frank@frank.uvena.de>")

/* Keybinding(s) */
enum
{
	SENDMAIL_KB,
	COUNT_KB
};

PLUGIN_KEY_GROUP(sendmail, COUNT_KB)


static gchar *config_file = NULL;
static gchar *mailer = NULL;
gboolean icon_in_toolbar = FALSE;	
/* Needed global to remove from toolbar again */
GtkWidget *mailbutton = NULL;
GtkWidget *separator = NULL;



void configure(GtkWidget *parent)
{
	GtkWidget	*dialog, *label1, *label2, *entry, *vbox;
	GtkWidget	*checkbox_icon_to_toolbar = NULL;
	GKeyFile 	*config = g_key_file_new();
	gchar 		*config_dir = g_path_get_dirname(config_file);
	gint 		tmp;
	GtkTooltips *tooltip = NULL;

	tooltip = gtk_tooltips_new();

	dialog = gtk_dialog_new_with_buttons(_("Mail Configuration"),
		GTK_WINDOW(parent), GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
	vbox = ui->dialog_vbox_new(GTK_DIALOG(dialog));
	gtk_widget_set_name(dialog, "GeanyDialog");
	gtk_box_set_spacing(GTK_BOX(vbox), 10);

	// add a label and a text entry to the dialog
	label1 = gtk_label_new(_("Path and options for the mail programm:"));
	gtk_widget_show(label1);
	gtk_misc_set_alignment(GTK_MISC(label1), 0, 0.5);
	entry = gtk_entry_new();
	gtk_widget_show(entry);
	if (mailer != NULL)
		gtk_entry_set_text(GTK_ENTRY(entry), mailer);

	label2 = gtk_label_new(_("Note: \%f will be replaced by your filename."));
	gtk_widget_show(label2);
	gtk_misc_set_alignment(GTK_MISC(label2), 0, 0.5);

	checkbox_icon_to_toolbar = gtk_check_button_new_with_label(_("Showing icon in toolbar (EXPERIMENTAL)"));
	gtk_tooltips_set_tip(tooltip, checkbox_icon_to_toolbar,
			     _
			     ("Shows a icon in the toolbar to send file more easy."),
			     NULL);
	gtk_button_set_focus_on_click(GTK_BUTTON(checkbox_icon_to_toolbar), FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox_icon_to_toolbar), icon_in_toolbar);
	gtk_widget_show(checkbox_icon_to_toolbar); 
	
	gtk_container_add(GTK_CONTAINER(vbox), label1);
	gtk_container_add(GTK_CONTAINER(vbox), entry);
	gtk_container_add(GTK_CONTAINER(vbox), label2);
	gtk_box_pack_start(GTK_BOX(vbox), checkbox_icon_to_toolbar, TRUE, FALSE, 2);
		
	gtk_widget_show(vbox);

	// run the dialog and check for the response code
	tmp = gtk_dialog_run(GTK_DIALOG(dialog));
	if (tmp == GTK_RESPONSE_ACCEPT)
	{
		g_free(mailer);
		mailer = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
		
		icon_in_toolbar = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbox_icon_to_toolbar));
		
		g_key_file_load_from_file(config, config_file, G_KEY_FILE_NONE, NULL);
		g_key_file_set_string(config, "tools", "mailer", mailer);
		g_key_file_set_boolean(config, "icon", "show_icon", icon_in_toolbar);

		if (! g_file_test(config_dir, G_FILE_TEST_IS_DIR) && utils->mkdir(config_dir, TRUE) != 0)
		{
			dialogs->show_msgbox(GTK_MESSAGE_ERROR,
				_("Plugin configuration directory could not be created."));
		}
		else
		{
			// write config to file
			gchar *data = g_key_file_to_data(config, NULL, NULL);
			utils->write_file(config_file, data);
			g_free(data);
		}
		g_key_file_free(config);
		g_free(config_dir);
	}

	gtk_widget_destroy(dialog);
}

/* Callback for sending file as attachment */
static void
send_as_attachment(G_GNUC_UNUSED GtkMenuItem *menuitem, G_GNUC_UNUSED gpointer gdata)
{
	guint	idx;
	gchar	*locale_filename = NULL;
	gchar	*command = NULL;
	GError	*error = NULL;
	GString	*cmd_str = NULL;


	idx = documents->get_cur_idx();

	if (doc_list[idx].file_name == NULL)
	{
		dialogs->show_save_as();
	}
	else
	{
		documents->save_file(idx, FALSE);
	}

    if (doc_list[idx].file_name != NULL)
	{
		if (mailer)
		{
			locale_filename = utils->get_locale_from_utf8(doc_list[idx].file_name);
			cmd_str = g_string_new(mailer);

			if (! utils->string_replace_all(cmd_str, "%f", locale_filename))
				ui->set_statusbar(FALSE, _("Filename placeholder not found. The executed command might have failed."));

			command = g_string_free(cmd_str, FALSE);
			g_spawn_command_line_async(command, &error);
			if (error != NULL)
			{
				ui->set_statusbar(FALSE, _("Could not execute mailer. Please check your configuration."));
				g_error_free(error);
			}

			g_free(locale_filename);
			g_free(command);
		}
		else
		{
			ui->set_statusbar(FALSE, _("Have to define some mailing tool before."));
		}
	}
	else
	{
		ui->set_statusbar(FALSE, _("File have to be saved before sending."));
	}
}

static void key_send_as_attachment(G_GNUC_UNUSED guint key_id)
{
	send_as_attachment(NULL, NULL);
}


/* Called by Geany to initialize the plugin */
void init(GeanyData *data)
{
	GtkTooltips *tooltips = NULL;

	GKeyFile *config = g_key_file_new();

	gchar *kb_label = _("Send file by mail");

	GtkWidget *menu_mail = NULL;
	GtkWidget *menu_mail_submenu = NULL;
	GtkWidget *menu_mail_attachment = NULL;
	GdkPixbuf *pixbuf = NULL;
	GtkWidget *icon = NULL;

	config_file = g_strconcat(app->configdir, G_DIR_SEPARATOR_S, "plugins", G_DIR_SEPARATOR_S,
		"geanysendmail", G_DIR_SEPARATOR_S, "mail.conf", NULL);

	// Initialising options from config file
	g_key_file_load_from_file(config, config_file, G_KEY_FILE_NONE, NULL);
	mailer = g_key_file_get_string(config, "tools", "mailer", NULL);
	icon_in_toolbar =g_key_file_get_boolean(config, "icon", "show_icon", NULL);

	g_key_file_free(config);

	tooltips = gtk_tooltips_new();

	plugin_fields->flags = PLUGIN_IS_DOCUMENT_SENSITIVE;

	if (icon_in_toolbar == TRUE)
	{
		pixbuf = gdk_pixbuf_new_from_inline(-1, mail_pixbuf, FALSE, NULL);
		icon = gtk_image_new_from_pixbuf(pixbuf);
		g_object_unref(pixbuf);

		separator = (GtkWidget*) gtk_separator_tool_item_new ();
		gtk_widget_show (separator);
		gtk_container_add (GTK_CONTAINER (app->toolbar), separator);

		mailbutton = (GtkWidget*) gtk_tool_button_new (icon, "Mail");
		gtk_container_add (GTK_CONTAINER (app->toolbar), mailbutton);
		g_signal_connect (G_OBJECT(mailbutton), "clicked", G_CALLBACK(send_as_attachment), NULL);
		gtk_widget_show_all (mailbutton);
	}

	// Build up menu

	menu_mail = gtk_image_menu_item_new_with_mnemonic(_("_Mail"));
	gtk_container_add(GTK_CONTAINER(data->tools_menu), menu_mail);

	menu_mail_submenu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_mail), menu_mail_submenu);

	menu_mail_attachment = gtk_menu_item_new_with_mnemonic(_("_Mail document"));
	gtk_container_add(GTK_CONTAINER(menu_mail_submenu), menu_mail_attachment);
	gtk_tooltips_set_tip(tooltips, menu_mail_attachment,
			     _("Sends the opened file as unzipped attachment by any mailer from your $PATH"), NULL);
	g_signal_connect(G_OBJECT(menu_mail_attachment), "activate", G_CALLBACK(send_as_attachment), NULL);

	/* setup keybindings */
	p_keybindings->set_item(plugin_key_group, SENDMAIL_KB, key_send_as_attachment,
		0, 0, "send_file_as_attachment", kb_label, menu_mail_attachment);

	gtk_widget_show_all(menu_mail);
	plugin_fields->menu_item = menu_mail;
}


void cleanup()
{
	gtk_widget_destroy(plugin_fields->menu_item);
	if (mailbutton != NULL)
	{
		gtk_container_remove(GTK_CONTAINER (app->toolbar), mailbutton);
	}
	if (separator != NULL)
	{
		gtk_container_remove(GTK_CONTAINER (app->toolbar), separator);
	}
	g_free(mailer);
	g_free(config_file);
}
