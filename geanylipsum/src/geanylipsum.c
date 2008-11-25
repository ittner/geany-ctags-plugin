/*
 *      geanylipsum.c
 *
 *      Copyright 2008 Frank Lanitz <frank(at)frank(dot)uvena(dot)de>
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
#include "pluginmacros.h"


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

#define TITLE "\
{{TITLE_S}}{{T_CONTENT}}{{TITLE_E}}"

#define SUBTITLE "\
{{SUBTITLE_S}}{{S_CONTENT}}{{SUBTITLE_S}}"

#define CONTENT "\
{{B_CONTENT}}"

GeanyPlugin		*geany_plugin;
GeanyData		*geany_data;
GeanyFunctions	*geany_functions;

PLUGIN_VERSION_CHECK(104)
PLUGIN_SET_INFO(_("Lipsum"), _("Creating dummy text with Geany"), VERSION, _("Frank Lanitz <frank@frank.uvena.de>"));

static GtkWidget *main_menu_item = NULL;

void
insert_string(gchar *string)
{
	GeanyDocument *doc = NULL;

	doc = p_document->get_current();

	if (doc != NULL)
	{
		gint pos = p_sci->get_current_position(doc->editor->sci);
		p_sci->insert_text(doc->editor->sci, pos, string);
	}
}

/* Only inserts a default tet. So dialog is some kind of useless at the moment ;) */
void
lipsum_activated(G_GNUC_UNUSED GtkMenuItem *menuitem, G_GNUC_UNUSED gpointer gdata)
{
	gint type;
	GtkWidget *dialog = NULL;
	GtkWidget *vbox = NULL;
	GtkWidget *radio1 = NULL;
	GtkWidget *radio2= NULL;
	GtkTooltips *tooltip = NULL;
	tooltip = gtk_tooltips_new();

	dialog = gtk_dialog_new_with_buttons(_("Lipsum-generator"),
 					GTK_WINDOW(geany->main_widgets->window),
					GTK_DIALOG_DESTROY_WITH_PARENT,
 					GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
	vbox = p_ui->dialog_vbox_new(GTK_DIALOG(dialog));
	gtk_widget_set_name(dialog, "GeanyDialog");
	gtk_box_set_spacing(GTK_BOX(vbox), 10);

	radio1 = gtk_radio_button_new_with_label(NULL,
		_("HTML"));
	gtk_button_set_focus_on_click(GTK_BUTTON(radio1), FALSE);
	gtk_container_add(GTK_CONTAINER(vbox), radio1);

	radio2 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio1),
		_("LaTeX"));
	gtk_button_set_focus_on_click(GTK_BUTTON(radio2), FALSE);
	gtk_container_add(GTK_CONTAINER(vbox), radio2);

	gtk_widget_show_all(vbox);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		// Checking, what the user likes to have
		// Filetyp
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio1)))
		{
			type = 0;
		}
		else
		{
			type = 1;
		}
		// Number of titles/paragraphes etc.
		// t.b.d.
		gtk_widget_destroy(dialog);
	}
	else
	{
		gtk_widget_destroy(dialog);
	}

	insert_string(LOREMIPSUM);

}


/* Called by Geany to initialize the plugin */
void
plugin_init(G_GNUC_UNUSED GeanyData *data)
{
	GtkWidget *menu_lipsum = NULL;
	GtkTooltips *tooltips = NULL;
	tooltips = gtk_tooltips_new();

	menu_lipsum = gtk_image_menu_item_new_with_mnemonic(_("_Lipsum"));
	gtk_tooltips_set_tip(tooltips, menu_lipsum,
			     _("Include Pseudotext to your code"), NULL);
	gtk_widget_show(menu_lipsum);
	g_signal_connect((gpointer) menu_lipsum, "activate",
			 G_CALLBACK(lipsum_activated), NULL);
	gtk_container_add(GTK_CONTAINER(geany->main_widgets->tools_menu), menu_lipsum);

	main_menu_item = menu_lipsum;
}

/* Called by Geany before unloading the plugin. */
void plugin_cleanup(void)
{
	/* remove the menu item added in plugin_init() */
	gtk_widget_destroy(main_menu_item);
}

