/*
 *      speller.c - this file is part of Spellcheck, a Geany plugin
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

#include <string.h>
#include <ctype.h>
#include <enchant.h>

#include "plugindata.h"

#include "document.h"
#include "editor.h"
#include "msgwindow.h"
#include "utils.h"

#include "pluginmacros.h"

#include "speller.h"
#include "scplugin.h"



static EnchantBroker *speller_broker = NULL;
static EnchantDict *speller_dict = NULL;



static void dict_describe(const gchar* const lang, const gchar* const name,
						  const gchar* const desc, const gchar* const file, void *target)
{
	gchar **result = (gchar**) target;
	*result = g_strdup_printf("\"%s\" (%s)", lang, name);
}


gint speller_check_word(GeanyDocument *doc, gint line_number, const gchar *word,
					    gint start_pos, gint end_pos)
{
	gsize j;
	gsize n_suggs = 0;
	gchar **suggs;
	GString *str;

	g_return_val_if_fail(speller_dict != NULL, 0);

	/* ignore numbers or words starting with digits */
	if (isdigit(*word))
		return 0;

	/* early out if the word is spelled correctly */
	if (enchant_dict_check(speller_dict, word, -1) == 0)
		return 0;

	str = g_string_sized_new(256);
	suggs = enchant_dict_suggest(speller_dict, word, -1, &n_suggs);

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
			enchant_dict_free_string_list(speller_dict, suggs);
	}
	g_string_free(str, TRUE);

	return n_suggs;
}


gint speller_process_line(GeanyDocument *doc, gint line_number, const gchar *line)
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

		suggestions_found += speller_check_word(doc, line_number, str->str, wstart, wend);

		pos_start = wend + 1;
	}

	g_string_free(str, TRUE);
	return suggestions_found;
}


void speller_check_document(GeanyDocument *doc)
{
	gchar *line;
	gint i;
	gint first_line, last_line;
	gchar *dict_string = NULL;
	gint suggestions_found = 0;

	g_return_if_fail(speller_dict != NULL);

	enchant_dict_describe(speller_dict, dict_describe, &dict_string);

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

		suggestions_found += speller_process_line(doc, i, line);

		g_free(line);
	}

	if (suggestions_found == 0 && sc->use_msgwin)
		p_msgwindow->msg_add(COLOR_BLUE, -1, NULL, _("The checked text is spelled correctly."));
}


static void broker_init_failed(void)
{
	const gchar *err = enchant_broker_get_error(speller_broker);
	p_dialogs->show_msgbox(GTK_MESSAGE_ERROR,
		_("The Enchant library couldn't be initialized (%s)."),
		(err != NULL) ? err : _("unknown error (maybe the chosen language is not available)"));
}


static void dict_compare(gpointer data, gpointer user_data)
{
	gboolean *supported = user_data;

	if (p_utils->str_equal(sc->default_language, data))
		*supported = TRUE;
}


static gboolean check_default_lang(void)
{
	gboolean supported = FALSE;

	g_ptr_array_foreach(sc->dicts, dict_compare, &supported);

	return supported;
}


void speller_reinit_enchant_dict(void)
{
	gchar *lang = sc->default_language;

	/* Release a previous dict object */
	if (speller_dict != NULL)
		enchant_broker_free_dict(speller_broker, speller_dict);

	/* Check if the stored default dictionary is (still) avaiable, fall back to the first
	 * one in the list if not */
	if (! check_default_lang())
	{
		if (sc->dicts->len > 0)
		{
			lang = g_ptr_array_index(sc->dicts, 0);
			g_warning("Stored language ('%s') could not be loaded. Falling back to '%s'",
				sc->default_language, lang);
		}
		else
			g_warning("Stored language ('%s') could not be loaded.", sc->default_language);
	}

	/* Request new dict object */
	speller_dict = enchant_broker_request_dict(speller_broker, lang);
	if (speller_dict == NULL)
	{
		broker_init_failed();
		gtk_widget_set_sensitive(sc->menu_item, FALSE);
	}
	else
	{
		gtk_widget_set_sensitive(sc->menu_item, TRUE);
	}
}


const gchar *speller_get_default_lang(void)
{
	const gchar *lang = g_getenv("LANG");
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


static gint sort_dicts(gconstpointer a, gconstpointer b)
{	/* casting mania ;-) */
	return strcmp((gchar*)((GPtrArray*)a)->pdata, (gchar*)((GPtrArray*)b)->pdata);
}


static void create_dicts_array(void)
{
	sc->dicts = g_ptr_array_new();

	enchant_broker_list_dicts(speller_broker, add_dict_array, sc->dicts);

	g_ptr_array_sort(sc->dicts, sort_dicts);
}


void speller_dict_free_string_list(gchar **tmp_suggs)
{
	g_return_if_fail(speller_dict != NULL);

	enchant_dict_free_string_list(speller_dict, tmp_suggs);
}


void speller_add_word(const gchar *word)
{
	enchant_dict_add_to_pwl(speller_dict, clickinfo.word, -1);

}

gboolean speller_dict_check(const gchar *word)
{
	g_return_val_if_fail(speller_dict != NULL, FALSE);

	return enchant_dict_check(speller_dict, word, -1);
}


gchar **speller_dict_suggest(const gchar *word, gsize *n_suggs)
{
	g_return_val_if_fail(speller_dict != NULL, NULL);

	return enchant_dict_suggest(speller_dict, word, -1, n_suggs);
}


void speller_init(void)
{
	speller_broker = enchant_broker_init();

	create_dicts_array();

	speller_reinit_enchant_dict();
}


void speller_free(void)
{
	if (speller_dict != NULL)
		enchant_broker_free_dict(speller_broker, speller_dict);
	enchant_broker_free(speller_broker);
}
