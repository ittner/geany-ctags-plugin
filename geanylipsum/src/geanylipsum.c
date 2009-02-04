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

#include <string.h>
#ifdef HAVE_LOCALE_H
# include <locale.h>
#endif

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

PLUGIN_VERSION_CHECK(130)
PLUGIN_SET_INFO(_("Lipsum"), _("Creating dummy text with Geany"), VERSION, "Frank Lanitz <frank@frank.uvena.de>");

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
	GeanyDocument *doc = NULL;

	/* Setting default length to 1500 characters */
	gdouble value = 1500;

	doc = document_get_current();

	if (dialogs_show_input_numeric(_("Lipsum-Generator"),
		_("Enter the length of Lipsum text here"), &value, 1, 5000, 1))
	{

		int tmp = 0;
		int x = 0;
		int i = 0;
		int missing = 0;

		/* Checking what we have */
		tmp = strlen(lipsum);

		if (tmp > value)
		{
			x = 0;
			missing = value - (x * tmp);
		}
		else if (tmp == (int)value)
		{
			x = 1;
		}
		else if (tmp > 0)
		{
			x = value / tmp;
			missing = value - (x * tmp);
		}

		/* Insert lipsum snippet as often as needed ... */
		for (i = 0; i < x; i++)
			insert_string(lipsum);

		/* .. and insert a little more if needed */
		if (missing > 0)
			insert_string(g_strndup(lipsum, missing));

	}
}


/* Called when keystroke were pressed */
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

	/* Initialising options from config file  if there is any*/
	g_key_file_load_from_file(config, config_file, G_KEY_FILE_NONE, NULL);
	lipsum = g_key_file_get_string(config, "snippets", "lipsumtext", NULL);

	/* Setting default value */
	if (lipsum == NULL)
	{
		lipsum = g_strdup(LOREMIPSUM);
	}
	g_key_file_free(config);

	/* Building menu entry */
	menu_lipsum = gtk_image_menu_item_new_with_mnemonic(_("_Lipsum"));
	gtk_tooltips_set_tip(tooltips, menu_lipsum,
			     _("Include Pseudotext to your code"), NULL);
	gtk_widget_show(menu_lipsum);
	g_signal_connect((gpointer) menu_lipsum, "activate",
			 G_CALLBACK(lipsum_activated), NULL);
	gtk_container_add(GTK_CONTAINER(geany->main_widgets->tools_menu), menu_lipsum);

	main_menu_item = menu_lipsum;

	/* init keybindings */
	keybindings_set_item(plugin_key_group, LIPSUM_KB_INSERT, kblipsum_insert,
		0, 0, "inster_lipsum", _("Insert Lipsum text"), menu_lipsum);

}

/* Called by Geany before unloading the plugin. */
void plugin_cleanup(void)
{
	/* remove the menu item added in plugin_init() */
	gtk_widget_destroy(main_menu_item);

	/* free lipsum snippet */
	g_free(lipsum);
}

