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

#ifdef HAVE_LOCALE_H
# include <locale.h>
#endif

#ifdef G_OS_WIN32
# include <windows.h>
#endif

#include <string.h>
#include <ctype.h>
#include <enchant.h>

#include "plugindata.h"

#include "document.h"
#include "editor.h"
#include "msgwindow.h"
#include "keybindings.h"
#include "utils.h"
#include "ui_utils.h"

#include "pluginmacros.h"


GeanyPlugin		*geany_plugin;
GeanyData		*geany_data;
GeanyFunctions	*geany_functions;


PLUGIN_VERSION_CHECK(100)
PLUGIN_SET_INFO(_("Spell Check"), _("Checks the spelling of the current document."), "0.2",
			_("The Geany developer team"))


typedef struct
{
	gchar *config_file;
	gchar *default_language;
	gboolean use_msgwin;
	gboolean check_while_typing;
	gboolean show_toolbar_item;
	gulong signal_id;
	GPtrArray *dicts;
	GtkWidget *menu_item;
	GtkWidget *edit_menu;
	GtkWidget *edit_menu_sep;
	GtkWidget *edit_menu_sub;
	GtkToolItem *toolbar_button;
	EnchantBroker *broker;
	EnchantDict *dict;
} SpellCheck;
static SpellCheck *sc;


#define MAX_MENU_SUGGESTIONS 10
typedef struct
{
	gint pos;
	GeanyDocument *doc;
	/* static array to keep suggestions for use as callback user data for the editing menu items */
	gchar *suggs[MAX_MENU_SUGGESTIONS];
	/* static storage for the misspelled word under the cursor when using the editing menu */
	gchar *word;
} SpellClickInfo;
static SpellClickInfo clickinfo;


/* Flag to indicate that a callback function will be triggered by generating the appropiate event
 * but the callback should be ignored. */
static gboolean ignore_sc_callback = FALSE;


static void on_update_editor_menu(GObject *obj, const gchar *word, gint pos,
								  GeanyDocument *doc, gpointer user_data);

/* Keybinding(s) */
enum
{
	KB_SPELL_CHECK,
	KB_COUNT
};
PLUGIN_KEY_GROUP(spellcheck, KB_COUNT)



PluginCallback plugin_callbacks[] =
{
    { "update-editor-menu", (GCallback) &on_update_editor_menu, FALSE, NULL },
    { NULL, NULL, FALSE, NULL }
};


/* currently unused */
#ifdef G_OS_WIN32
#warning TODO check Windows support
/* On Windows we need to find the Aspell installation prefix via the Windows Registry
 * and then set the prefix in the Aspell config object. */
static void set_up_aspell_prefix(AspellConfig *config)
{
	char sTemp[1024];
	HKEY hkey;
	DWORD len = sizeof(sTemp);

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Aspell"), 0,
			KEY_QUERY_VALUE, &hkey) != ERROR_SUCCESS)
		return;

	if (RegQueryValueEx(hkey, NULL, 0, NULL, (LPBYTE)sTemp, &len) == ERROR_SUCCESS)
		aspell_config_replace(config, "prefix", sTemp);

	RegCloseKey(hkey);
}
#endif


static void toolbar_item_toggled_cb(GtkToggleToolButton *button, gpointer user_data)
{
	if (ignore_sc_callback)
		return;

	sc->check_while_typing = gtk_toggle_tool_button_get_active(button);

	p_ui->set_statusbar(FALSE, _("Spell checking while typing is now %s"),
		(sc->check_while_typing) ? _("enabled") : _("disabled"));
}


static void toolbar_update(void)
{
	/* toolbar item is not requested, so remove the item if it exists */
	if (! sc->show_toolbar_item)
	{
		if (sc->toolbar_button != NULL)
		{
			gtk_widget_destroy(GTK_WIDGET(sc->toolbar_button));
			sc->toolbar_button = NULL;
		}
	}
	else
	{
		if (sc->toolbar_button == NULL)
		{
			sc->toolbar_button = gtk_toggle_tool_button_new_from_stock("gtk-spell-check");
	#if GTK_CHECK_VERSION(2, 12, 0)
			gtk_widget_set_tooltip_text(GTK_WIDGET(sc->toolbar_button),
				_("Toggle spell check while typing."));
	#endif
			gtk_widget_show(GTK_WIDGET(sc->toolbar_button));
			p_plugin->add_toolbar_item(geany_plugin, sc->toolbar_button);
			p_ui->add_document_sensitive(GTK_WIDGET(sc->toolbar_button));

			g_signal_connect(sc->toolbar_button, "toggled",
				G_CALLBACK(toolbar_item_toggled_cb), NULL);
		}

		ignore_sc_callback = TRUE;
		gtk_toggle_tool_button_set_active(
			GTK_TOGGLE_TOOL_BUTTON(sc->toolbar_button), sc->check_while_typing);
		ignore_sc_callback = FALSE;
	}
}


static void clear_indicators_on_range(GeanyDocument *doc, gint start, gint len)
{
	g_return_if_fail(doc != NULL);

	if (len > 0)
	{
		p_sci->indicator_clear(doc->editor->sci, start, start + len);
	}
}


static void clear_indicators_on_line(GeanyDocument *doc, gint line_number)
{
	gint start_pos, length;

	g_return_if_fail(doc != NULL);

	start_pos = p_sci->get_position_from_line(doc->editor->sci, line_number);
	length = p_sci->get_line_length(doc->editor->sci, line_number);

	clear_indicators_on_range(doc, start_pos, length);
}



static void on_menu_suggestion_item_activate(GtkMenuItem *menuitem, gpointer gdata)
{
	gchar *sugg = gdata;
	gint startword, endword;

	if (clickinfo.doc == NULL || clickinfo.pos == -1)
	{
		g_free(sugg);
		return;
	}

	startword = p_sci->send_message(
		clickinfo.doc->editor->sci, SCI_WORDSTARTPOSITION, clickinfo.pos, 0);
	endword = p_sci->send_message(
		clickinfo.doc->editor->sci, SCI_WORDENDPOSITION, clickinfo.pos, 0);

	if (startword != endword)
	{
		p_sci->set_selection_start(clickinfo.doc->editor->sci, startword);
		p_sci->set_selection_end(clickinfo.doc->editor->sci, endword);
		p_sci->replace_sel(clickinfo.doc->editor->sci, sugg);

		clear_indicators_on_range(clickinfo.doc, startword, endword - startword);
	}
}


static void on_menu_addword_item_activate(GtkMenuItem *menuitem, gpointer gdata)
{
	gint startword, endword;

	if (clickinfo.doc == NULL || clickinfo.word == NULL || clickinfo.pos == -1)
		return;

	enchant_dict_add_to_pwl(sc->dict, clickinfo.word, -1);

	startword = p_sci->send_message(
		clickinfo.doc->editor->sci, SCI_WORDSTARTPOSITION, clickinfo.pos, 0);
	endword = p_sci->send_message(
		clickinfo.doc->editor->sci, SCI_WORDENDPOSITION, clickinfo.pos, 0);
	if (startword != endword)
	{
		clear_indicators_on_range(clickinfo.doc, startword, endword - startword);
	}
}


static void on_update_editor_menu(GObject *obj, const gchar *word, gint pos,
								  GeanyDocument *doc, gpointer user_data)
{
	gsize n_suggs, i;
	gchar **tmp_suggs;

	g_return_if_fail(doc != NULL && doc->is_valid);

	/* hide the submenu in any case, we will reshow it again if we actually found something */
	gtk_widget_hide(sc->edit_menu);
	gtk_widget_hide(sc->edit_menu_sep);

	if (! NZV(word) || enchant_dict_check(sc->dict, word, -1) == 0)
		return;

	tmp_suggs = enchant_dict_suggest(sc->dict, word, -1, &n_suggs);

	if (tmp_suggs != NULL)
	{
		GtkWidget *menu_item, *image;
		gchar *label;

		clickinfo.pos = pos;
		clickinfo.doc = doc;
		setptr(clickinfo.word, g_strdup(word));

		if (GTK_IS_WIDGET(sc->edit_menu_sub))
			gtk_widget_destroy(sc->edit_menu_sub);

		sc->edit_menu_sub = gtk_menu_new();
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(sc->edit_menu), sc->edit_menu_sub);

		/* TODO do we need more than 10 suggestions? gtkspell offers additional suggestions
		 * in another sub menu, should we too? */
		for (i = 0; i < MIN(n_suggs, MAX_MENU_SUGGESTIONS); i++)
		{
			/* keep the suggestions in a static array for the callback function */
			g_free(clickinfo.suggs[i]);
			clickinfo.suggs[i] = g_strdup(tmp_suggs[i]);

			menu_item = gtk_menu_item_new_with_label(clickinfo.suggs[i]);
			gtk_container_add(GTK_CONTAINER(sc->edit_menu_sub), menu_item);
			g_signal_connect((gpointer) menu_item, "activate",
				G_CALLBACK(on_menu_suggestion_item_activate), clickinfo.suggs[i]);
		}
		menu_item = gtk_separator_menu_item_new();
		gtk_container_add(GTK_CONTAINER(sc->edit_menu_sub), menu_item);

		image = gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU);

		label = g_strdup_printf(_("Add \"%s\" to Dictionary"), word);
		menu_item = gtk_image_menu_item_new_with_label(label);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item), image);
		gtk_container_add(GTK_CONTAINER(sc->edit_menu_sub), menu_item);
		g_signal_connect((gpointer) menu_item, "activate",
			G_CALLBACK(on_menu_addword_item_activate), NULL);

		gtk_widget_show(sc->edit_menu);
		gtk_widget_show(sc->edit_menu_sep);
		gtk_widget_show_all(sc->edit_menu_sub);

		enchant_dict_free_string_list(sc->dict, tmp_suggs);
		g_free(label);
	}
}


static void dict_describe(const gchar* const lang, const gchar* const name,
						  const gchar* const desc, const gchar* const file, void *target)
{
	gchar **result = (gchar**) target;
	*result = g_strdup_printf("\"%s\" (%s)", lang, name);
}


static gint check_word(GeanyDocument *doc, gint line_number, const gchar *word,
					   gint start_pos, gint end_pos)
{
	gsize j;
	gsize n_suggs = 0;
	gchar **suggs;
	GString *str = g_string_sized_new(256);

	/* ignore numbers or words starting with digits */
	if (isdigit(*word))
		return 0;

	/* early out if the word is spelled correctly */
	if (enchant_dict_check(sc->dict, word, -1) == 0)
		return 0;

	suggs = enchant_dict_suggest(sc->dict, word, -1, &n_suggs);

	if (suggs != NULL)
	{
		g_string_append_printf(str, "line %d: %s | ",  line_number + 1, word);

		g_string_append(str, _("Try: "));

		/* Now find the misspellings in the line, limit suggestions to a maximum of 15 (for now) */
		for (j = 0; j < MIN(n_suggs, 15); j++)
		{
			g_string_append(str, suggs[j]);
			g_string_append_c(str, ' ');
		}

		if (start_pos == -1)
			start_pos = end_pos - strlen(word);

		p_editor->set_indicator(doc->editor, start_pos, end_pos);
		if (sc->use_msgwin)
			p_msgwindow->msg_add(COLOR_RED, line_number + 1, doc, "%s", str->str);

		if (suggs != NULL && n_suggs)
			enchant_dict_free_string_list(sc->dict, suggs);
	}
	g_string_free(str, TRUE);

	return n_suggs;
}


static gint process_line(GeanyDocument *doc, gint line_number, const gchar *line)
{
	gint pos_start, pos_end;
	gint wstart, wend;
	GString *str = g_string_sized_new(256);
	gint suggestions_found = 0;
	gchar c;

	pos_start = p_sci->get_position_from_line(doc->editor->sci, line_number);
	/* TODO use SCI_GETLINEENDPOSITION */
	pos_end = p_sci->get_position_from_line(doc->editor->sci, line_number + 1);

	while (pos_start < pos_end)
	{
		wstart = p_sci->send_message(doc->editor->sci, SCI_WORDSTARTPOSITION, pos_start, TRUE);
		wend = p_sci->send_message(doc->editor->sci, SCI_WORDENDPOSITION, wstart, FALSE);
		if (wstart == wend)
			break;
		c = p_sci->get_char_at(doc->editor->sci, wstart);
		/* hopefully it's enough to check for these both */
		if (ispunct(c) || isspace(c))
		{
			pos_start++;
			continue;
		}

		/* ensure the string has enough allocated memory */
		if (str->len < (guint)(wend - wstart))
			g_string_set_size(str, wend - wstart);

		p_sci->get_text_range(doc->editor->sci, wstart, wend, str->str);

		suggestions_found += check_word(doc, line_number, str->str, wstart, wend);

		pos_start = wend + 1;
	}

	g_string_free(str, TRUE);
	return suggestions_found;
}


static void check_document(GeanyDocument *doc)
{
	gchar *line;
	gint i;
	gint first_line, last_line;
	gchar *dict_string = NULL;
	gint suggestions_found = 0;

	enchant_dict_describe(sc->dict, dict_describe, &dict_string);

	if (p_sci->has_selection(doc->editor->sci))
	{
		first_line = p_sci->get_line_from_position(
			doc->editor->sci, p_sci->get_selection_start(doc->editor->sci));
		last_line = p_sci->get_line_from_position(
			doc->editor->sci, p_sci->get_selection_end(doc->editor->sci));

		if (sc->use_msgwin)
			p_msgwindow->msg_add(COLOR_BLUE, -1, NULL,
				_("Checking file \"%s\" (lines %d to %d using %s):"),
				DOC_FILENAME(doc), first_line + 1, last_line + 1, dict_string);
		g_message("Checking file \"%s\" (lines %d to %d using %s):",
			DOC_FILENAME(doc), first_line + 1, last_line + 1, dict_string);
	}
	else
	{
		first_line = 0;
		last_line = p_sci->get_line_count(doc->editor->sci);
		if (sc->use_msgwin)
			p_msgwindow->msg_add(COLOR_BLUE, -1, NULL, _("Checking file \"%s\" (using %s):"),
				DOC_FILENAME(doc), dict_string);
		g_message("Checking file \"%s\" (using %s):", DOC_FILENAME(doc), dict_string);
	}
	g_free(dict_string);

	for (i = first_line; i < last_line; i++)
	{
		line = p_sci->get_line(doc->editor->sci, i);

		suggestions_found += process_line(doc, i, line);

		g_free(line);
	}

	if (suggestions_found == 0 && sc->use_msgwin)
		p_msgwindow->msg_add(COLOR_BLUE, -1, NULL, _("The checked text is spelled correctly."));
}


static void perform_check(GeanyDocument *doc)
{
	p_editor->clear_indicators(doc->editor);
	if (sc->use_msgwin)
	{
		p_msgwindow->clear_tab(MSG_MESSAGE);
		p_msgwindow->switch_tab(MSG_MESSAGE, FALSE);
	}

	check_document(doc);
}


/* Checks only the last word before the current cursor position -> check as you type. */
static gboolean on_key_release(GtkWidget *widget, GdkEventKey *ev, gpointer user_data)
{
	gint line_number;
	GString *str = g_string_sized_new(256);
	gchar *line;
	GeanyDocument *doc;
	static time_t time_prev = 0;
	time_t time_now = time(NULL);
	GtkWidget *focusw;

	if (! sc->check_while_typing)
		return FALSE;
	/* check only once a second */
	if (time_now == time_prev)
		return FALSE;
	/* set current time for the next key press */
	time_prev = time_now;

	doc = p_document->get_current();
	/* bail out if we don't have a document or if we are not in the editor widget */
	focusw = gtk_window_get_focus(GTK_WINDOW(geany->main_widgets->window));
	if (doc == NULL || focusw != GTK_WIDGET(doc->editor->sci))
		return FALSE;

	if (ev->keyval == '\r' &&
		p_sci->send_message(doc->editor->sci, SCI_GETEOLMODE, 0, 0) == SC_EOL_CRLF)
	{	/* prevent double line checking */
		return FALSE;
	}

	line_number = p_sci->get_current_line(doc->editor->sci);
	if (ev->keyval == '\n' || ev->keyval == '\r')
		line_number--; /* check previous line if we start a new one */
	line = p_sci->get_line(doc->editor->sci, line_number);

	clear_indicators_on_line(doc, line_number);
	if (process_line(doc, line_number, line) != 0)
	{
		if (sc->use_msgwin)
			p_msgwindow->switch_tab(MSG_MESSAGE, FALSE);
	}

	g_string_free(str, TRUE);
	g_free(line);

	return FALSE;
}


static void broker_init_failed()
{
	const gchar *err = enchant_broker_get_error(sc->broker);
	p_dialogs->show_msgbox(GTK_MESSAGE_ERROR,
		_("The Enchant library couldn't be initialized (%s)."),
		(err != NULL) ? err : _("unknown error (maybe the chosen language is not available)"));
}


static void init_enchant_dict()
{
	/* Release a previous dict object */
	if (sc->dict != NULL)
		enchant_broker_free_dict(sc->broker, sc->dict);

	/* Request new dict object */
	sc->dict = enchant_broker_request_dict(sc->broker, sc->default_language);
	if (sc->dict == NULL)
	{
		broker_init_failed();
		gtk_widget_set_sensitive(sc->menu_item, FALSE);
	}
	else
	{
		gtk_widget_set_sensitive(sc->menu_item, TRUE);
	}
}


static void on_menu_item_activate(GtkMenuItem *menuitem, gpointer gdata)
{
	GeanyDocument *doc;

	if (sc->dict == NULL)
		return;

	doc = p_document->get_current();

	/* Another language was chosen from the menu item, so make it default for this session. */
    if (gdata != NULL)
		setptr(sc->default_language, g_strdup(gdata));

	init_enchant_dict();

	perform_check(doc);
}


static void kb_activate(guint key_id)
{
	on_menu_item_activate(NULL, NULL);
}


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
	locale_dir = g_strconcat(install_dir, "\\share\\locale", NULL);
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
	/** TODO check whether the returned lang is actually provided by enchant and
	 *  choose something else if not
	 *  N.B. this is not really possible/reasonable with enchant < 1.4.3 because of a
	 *  bug in the Zemberek provider which always reports it has the requested dict */
	if (NZV(lang))
	{
		if (g_ascii_strncasecmp(lang, "C", 1) == 0)
			lang = "en";
	}
	else
		lang = "en";

	return lang;
}


static void add_dict_array(const gchar* const lang_tag, const gchar* const provider_name,
						   const gchar* const provider_desc, const gchar* const provider_file,
						   gpointer user_data)
{
	guint i;
	gchar *result = g_strdup(lang_tag);

	/* sometimes dictionaries are named lang-LOCALE instead of lang_LOCALE, so replace the
	 * hyphen by a dash, enchant seems to not care about it. */
	for (i = 0; i < strlen(result); i++)
	{
		if (result[i] == '-')
			result[i] = '_';
	}

	/* find duplicates and skip them */
	for (i = 0; i < sc->dicts->len; i++)
	{
		if (p_utils->str_equal(g_ptr_array_index(sc->dicts, i), result))
			return;
	}

	g_ptr_array_add(sc->dicts, result);
}


static void on_configure_response(GtkDialog *dialog, gint response, gpointer user_data)
{
	if (response == GTK_RESPONSE_OK || response == GTK_RESPONSE_APPLY)
	{
		GKeyFile *config = g_key_file_new();
		gchar *data;
		gchar *config_dir = g_path_get_dirname(sc->config_file);

		setptr(sc->default_language, gtk_combo_box_get_active_text(GTK_COMBO_BOX(
			g_object_get_data(G_OBJECT(dialog), "combo"))));
		init_enchant_dict();

		sc->check_while_typing = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
			g_object_get_data(G_OBJECT(dialog), "check_type"))));

		sc->use_msgwin = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
			g_object_get_data(G_OBJECT(dialog), "check_msgwin"))));

		sc->show_toolbar_item = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
			g_object_get_data(G_OBJECT(dialog), "check_toolbar"))));

		g_key_file_load_from_file(config, sc->config_file, G_KEY_FILE_NONE, NULL);
		if (sc->default_language != NULL) /* lang may be NULL */
			g_key_file_set_string(config, "spellcheck", "language", sc->default_language);
		g_key_file_set_boolean(config, "spellcheck", "check_while_typing", sc->check_while_typing);
		g_key_file_set_boolean(config, "spellcheck", "use_msgwin", sc->use_msgwin);
		g_key_file_set_boolean(config, "spellcheck", "show_toolbar_item", sc->show_toolbar_item);

		toolbar_update();

		if (! g_file_test(config_dir, G_FILE_TEST_IS_DIR) && p_utils->mkdir(config_dir, TRUE) != 0)
		{
			p_dialogs->show_msgbox(GTK_MESSAGE_ERROR,
				_("Plugin configuration directory could not be created."));
		}
		else
		{
			/* write config to file */
			data = g_key_file_to_data(config, NULL, NULL);
			p_utils->write_file(sc->config_file, data);
			g_free(data);
		}
		g_free(config_dir);
		g_key_file_free(config);
	}
}


static gint sort_dicts(gconstpointer a, gconstpointer b)
{	/* casting mania ;-) */
	return strcmp((gchar*)((GPtrArray*)a)->pdata, (gchar*)((GPtrArray*)b)->pdata);
}


static void create_dicts_array()
{
	sc->dicts = g_ptr_array_new();

	enchant_broker_list_dicts(sc->broker, add_dict_array, sc->dicts);

	g_ptr_array_sort(sc->dicts, sort_dicts);
}


static void create_edit_menu()
{
	sc->edit_menu = gtk_image_menu_item_new_from_stock(GTK_STOCK_SPELL_CHECK, NULL);
	gtk_container_add(GTK_CONTAINER(geany->main_widgets->editor_menu), sc->edit_menu);
	gtk_menu_reorder_child(GTK_MENU(geany->main_widgets->editor_menu), sc->edit_menu, 0);

	sc->edit_menu_sep = gtk_separator_menu_item_new();
	gtk_container_add(GTK_CONTAINER(geany->main_widgets->editor_menu), sc->edit_menu_sep);
	gtk_menu_reorder_child(GTK_MENU(geany->main_widgets->editor_menu), sc->edit_menu_sep, 1);

	gtk_widget_show_all(sc->edit_menu);
}


static GtkWidget *create_menu(GtkWidget *sp_item)
{
	GtkWidget *menu, *subitem;
	guint i;

	gtk_container_add(GTK_CONTAINER(geany->main_widgets->tools_menu), sp_item);

	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(sp_item), menu);

	subitem = gtk_menu_item_new_with_mnemonic(_("Default"));
	gtk_container_add(GTK_CONTAINER(menu), subitem);
	g_signal_connect((gpointer) subitem, "activate", G_CALLBACK(on_menu_item_activate), NULL);

	subitem = gtk_separator_menu_item_new();
	gtk_container_add(GTK_CONTAINER(menu), subitem);

	for (i = 0; i < sc->dicts->len; i++)
	{
		GtkWidget *menu_item;

		menu_item = gtk_menu_item_new_with_label(g_ptr_array_index(sc->dicts, i));
		gtk_container_add(GTK_CONTAINER(menu), menu_item);
		g_signal_connect((gpointer) menu_item, "activate",
			G_CALLBACK(on_menu_item_activate), g_ptr_array_index(sc->dicts, i));
	}

	return sp_item;
}


void plugin_init(GeanyData *data)
{
	GtkWidget *sp_item;
	GKeyFile *config = g_key_file_new();
	guint i;

	sc = g_new0(SpellCheck, 1);

	sc->config_file = g_strconcat(geany->app->configdir, G_DIR_SEPARATOR_S, "plugins", G_DIR_SEPARATOR_S,
		"spellcheck", G_DIR_SEPARATOR_S, "spellcheck.conf", NULL);

	g_key_file_load_from_file(config, sc->config_file, G_KEY_FILE_NONE, NULL);
	sc->default_language = p_utils->get_setting_string(config,
		"spellcheck", "language", get_default_lang());
	sc->check_while_typing = p_utils->get_setting_boolean(config,
		"spellcheck", "check_while_typing", FALSE);
	sc->show_toolbar_item = p_utils->get_setting_boolean(config,
		"spellcheck", "show_toolbar_item", TRUE);
	sc->use_msgwin = p_utils->get_setting_boolean(config, "spellcheck", "use_msgwin", FALSE);
	g_key_file_free(config);

	locale_init();

	sc->menu_item = gtk_image_menu_item_new_from_stock("gtk-spell-check", NULL);
	p_ui->add_document_sensitive(sc->menu_item);

	toolbar_update();

	sc->broker = enchant_broker_init();
	init_enchant_dict();

	for (i = 0; i < MAX_MENU_SUGGESTIONS; i++)
	{
		clickinfo.suggs[i] = NULL;
	}
	clickinfo.word = NULL;

	create_dicts_array();

	create_edit_menu();
	sp_item = create_menu(sc->menu_item);
	gtk_widget_show_all(sp_item);

	sc->signal_id = g_signal_connect(geany->main_widgets->window,
		"key-release-event", G_CALLBACK(on_key_release), NULL);

	/* setup keybindings */
	p_keybindings->set_item(plugin_key_group, KB_SPELL_CHECK, kb_activate,
		0, 0, "spell_check", _("Run Spell Check"), NULL);
}


GtkWidget *plugin_configure(GtkDialog *dialog)
{
	GtkWidget *label, *vbox, *combo, *check_type, *check_msgwin, *check_toolbar;
	guint i;

	vbox = gtk_vbox_new(FALSE, 6);

	check_type = gtk_check_button_new_with_label(_("Check spelling while typing"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_type), sc->check_while_typing);
	gtk_box_pack_start(GTK_BOX(vbox), check_type, FALSE, FALSE, 6);

	check_toolbar = gtk_check_button_new_with_label(_("Show toolbar item to toggle spell checking"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_toolbar), sc->show_toolbar_item);
	gtk_box_pack_start(GTK_BOX(vbox), check_toolbar, FALSE, FALSE, 3);

	check_msgwin = gtk_check_button_new_with_label(
		_("Print misspelled words and suggestions in the messages window"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_msgwin), sc->use_msgwin);
	gtk_box_pack_start(GTK_BOX(vbox), check_msgwin, FALSE, FALSE, 3);

	label = gtk_label_new(_("Language to use for the spell check:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 3);

	combo = gtk_combo_box_new_text();

	for (i = 0; i < sc->dicts->len; i++)
	{
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), g_ptr_array_index(sc->dicts, i));

		if (p_utils->str_equal(g_ptr_array_index(sc->dicts, i), sc->default_language))
			gtk_combo_box_set_active(GTK_COMBO_BOX(combo), i);
	}
	/* if the default language couldn't be selected, select the first available language */
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(combo)) == -1)
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);

	if (sc->dicts->len > 20)
		gtk_combo_box_set_wrap_width(GTK_COMBO_BOX(combo), 3);
	else if (sc->dicts->len > 10)
		gtk_combo_box_set_wrap_width(GTK_COMBO_BOX(combo), 2);
	gtk_box_pack_start(GTK_BOX(vbox), combo, FALSE, FALSE, 6);

	g_object_set_data(G_OBJECT(dialog), "combo", combo);
	g_object_set_data(G_OBJECT(dialog), "check_type", check_type);
	g_object_set_data(G_OBJECT(dialog), "check_msgwin", check_msgwin);
	g_object_set_data(G_OBJECT(dialog), "check_toolbar", check_toolbar);
	g_signal_connect(dialog, "response", G_CALLBACK(on_configure_response), NULL);

	gtk_widget_show_all(vbox);

	return vbox;
}


void plugin_cleanup(void)
{
	guint i;
	for (i = 0; i < MAX_MENU_SUGGESTIONS; i++)
	{
		clickinfo.suggs[i] = NULL;
	}
	g_free(clickinfo.word);
	for (i = 0; i < sc->dicts->len; i++)
	{
		g_free(g_ptr_array_index(sc->dicts, i));
	}
	g_ptr_array_free(sc->dicts, TRUE);

	g_signal_handler_disconnect(geany->main_widgets->window, sc->signal_id);

	gtk_widget_destroy(sc->edit_menu);
	gtk_widget_destroy(sc->edit_menu_sep);
	if (sc->toolbar_button != NULL)
		gtk_widget_destroy(GTK_WIDGET(sc->toolbar_button));

	enchant_broker_free_dict(sc->broker, sc->dict);
	enchant_broker_free(sc->broker);

	g_free(sc->default_language);
	g_free(sc->config_file);
	gtk_widget_destroy(sc->menu_item);
	g_free(sc);
}
