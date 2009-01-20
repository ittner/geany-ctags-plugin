/*
 *      geanylipsum.c
 *
 *      Copyright 2008-2009 Frank Lanitz <frank(at)frank(dot)uvena(dot)de>
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


#include "geany.h"
#include "ui_utils.h"
#include "support.h"
#include "plugindata.h"
#include "document.h"
#include "filetypes.h"
#include "utils.h"
#include "keybindings.h"
#include "geanyfunctions.h"

#ifdef HAVE_LOCALE_H
# include <locale.h>
#endif


/* For your templates you can use:
 * {{TITLE_S}}: Start for title e.g <H1>
 * {{TITLE_E}}: End of title e.g. </H1>
 * {{T_CONTENT}}: Content for the title
 * {{SUBTITLE_S}}: Start of subtitle e.g. <H2>
 * {{SUBTITLE_S}}: End of subtitle e.g. </H2>
 * {{S_CONTENT}}: Content of subtitle
 * {{B_CONTENT}}: Content of Body
*/

#define LOREMIPSUM "\
Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy\
eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam\
voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet \
clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet."

enum {
  PLAIN_LIPSUM = 0,
};

GeanyPlugin		*geany_plugin;
GeanyData		*geany_data;
GeanyFunctions	*geany_functions;

PLUGIN_VERSION_CHECK(115)
PLUGIN_SET_INFO(_("Lipsum"), _("Creating dummy text with Geany"), VERSION, _("Frank Lanitz <frank@frank.uvena.de>"));

static GtkWidget *main_menu_item = NULL;
static gchar *config_file = NULL;
static gchar *lipsum = NULL;


/* Doing some basic keybinding stuff */
enum
{
	LIPSUM_KB_INSERT,
	COUNT_KB
};

PLUGIN_KEY_GROUP(geanylipsum, COUNT_KB);



void
insert_string(gchar *string)
{
	GeanyDocument *doc = NULL;

	doc = document_get_current();

	if (doc != NULL)
	{
		gint pos = sci_get_current_position(doc->editor->sci);
		sci_insert_text(doc->editor->sci, pos, string);
	}
}


void
lipsum_activated(G_GNUC_UNUSED GtkMenuItem *menuitem, G_GNUC_UNUSED gpointer gdata)
{
	GtkWidget *dialog = NULL;
	GtkWidget *vbox = NULL;
	GtkWidget *label = NULL;
	GtkWidget *hbox = NULL;
	GtkWidget *spin = NULL;
	GtkTooltips *tooltip = NULL;
	GeanyDocument *doc = NULL;

	int length = 0;

	tooltip = gtk_tooltips_new();

	doc = document_get_current();

	dialog = gtk_dialog_new_with_buttons(_("Lipsum-generator"),
 					GTK_WINDOW(geany->main_widgets->window),
					GTK_DIALOG_DESTROY_WITH_PARENT,
 					GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
	vbox = ui_dialog_vbox_new(GTK_DIALOG(dialog));
	gtk_widget_set_name(dialog, "GeanyDialog");
	gtk_box_set_spacing(GTK_BOX(vbox), 10);

	label = gtk_label_new(_("characters"));
	spin = gtk_spin_button_new_with_range(1, 1800, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), 1500);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hbox), spin, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

	gtk_widget_show_all(vbox);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{

		int tmp = 0;
		int x = 0;
		int i = 0;
		int missing = 0;

		/* Checking for length of string that should be inserted */
		length = gtk_spin_button_get_value_as_int(
			GTK_SPIN_BUTTON(spin));

		gtk_widget_destroy(dialog);

		/* Checking what we have */

		tmp = strlen(lipsum);

		if (tmp > length)
		{
			x = 0;
			missing = length - (x * tmp);
		}
		else if (tmp == length)
		{
			x = 1;
		}
		else if (tmp > 0)
		{
			x = length / tmp;
			missing = length - (x * tmp);
		}

		for (i = 0; i < x; i++)
			insert_string(lipsum);

		if (missing > 0)
			insert_string(g_strndup(lipsum, missing));
	}

	gtk_widget_destroy(dialog);


}

static void kblipsum_insert(G_GNUC_UNUSED guint key_id)
{
	lipsum_activated(NULL, NULL);
}


/* Called by Geany to initialize the plugin */
void
plugin_init(G_GNUC_UNUSED GeanyData *data)
{
	GtkWidget *menu_lipsum = NULL;
	GKeyFile *config = g_key_file_new();
	GtkTooltips *tooltips = NULL;
	tooltips = gtk_tooltips_new();

	main_locale_init(LOCALEDIR, GETTEXT_PACKAGE);

	config_file = g_strconcat(geany->app->configdir,
		G_DIR_SEPARATOR_S, "plugins", G_DIR_SEPARATOR_S,
		"geanylipsum", G_DIR_SEPARATOR_S, "lipsum.conf", NULL);

	/* Initialising options from config file */
	g_key_file_load_from_file(config, config_file, G_KEY_FILE_NONE, NULL);
	lipsum = g_key_file_get_string(config, "snippets", "lipsumtext", NULL);

	/* Setting default value */
	if (lipsum == NULL)
	{
		lipsum = g_strdup(LOREMIPSUM);
	}
	g_key_file_free(config);

	menu_lipsum = gtk_image_menu_item_new_with_mnemonic(_("_Lipsum"));
	gtk_tooltips_set_tip(tooltips, menu_lipsum,
			     _("Include Pseudotext to your code"), NULL);
	gtk_widget_show(menu_lipsum);
	g_signal_connect((gpointer) menu_lipsum, "activate",
			 G_CALLBACK(lipsum_activated), NULL);
	gtk_container_add(GTK_CONTAINER(geany->main_widgets->tools_menu), menu_lipsum);

	/* init keybindins */
	keybindings_set_item(plugin_key_group, LIPSUM_KB_INSERT, kblipsum_insert,
		0, 0, "inster_lipsum", _("Insert Lipsum tex"), menu_lipsum);

	main_menu_item = menu_lipsum;

}

/* Called by Geany before unloading the plugin. */
void plugin_cleanup(void)
{
	/* remove the menu item added in plugin_init() */
	gtk_widget_destroy(main_menu_item);
}

