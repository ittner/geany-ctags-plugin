/*
 *      spellcheck.c - this file is part of Geany, a fast and lightweight IDE
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


#include "geany.h"
#include "support.h"

#if HAVE_LOCALE_H
# include <locale.h>
#endif

#ifdef G_OS_WIN32
# include <windows.h>
#endif

#include <aspell.h>

#include "plugindata.h"

#include "document.h"
#include "editor.h"
#include "msgwindow.h"
#include "keybindings.h"
#include "utils.h"

#include "pluginmacros.h"


PluginFields	*plugin_fields;
GeanyData		*geany_data;
GeanyFunctions *geany_functions;


PLUGIN_VERSION_CHECK(58)
PLUGIN_INFO(_("Spell Check"), _("Checks the spelling of the current document."), VERSION,
			_("The Geany developer team"))


/* Keybinding(s) */
enum
{
	KB_SPELL_CHECK,
	KB_COUNT
};
PLUGIN_KEY_GROUP(spellcheck, KB_COUNT)

static gchar *config_file;
static gchar *language;


/* On Windows we need to find the Aspell installation prefix via the Windows Registry
 * and then set the prefix in the Aspell config object. */
static void set_up_aspell_prefix(AspellConfig *config)
{
#ifdef G_OS_WIN32
	char sTemp[1024];
	HKEY hkey;
	DWORD len = sizeof(sTemp);
	
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Aspell"), 0,
			KEY_QUERY_VALUE, &hkey) != ERROR_SUCCESS)
		return;
	
	if (RegQueryValueEx(hkey, NULL, 0, NULL, (LPBYTE)sTemp, &len) == ERROR_SUCCESS)
		aspell_config_replace(config, "prefix", sTemp);

	RegCloseKey(hkey);
#endif
}


static void print_word_list(AspellSpeller *speller, GString *str, const AspellWordList *wl)
{
	if (wl == NULL)
	{
		g_string_append_c(str, '?');
	}
	else
	{
		AspellStringEnumeration *els = aspell_word_list_elements(wl);
		const char *word;

		while ((word = aspell_string_enumeration_next(els)) != 0)
		{
			g_string_append(str, word);
			g_string_append_c(str, ' ');
		}
		delete_aspell_string_enumeration(els);
	}
}


static gint check_document(AspellSpeller *speller, gint idx)
{
	AspellCanHaveError *ret;
	AspellDocumentChecker *checker;
	AspellToken token;
	gchar *word_begin;
	gchar *line;
	gint linewidth;
	gint i;
	gint first_line, last_line;
	gboolean suggestions_found = FALSE;
	GString *str = g_string_sized_new(1024);

	if (! DOC_IDX_VALID(idx))
	return 1;

	str = g_string_sized_new(1024);
	if (p_sci->can_copy(doc_list[idx].sci))
	{
		first_line = p_sci->get_line_from_position(
			doc_list[idx].sci, p_sci->get_selection_start(doc_list[idx].sci));
		last_line = p_sci->get_line_from_position(
			doc_list[idx].sci, p_sci->get_selection_end(doc_list[idx].sci));

		p_msgwindow->msg_add(COLOR_BLUE, -1, -1,
			_("Checking file \"%s\" (lines %d to %d):"),
			DOC_FILENAME(idx), first_line + 1, last_line + 1);
	}
	else
	{
		first_line = 0;
		last_line = p_sci->get_line_count(doc_list[idx].sci);
		p_msgwindow->msg_add(COLOR_BLUE, -1, -1, _("Checking file \"%s\":"),
			DOC_FILENAME(idx));
	}

	/* Set up the document checker */
	ret = new_aspell_document_checker(speller);
	if (aspell_error(ret) != 0)
	{
		g_warning("spellcheck: %s", aspell_error_message(ret));
		return 4;
	}

	checker = to_aspell_document_checker(ret);

	for (i = first_line; i < last_line; i++)
	{
		line = p_sci->get_line(doc_list[idx].sci, i);
		linewidth = strlen(line);

		/* First process the line */
		aspell_document_checker_process(checker, line, -1);

		/* Now find the misspellings in the line */
		while (token = aspell_document_checker_next_misspelling(checker), token.len != 0)
		{
			p_editor->set_indicator_on_line(idx, i);
			/* Print out the misspelling and possible replacements */
			word_begin = line + token.offset;

			g_string_append_printf (str, "line %d: ",  i + 1);
			g_string_append_len(str, word_begin, token.len);

			g_string_append(str, " | ");
			g_string_append(str, _("Try: "));
			print_word_list(speller, str, aspell_speller_suggest(speller, word_begin, token.len));

			p_msgwindow->msg_add(COLOR_RED, i + 1, idx, "%s", str->str);
			g_string_erase(str, 0, str->len);
			suggestions_found = TRUE;
		}

		g_free(line);
	}

	delete_aspell_document_checker(checker);

	g_string_free(str, TRUE);

	if (! suggestions_found)
		p_msgwindow->msg_add(COLOR_BLUE, -1, -1, _("The checked text is spelled correctly."));

	return 0;
}


static void perform_check(gint idx)
{
	AspellCanHaveError *ret;
	AspellSpeller *speller;
	AspellConfig *config;

	if (! DOC_IDX_VALID(idx))
		return;

	p_editor->clear_indicators(idx);
	p_msgwindow->clear_tab(MSG_MESSAGE);
	p_msgwindow->switch_tab(MSG_MESSAGE, FALSE);

	config = new_aspell_config();
	aspell_config_replace(config, "lang", language);
	aspell_config_replace(config, "encoding", "utf-8");
	set_up_aspell_prefix(config);

	ret = new_aspell_speller(config);
	delete_aspell_config(config);
	if (aspell_error(ret) != 0)
	{
		delete_aspell_can_have_error(ret);
		return;
	}

	speller = to_aspell_speller(ret);
	check_document(speller, idx);
	delete_aspell_speller(speller);
}


static void
item_activate(GtkMenuItem *menuitem, gpointer gdata)
{
	gint idx = p_document->get_cur_idx();

	perform_check(idx);
}


static void kb_activate(guint key_id)
{
	item_activate(NULL, NULL);
}


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


static const gchar *get_default_lang(void)
{
	const gchar *lang = g_getenv("LANG");
	if (lang != NULL)
	{
		if (g_strncasecmp(lang, "C", 1) == 0)
			lang = "en";
		else if (lang[0] == 0)
			lang = "en";
	}
	else
		lang = "en";
	
	return lang;
}


static void fill_dicts_combo(GtkComboBox *combo)
{
	AspellConfig *config;
	AspellDictInfoList *dlist;
	AspellDictInfoEnumeration *dels;
	const AspellDictInfo *entry;
	guint i = 0;

	config = new_aspell_config();
	set_up_aspell_prefix(config);
	dlist = get_aspell_dict_info_list(config);
	delete_aspell_config(config);

	dels = aspell_dict_info_list_elements(dlist);
	while ((entry = aspell_dict_info_enumeration_next(dels)) != 0)
	{
		gtk_combo_box_append_text(combo, entry->name);

		if (p_utils->str_equal(entry->name, language))
			gtk_combo_box_set_active(GTK_COMBO_BOX(combo), i);
		i++;
	}
	delete_aspell_dict_info_enumeration(dels);
}


void init(GeanyData *data)
{
	GtkWidget *sp_item;
	GKeyFile *config = g_key_file_new();

	config_file = g_strconcat(app->configdir, G_DIR_SEPARATOR_S, "plugins", G_DIR_SEPARATOR_S,
		"spellcheck", G_DIR_SEPARATOR_S, "spellcheck.conf", NULL);

	g_key_file_load_from_file(config, config_file, G_KEY_FILE_NONE, NULL);
	language = p_utils->get_setting_string(config, "spellcheck", "language", get_default_lang());
	g_key_file_free(config);

	locale_init();

	sp_item = gtk_menu_item_new_with_mnemonic(_("_Spell Check"));
	gtk_widget_show(sp_item);
	gtk_container_add(GTK_CONTAINER(geany_data->tools_menu), sp_item);
	g_signal_connect(G_OBJECT(sp_item), "activate", G_CALLBACK(item_activate), NULL);

	plugin_fields->menu_item = sp_item;
	plugin_fields->flags = PLUGIN_IS_DOCUMENT_SENSITIVE;

	/* setup keybindings */
	p_keybindings->set_item(plugin_key_group, KB_SPELL_CHECK, kb_activate,
		0, 0, "spell_check", _("Run Spell Check"), NULL);
}


void configure(GtkWidget *parent)
{
	GtkWidget *dialog, *label, *vbox, *combo;

	dialog = gtk_dialog_new_with_buttons(_("Spell Check"),
		GTK_WINDOW(parent), GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
	vbox = p_ui->dialog_vbox_new(GTK_DIALOG(dialog));
	gtk_widget_set_name(dialog, "GeanyDialog");
	gtk_box_set_spacing(GTK_BOX(vbox), 6);

	label = gtk_label_new(_("Language to use for the spell check:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	combo = gtk_combo_box_new_text();
	fill_dicts_combo(GTK_COMBO_BOX(combo));

	gtk_combo_box_set_wrap_width(GTK_COMBO_BOX(combo), 3);
	gtk_box_pack_start(GTK_BOX(vbox), combo, FALSE, FALSE, 0);

	gtk_widget_show_all(vbox);

	/* run the dialog and check for the response code */
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		GKeyFile *config = g_key_file_new();
		gchar *data;
		gchar *config_dir = g_path_get_dirname(config_file);

		setptr(language, gtk_combo_box_get_active_text(GTK_COMBO_BOX(combo)));

		g_key_file_load_from_file(config, config_file, G_KEY_FILE_NONE, NULL);
		g_key_file_set_string(config, "spellcheck", "language", language);

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
	g_free(language);
	g_free(config_file);
	gtk_widget_destroy(plugin_fields->menu_item);
}
