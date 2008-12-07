/*
 *      geanylatex.c - Plugin to let Geany better work together with LaTeX
 *
 *      Copyright 2007-2008 Frank Lanitz <frank(at)frank(dot)uvena(dot)de>
 *      For long list of friendly supporters please have a look at THANKS.
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

/* LaTeX plugin */
/* This plugin improves the work with LaTeX and Geany.*/

#include "geanylatex.h"

PLUGIN_VERSION_CHECK(115)

PLUGIN_SET_INFO(_("LaTeX"), _("Plugin to provide better LaTeX support"), "0.3dev",
	    "Frank Lanitz <frank@frank.uvena.de>")

GeanyPlugin	*geany_plugin;
GeanyData	*geany_data;
GeanyFunctions	*geany_functions;


GtkWidget *menu_latex = NULL;
GtkWidget *menu_latex_menu = NULL;
GtkWidget *menu_latex_wizzard = NULL;
GtkWidget *menu_latex_menu_special_char = NULL;
GtkWidget *menu_latex_menu_special_char_submenu = NULL;
GtkWidget *menu_latex_ref = NULL;
GtkWidget *menu_latex_label = NULL;
GtkWidget *menu_latex_bibtex = NULL;
GtkWidget *menu_latex_bibtex_submenu = NULL;

static GtkWidget *main_menu_item = NULL;

/* Doing some basic keybinding stuff */
enum
{
	LATEX_WIZZARD_KB,
	LATEX_INSERT_LABEL_KB,
	LATEX_INSERT_REF_KB,
/*	LATEX_INSERT_BIBTEX_ENTRY_KB,*/
	COUNT_KB
};

PLUGIN_KEY_GROUP(geanylatex, COUNT_KB)

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


inline gchar*
get_latex_command(gint tab_index)
{
	return char_array[tab_index].latex;
}

static void
char_insert_activated(G_GNUC_UNUSED GtkMenuItem * menuitem, G_GNUC_UNUSED gpointer gdata)
{
	insert_string(get_latex_command(GPOINTER_TO_INT(gdata)));
}

static void
insert_label_activated(G_GNUC_UNUSED GtkMenuItem * menuitem, G_GNUC_UNUSED gpointer gdata)
{
	GtkWidget *dialog = NULL;
	GtkWidget *vbox = NULL;
	GtkWidget *label = NULL;
	GtkWidget *textbox_label = NULL;
	GtkWidget *table = NULL;

	dialog = gtk_dialog_new_with_buttons(_("Insert label"),
					     GTK_WINDOW(geany->main_widgets->window),
					     GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CANCEL,
					     GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
					     NULL);
	vbox = ui_dialog_vbox_new(GTK_DIALOG(dialog));
	gtk_widget_set_name(dialog, "GeanyDialog");
	gtk_box_set_spacing(GTK_BOX(vbox), 10);

	table = gtk_table_new(1, 2, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(table), 6);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);

	label = gtk_label_new(_("Label name:"));
	textbox_label = gtk_entry_new();

	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), textbox_label, 1, 2, 0, 1);
	gtk_container_add(GTK_CONTAINER(vbox), table);

	gtk_widget_show_all(vbox);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		gchar *label_str = NULL;
		label_str = g_strconcat("\\label{", g_strdup(gtk_entry_get_text(GTK_ENTRY(textbox_label))), "}", NULL);
		insert_string(label_str);
	}
	gtk_widget_destroy(dialog);
}

static void
insert_ref_activated(G_GNUC_UNUSED GtkMenuItem * menuitem, G_GNUC_UNUSED gpointer gdata)
{
	GtkWidget *dialog;
	GtkWidget *vbox = NULL;
	GtkWidget *label_ref = NULL;
	GtkWidget *textbox_ref = NULL;
	GtkWidget *table = NULL;
	GtkWidget *radio1 = NULL;
	GtkWidget *radio2 = NULL;

	dialog = gtk_dialog_new_with_buttons(_("Insert reference"),
					     GTK_WINDOW(geany->main_widgets->window),
					     GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CANCEL,
					     GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
					     NULL);
	vbox = ui_dialog_vbox_new(GTK_DIALOG(dialog));
	gtk_widget_set_name(dialog, "GeanyDialog");
	gtk_box_set_spacing(GTK_BOX(vbox), 10);

	table = gtk_table_new(1, 2, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(table), 6);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);

	label_ref = gtk_label_new(_("Ref name:"));
	textbox_ref = gtk_entry_new();

	gtk_misc_set_alignment(GTK_MISC(label_ref), 0, 0.5);

	gtk_table_attach_defaults(GTK_TABLE(table), label_ref, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), textbox_ref, 1, 2, 0, 1);
	gtk_container_add(GTK_CONTAINER(vbox), table);

	radio1 = gtk_radio_button_new_with_label(NULL,
		_("standard reference"));
	gtk_button_set_focus_on_click(GTK_BUTTON(radio1), FALSE);
	gtk_container_add(GTK_CONTAINER(vbox), radio1);

	radio2 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio1),
		_("page reference"));
	gtk_button_set_focus_on_click(GTK_BUTTON(radio2), FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio2), FALSE);
	gtk_container_add(GTK_CONTAINER(vbox), radio2);

	gtk_widget_show_all(vbox);


	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		gchar *ref_string = NULL;

		ref_string = g_strdup(gtk_entry_get_text(GTK_ENTRY(textbox_ref)));

		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio2)) == FALSE)
		{
			ref_string = g_strconcat("\\ref{", ref_string, "}", NULL);
		}
		else
		{
			ref_string = g_strconcat("\\pageref{", ref_string, "}", NULL);
		}

		if (ref_string != NULL)
		{
			insert_string(ref_string);
			g_free(ref_string);
		}
	}

	gtk_widget_destroy(dialog);
}

static void character_create_menu_item(GtkWidget *menu, const gchar *label, gint letter, SubMenuCallback callback)
{
	GtkWidget *tmp;

	tmp = gtk_menu_item_new_with_label(label);
	gtk_widget_show(tmp);
	gtk_container_add(GTK_CONTAINER(menu), tmp);
	g_signal_connect((gpointer) tmp, "activate", G_CALLBACK(callback), GINT_TO_POINTER(letter));
}

/* returns -1, if there are more than gint can work with or any other error
 * returns 0, if categorie is empty
 * if gint categorie is -1, function will count every element.
 * Useful, if there is no need for a categorie check.*/
gint
count_menu_entries(SubMenuTemplate *tmp, gint categorie)
{
	// TODO: Reset max value to stop before it's too late
	gint i;
	gint count = 0;
	if (categorie == -1)
	{
		for (i =1; tmp[i].label != NULL; i++)
		{
			count = count + 1;
		}
	}
	else
	{
		for (i = 1; tmp[i].label != NULL; i++)
		{
			if (tmp[i].cat == categorie)
			{
				count = count + 1;
			}
			if (i >= 256)
			{
				count = -1;
				break;
			}
		}
	}
	return count;
}

gint
count_menu_cat_entries(CategoryName *tmp)
{
	gint i;

	for (i = 0; tmp[i].label != NULL; i++);
	return i;
}

void sub_menu_init(GtkWidget *base_menu, SubMenuTemplate *menu_template, CategoryName *category_name, SubMenuCallback callback_function)
{
	gint i;
	gint j;
	gint categories = count_menu_cat_entries(category_name);
	GtkWidget *sub_menu = NULL;
	GtkWidget *sub_menu_cat[categories][2];
	GtkWidget *active_submenu = NULL;

	// Creates sub menus based on information from letter.h
	for (i = 0; i < categories; i++)
	{
		if (count_menu_entries(menu_template, i) > 0)
		{
			create_sub_menu(base_menu, sub_menu_cat[i][0],
			 sub_menu_cat[i][1], category_name[i].label);
		}
	}

	// Searching for all categories
	for (i = 0; i < categories; i++)
	{
		gboolean split = FALSE;
		gboolean last_sub_menu = FALSE;
		gboolean sorted = category_name[i].sorted;
		gint local_count = 0; // To check whether we need to build up a new sub sub menu.
		gint item_count = count_menu_entries(menu_template, i);

		if (item_count < 1)
			continue;

		// Default is, not to split anything to make menu not
		// deeper than realy needed.
		if (item_count > MAX_MENU_ENTRIES)
		{
			split = TRUE;
		}

		// Setting active sub menu to sub menu of category
		sub_menu = sub_menu_cat[i][0];
		active_submenu = sub_menu;
		// Finding entries for each category

		for (j = 0; menu_template[j].latex != NULL; j++)
		{
			if (menu_template[j].cat == i)
			{
				// Creates a new sub sub menu if needed
				if (split == TRUE && (local_count % MAX_MENU_ENTRIES) == 0)
				{
					gint next_split_point = 0;
					GtkWidget *tmp = NULL;
					GtkWidget *tmp_item = NULL;

					sub_menu = active_submenu;

					for (next_split_point = 0;
						next_split_point < MAX_MENU_ENTRIES ; next_split_point ++)
					{
						if (menu_template[j+next_split_point].cat != i)
						{
							last_sub_menu = TRUE;
							break;
						}

					}

					if (sorted == TRUE)
					{
						create_sub_menu(sub_menu_cat[i][0], tmp, tmp_item, g_strconcat(menu_template[j].label, " ... ", menu_template[j + next_split_point-1].label, NULL));

						sub_menu = tmp;
					}
					else if (sorted == FALSE)
					{
						if (last_sub_menu == FALSE)
						{
							create_sub_menu(sub_menu, tmp, tmp_item, _("More"));
							sub_menu = active_submenu;
							active_submenu = tmp;
						}
					}
				}

				// Sets the counter to keep in track if a new submenu needs to be build up
				local_count = local_count + 1;
				character_create_menu_item(sub_menu, g_strconcat(menu_template[j].label, "\t", menu_template[j].latex, NULL), j, callback_function);
			}
		}
	}
}

static int
find_latex_enc(gint l_geany_enc)
{
	guint i;
	for (i = 0; i < LATEX_ENCODINGS_MAX; i++)
	{
		if (latex_encodings[i].geany_enc == l_geany_enc)
			return i;
	}
	return LATEX_ENCODING_NONE;
}


static void
show_output(const gchar * output, const gchar * name, const gint local_enc)
{
	GeanyDocument *doc = NULL;
	GeanyFiletype *ft = filetypes_lookup_by_name("LaTeX");

	if (output)
	{
		doc = document_new_file(name, ft, output);
		document_set_encoding(doc, encodings_get_charset_from_index(latex_encodings[local_enc].geany_enc));
	}

}

static void
wizard_activated(G_GNUC_UNUSED GtkMenuItem * menuitem, G_GNUC_UNUSED gpointer gdata)
{
	gint i;
	GString *code = NULL;
	gchar *author = NULL;
	gchar *date = NULL;
	gchar *output = NULL;
	gchar *classoptions = NULL;
	gchar *title = NULL;
	gchar *enc_latex_char = NULL;
	gchar *documentclass_str = NULL;
	gchar *papersize = NULL;
	gchar *draft = NULL;
	gchar *scriptsize = NULL;
	gint documentclass_int;
	gint encoding_int;
	gint papersize_int;
	gint scriptsize_int;
	GtkWidget *dialog = NULL;
	GtkWidget *vbox = NULL;
	GtkWidget *label_documentclass = NULL;
	GtkWidget *documentclass_combobox = NULL;
	GtkWidget *label_encoding = NULL;
	GtkWidget *encoding_combobox = NULL;
	GtkWidget *scriptsize_combobox = NULL;
	GtkWidget *label_scriptsize = NULL;
	GtkWidget *table = NULL;
	GtkWidget *checkbox_KOMA = NULL;
	GtkWidget *author_textbox = NULL;
	GtkWidget *label_author = NULL;
	GtkWidget *date_textbox = NULL;
	GtkWidget *label_date = NULL;
	GtkWidget *title_textbox = NULL;
	GtkWidget *label_title = NULL;
	GtkWidget *papersize_combobox = NULL;
	GtkWidget *label_papersize = NULL;
	GtkWidget *checkbox_draft = NULL;
	gboolean KOMA_active = TRUE;
	gboolean draft_active = FALSE;

	GtkTooltips *tooltip = gtk_tooltips_new();

	// Creating and formatting table
	table = gtk_table_new(2, 6, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(table), 6);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);

	// Documentclass
	label_documentclass = gtk_label_new(_("Documentclass:"));
	documentclass_combobox = gtk_combo_box_new_text();
	gtk_tooltips_set_tip(tooltip, documentclass_combobox,
		_("Choose the kind of document you want to write"), NULL);
	gtk_combo_box_insert_text(GTK_COMBO_BOX(documentclass_combobox), 0, _("Book"));
	gtk_combo_box_insert_text(GTK_COMBO_BOX(documentclass_combobox), 1, _("Article"));
	gtk_combo_box_insert_text(GTK_COMBO_BOX(documentclass_combobox), 2, _("Report"));
	gtk_combo_box_insert_text(GTK_COMBO_BOX(documentclass_combobox), 3, _("Letter"));

	gtk_combo_box_set_active(GTK_COMBO_BOX(documentclass_combobox), 0);

	gtk_misc_set_alignment(GTK_MISC(label_documentclass), 0, 0.5);

	gtk_table_attach_defaults(GTK_TABLE(table), label_documentclass, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), documentclass_combobox, 1, 2, 0, 1);

	// Encoding
	label_encoding = gtk_label_new(_("Encoding:"));

	encoding_combobox = gtk_combo_box_new_text();
	gtk_tooltips_set_tip(tooltip, encoding_combobox,
		_("Set the encoding for your new document"), NULL);
	for (i = 0; i < LATEX_ENCODINGS_MAX; i++)
	{
		gtk_combo_box_insert_text(GTK_COMBO_BOX(encoding_combobox), i,
					  latex_encodings[i].name);
	}

	gtk_combo_box_set_active(GTK_COMBO_BOX(encoding_combobox), find_latex_enc(geany_data->file_prefs->default_new_encoding));

	gtk_misc_set_alignment(GTK_MISC(label_encoding), 0, 0.5);

	gtk_table_attach_defaults(GTK_TABLE(table), label_encoding, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(table), encoding_combobox, 1, 2, 1, 2);

	// Scriptsize
	label_scriptsize = gtk_label_new(_("Fontsize:"));
	scriptsize_combobox = gtk_combo_box_new_text();
	gtk_tooltips_set_tip(tooltip, scriptsize_combobox,
		_("Set the default fontsize of your new document"), NULL);
	gtk_combo_box_insert_text(GTK_COMBO_BOX(scriptsize_combobox), 0, "10pt");
	gtk_combo_box_insert_text(GTK_COMBO_BOX(scriptsize_combobox), 1, "11pt");
	gtk_combo_box_insert_text(GTK_COMBO_BOX(scriptsize_combobox), 2, "12pt");

	gtk_combo_box_set_active(GTK_COMBO_BOX(scriptsize_combobox), 0);

	gtk_misc_set_alignment(GTK_MISC(label_scriptsize), 0, 0.5);

	gtk_table_attach_defaults(GTK_TABLE(table), label_scriptsize, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(table), scriptsize_combobox, 1, 2, 2, 3);

	// Author
	label_author = gtk_label_new(_("Author:"));
	author_textbox = gtk_entry_new();
	gtk_tooltips_set_tip(tooltip, author_textbox,
		_("Sets the value of the \\author command. In most cases this should be your name"), NULL);
	if (geany_data->template_prefs->developer != NULL)
	{
		author = geany_data->template_prefs->developer;
		gtk_entry_set_text(GTK_ENTRY(author_textbox), author);
	}
	gtk_misc_set_alignment(GTK_MISC(label_author), 0, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(table), label_author, 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(table), author_textbox, 1, 2, 3, 4);

	// Date
	label_date = gtk_label_new(_("Date:"));
	date_textbox = gtk_entry_new();
	gtk_tooltips_set_tip(tooltip, date_textbox,
		_("Sets the value of the \\date command inside header of your newly created LaTeX-document. Keeping it at \\today is a good decision if you don't need any fixed date."), NULL);
	gtk_entry_set_text(GTK_ENTRY(date_textbox), "\\today");
	gtk_misc_set_alignment(GTK_MISC(label_date), 0, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(table), label_date, 0, 1, 4, 5);
	gtk_table_attach_defaults(GTK_TABLE(table), date_textbox, 1, 2, 4, 5);

	// Title of the new document
	label_title = gtk_label_new(_("Title:"));
	title_textbox = gtk_entry_new();
	gtk_tooltips_set_tip(tooltip, title_textbox, _("Sets the title of your new document."),
			     NULL);
	gtk_misc_set_alignment(GTK_MISC(label_title), 0, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(table), label_title, 0, 1, 5, 6);
	gtk_table_attach_defaults(GTK_TABLE(table), title_textbox, 1, 2, 5, 6);

	// Papersize
	label_papersize = gtk_label_new(_("Paper size:"));
	papersize_combobox = gtk_combo_box_new_text();
	gtk_tooltips_set_tip(tooltip, papersize_combobox,
		_("Choose the paper format for the newly created document"), NULL);
	gtk_combo_box_insert_text(GTK_COMBO_BOX(papersize_combobox), 0, "A4");
	gtk_combo_box_insert_text(GTK_COMBO_BOX(papersize_combobox), 1, "A5");
	gtk_combo_box_insert_text(GTK_COMBO_BOX(papersize_combobox), 2, "A6");

	gtk_combo_box_set_active(GTK_COMBO_BOX(papersize_combobox), 0);

	gtk_misc_set_alignment(GTK_MISC(label_papersize), 0, 0.5);

	gtk_table_attach_defaults(GTK_TABLE(table), label_papersize, 0, 1, 6, 7);
	gtk_table_attach_defaults(GTK_TABLE(table), papersize_combobox, 1, 2, 6, 7);

	gtk_widget_show_all(table);

	// Building the wizard-dialog and showing it
	dialog = gtk_dialog_new_with_buttons(_("LaTeX-Wizard"),
					     GTK_WINDOW(geany->main_widgets->window),
					     GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CANCEL,
					     GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
					     NULL);
	vbox = ui_dialog_vbox_new(GTK_DIALOG(dialog));
	gtk_widget_set_name(dialog, "GeanyDialog");
	gtk_box_set_spacing(GTK_BOX(vbox), 10);
	gtk_container_add(GTK_CONTAINER(vbox), table);

	checkbox_KOMA = gtk_check_button_new_with_label(_("Use KOMA-script classes if possible"));
	gtk_tooltips_set_tip(tooltip, checkbox_KOMA,
			     _
			     ("Uses the KOMA-script classes by Markus Kohm.\nKeep in mind: To compile your document these classes have to be installed before."),
			     NULL);
	gtk_button_set_focus_on_click(GTK_BUTTON(checkbox_KOMA), FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox_KOMA), KOMA_active);
	gtk_box_pack_start(GTK_BOX(vbox), checkbox_KOMA, FALSE, FALSE, 5);

	checkbox_draft = gtk_check_button_new_with_label(_("Use draft mode"));
	gtk_tooltips_set_tip(tooltip, checkbox_draft,
		_("Set the draft flag inside new created documents to get documents with a number of debugging helpers"), NULL);
	gtk_button_set_focus_on_click(GTK_BUTTON(checkbox_draft), FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox_draft), draft_active);
	gtk_box_pack_start(GTK_BOX(vbox), checkbox_draft, FALSE, FALSE, 5);

	gtk_widget_show_all(vbox);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		KOMA_active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbox_KOMA));
		draft_active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbox_draft));
		documentclass_int = gtk_combo_box_get_active(GTK_COMBO_BOX(documentclass_combobox));
		encoding_int = gtk_combo_box_get_active(GTK_COMBO_BOX(encoding_combobox));
		enc_latex_char = g_strconcat("\\usepackage[", latex_encodings[encoding_int].latex,"]{inputenc}\n", NULL);
		author = g_strdup(gtk_entry_get_text(GTK_ENTRY(author_textbox)));
		date = g_strdup(gtk_entry_get_text(GTK_ENTRY(date_textbox)));
		title = g_strdup(gtk_entry_get_text(GTK_ENTRY(title_textbox)));
		papersize_int = gtk_combo_box_get_active(GTK_COMBO_BOX(papersize_combobox));
		switch (papersize_int)
		{
			case 0:
			{
				papersize = g_utf8_casefold("a4paper", -1);
				break;
			}
			case 1:
			{
				papersize = g_utf8_casefold("a5paper", -1);
				break;
			}
			case 2:
			{
				papersize = g_utf8_casefold("a6paper", -1);
				break;
			}
		}

		scriptsize_int = gtk_combo_box_get_active(GTK_COMBO_BOX(scriptsize_combobox));
		switch (scriptsize_int)
		{
			case 0:
			{
				scriptsize = g_strconcat("10pt", NULL);
				break;
			}
			case 1:
			{
				scriptsize = g_strconcat("11pt", NULL);
				break;
			}
			case 2:
			{
				scriptsize = g_strconcat("12pt", NULL);
				break;
			}
		}

		if (papersize != NULL)
		{
			classoptions = g_strconcat(papersize, NULL);
		}
		if (classoptions != NULL && draft_active == TRUE)
		{
			draft = g_utf8_casefold("draft", -1);
			classoptions = g_strconcat(classoptions,",", draft, NULL);
		}
		else if (classoptions == NULL && draft_active == TRUE)
		{
			draft = g_utf8_casefold("draft", -1);
			classoptions = g_strconcat(draft, NULL);
		}
		if (classoptions != NULL && scriptsize != NULL)
		{
			classoptions = g_strconcat(classoptions, ",", scriptsize, NULL);
		}
		else if (classoptions == NULL && scriptsize != NULL)
		{
			classoptions = g_strconcat(scriptsize, NULL);
		}

		if (KOMA_active)
		{
			switch (documentclass_int)
			{
				case 0:
				{
					documentclass_str = g_utf8_casefold("scrbook", -1);
					break;
				}
				case 1:
				{
					documentclass_str = g_utf8_casefold("scrartcl", -1);
					break;
				}
				case 2:
				{
					documentclass_str = g_utf8_casefold("scrreprt", -1);
					break;
				}
				case 3:
				{
					documentclass_str = g_utf8_casefold("letter", -1);
				}
			}
		}
		else
		{
			switch (documentclass_int)
			{
				case 0:
				{
					documentclass_str = g_utf8_casefold("book", -1);
					break;
				}
				case 1:
				{
					documentclass_str = g_utf8_casefold("article", -1);
					break;
				}
				case 2:
				{
					documentclass_str = g_utf8_casefold("report", -1);
					break;
				}
			    case 3:
				{
				  	documentclass_str = g_utf8_casefold("letter", -1);
					break;
				}
			}
		}

		if (documentclass_int == 3)
			code = g_string_new(TEMPLATE_LATEX_LETTER);
		else
			code = g_string_new(TEMPLATE_LATEX);

		if (classoptions != NULL)
		{
			utils_string_replace_all(code, "{CLASSOPTION}", classoptions);
			g_free(classoptions);
		}
		if (documentclass_str != NULL)
		{
			utils_string_replace_all(code, "{DOCUMENTCLASS}", documentclass_str);
			g_free(documentclass_str);
		}
		if (enc_latex_char != NULL)
		{
			utils_string_replace_all(code, "{ENCODING}", enc_latex_char);
			g_free(enc_latex_char);
		}
		if (author != NULL)
		{
			if (author[0] != '\0')
			{
				if (documentclass_int == 3)
			  	{
			  		author = g_strconcat("\\signature{", author, "}\n", NULL);
				}
			  	else
				{
					author = g_strconcat("\\author{", author, "}\n", NULL);
				}

				utils_string_replace_all(code, "{AUTHOR}", author);
			}
			else
				if (documentclass_int == 3)
				{
					utils_string_replace_all(code, "{AUTHOR}", "\% \\signature{}\n");
				}
				else
				{
					utils_string_replace_all(code, "{AUTHOR}", "\% \\author{}\n");
				}

			g_free(author);
		}
		if (date != NULL)
		{
			if (date[0] != '\0')
			{
				date = g_strconcat("\\date{", date, "}\n", NULL);
				utils_string_replace_all(code, "{DATE}", date);
			}
			else
				utils_string_replace_all(code, "{DATE}", "\% \\date{}\n");
			g_free(date);
		}
		if (title != NULL)
		{
			if (title[0] != '\0')
			{
				if (documentclass_int == 3)
				{
					title = g_strconcat("\\subject{", title, "}\n", NULL);
				}
				else
				{
					title = g_strconcat("\\title{", title, "}\n", NULL);
				}

				utils_string_replace_all(code, "{TITLE}", title);
			}
			else
				if (documentclass_int == 3)
				{
					utils_string_replace_all(code, "{TITLE}", "\% \\subject{} \n");
				}
				else
				{
					utils_string_replace_all(code, "{TITLE}", "\% \\title{} \n");
				}

			g_free(title);
		}

		utils_string_replace_all(code, "{OPENING}", _("Dear Sir or Madame"));
		utils_string_replace_all(code, "{CLOSING}", _("With kind regards"));

		output = g_string_free(code, FALSE);
		show_output(output, NULL, encoding_int);
	}
	gtk_widget_destroy(dialog);
}


static void kblabel_insert(G_GNUC_UNUSED guint key_id)
{
	insert_label_activated(NULL, NULL);
}

static void kbref_insert(G_GNUC_UNUSED guint key_id)
{
	insert_ref_activated(NULL, NULL);
}

static void kbwizard(G_GNUC_UNUSED guint key_id)
{
	wizard_activated(NULL, NULL);
}

/*static void kb_bibtex_entry_insert(G_GNUC_UNUSED guint key_id)
{
	insert_bibtex_entry(NULL, NULL);
}*/

void
plugin_init(G_GNUC_UNUSED GeanyData * data)
{
	GtkTooltips *tooltips = NULL;
	GtkWidget *tmp = NULL;
	int i;

	main_locale_init(LOCALEDIR, GETTEXT_PACKAGE);

	init_encodings_latex();

	tooltips = gtk_tooltips_new();

	menu_latex = gtk_menu_item_new_with_mnemonic(_("_LaTeX"));
	gtk_container_add(GTK_CONTAINER(geany->main_widgets->tools_menu), menu_latex);

	menu_latex_menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_latex), menu_latex_menu);


	menu_latex_wizzard = ui_image_menu_item_new(GTK_STOCK_NEW, _("LaTeX-_Wizard"));
	gtk_container_add(GTK_CONTAINER(menu_latex_menu), menu_latex_wizzard);
	gtk_tooltips_set_tip(tooltips, menu_latex_wizzard,
			     _("Starts a Wizard to easily create LaTeX-documents"), NULL);

	g_signal_connect((gpointer) menu_latex_wizzard, "activate",
			 G_CALLBACK(wizard_activated), NULL);

	menu_latex_menu_special_char = gtk_menu_item_new_with_mnemonic(_("Insert _Special Character"));
	gtk_tooltips_set_tip(tooltips, menu_latex_menu_special_char,
			     _("Helps to use some not very common letters and signs"), NULL);
	gtk_container_add(GTK_CONTAINER(menu_latex_menu), menu_latex_menu_special_char);

	menu_latex_menu_special_char_submenu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_latex_menu_special_char), menu_latex_menu_special_char_submenu);
	sub_menu_init(menu_latex_menu_special_char_submenu, char_array, cat_names, char_insert_activated);

	menu_latex_ref = gtk_menu_item_new_with_mnemonic(_("Insert _Reference"));
	gtk_tooltips_set_tip(tooltips, menu_latex_ref,
		_("Inserting references to the document"), NULL);
	gtk_container_add(GTK_CONTAINER(menu_latex_menu), menu_latex_ref);
	g_signal_connect((gpointer) menu_latex_ref, "activate", G_CALLBACK(insert_ref_activated), NULL);

	menu_latex_label = gtk_menu_item_new_with_mnemonic(_("Insert _Label"));
	gtk_tooltips_set_tip(tooltips, menu_latex_label,
	     _("Helps at inserting labels to a document"), NULL);
	gtk_container_add(GTK_CONTAINER(menu_latex_menu), menu_latex_label);
	g_signal_connect((gpointer) menu_latex_label, "activate", G_CALLBACK(insert_label_activated), NULL);

	menu_latex_bibtex = gtk_menu_item_new_with_mnemonic(_("BibTeX"));
	gtk_container_add(GTK_CONTAINER(menu_latex_menu), menu_latex_bibtex);

	menu_latex_bibtex_submenu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_latex_bibtex), menu_latex_bibtex_submenu);

	for (i = 0; i < N_TYPES; i++)
	{
		tmp = NULL;
		tmp = gtk_menu_item_new_with_mnemonic(_(label_types[i]));
		gtk_container_add(GTK_CONTAINER(menu_latex_bibtex_submenu), tmp);
		g_signal_connect((gpointer) tmp, "activate", G_CALLBACK(insert_bibtex_entry), GINT_TO_POINTER(i));
	}

	/* init keybindins */

	keybindings_set_item(plugin_key_group, LATEX_WIZZARD_KB, kbwizard,
		0, 0, "run_latex_wizard", _("Run LaTeX-Wizard"), menu_latex_wizzard);
	keybindings_set_item(plugin_key_group, LATEX_INSERT_LABEL_KB, kblabel_insert,
		0, 0, "insert_latex_label", _("Insert \\label"), menu_latex_label);
	keybindings_set_item(plugin_key_group, LATEX_INSERT_REF_KB, kbref_insert,
		0, 0, "insert_latex_ref", _("Insert \\ref"), menu_latex_ref);
/*	keybindings_set_item(plugin_key_group, LATEX_INSERT_BIBTEX_ENTRY_KB,
		kb_bibtex_entry_insert, 0, 0, "insert_latex_bibtex_entry", _("Add BiBTeX entry"),
		menu_latex_bibtex); */

	ui_add_document_sensitive(menu_latex_menu_special_char);
	ui_add_document_sensitive(menu_latex_ref);
	ui_add_document_sensitive(menu_latex_label);
	ui_add_document_sensitive(menu_latex_bibtex);

	gtk_widget_set_sensitive(menu_latex_wizzard, TRUE);
	gtk_widget_show_all(menu_latex);
	main_menu_item = menu_latex;
}

void
plugin_cleanup()
{
	gtk_widget_destroy(main_menu_item);
}
