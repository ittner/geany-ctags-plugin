/*
 *      geanyvc.c - Plugin to geany light IDE to work with vc
 *
 *      Copyright 2007, 2008 Frank Lanitz <frank(at)frank(dot)uvena(dot)de>
 *      Copyright 2007, 2008 Enrico Tröger <enrico.troeger@uvena.de>
 *      Copyright 2007 Nick Treleaven <nick.treleaven@btinternet.com>
 *      Copyright 2007, 2008 Yura Siamashka <yurand2@gmail.com>
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

/* VC plugin */
/* This plugin allow to works with cvs/svn/git inside geany light IDE. */

#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "geany.h"
#include "support.h"
#include "plugindata.h"
#include "document.h"
#include "editor.h"
#include "filetypes.h"
#include "utils.h"
#include "ui_utils.h"
#include "project.h"
#include "prefs.h"
#include "pluginmacros.h"

#ifdef HAVE_LOCALE_H
# include <locale.h>
#endif

#include "highlighting.h"
#include "SciLexer.h"

#include "geanyvc.h"

#include "config.h"

#ifdef USE_GTKSPELL
#include <gtkspell/gtkspell.h>
#endif

PluginFields *plugin_fields;
GeanyData *geany_data;
GeanyFunctions *geany_functions;


PLUGIN_VERSION_CHECK(89);
PLUGIN_SET_INFO(_("VC"), _("Interface to different Version Control systems."), VERSION,
		_("Yura Siamashka <yurand2@gmail.com>,\nFrank Lanitz <frank@frank.uvena.de>"));

/* Some global variables */
static gboolean set_changed_flag;
static gboolean set_add_confirmation;
static gboolean set_maximize_commit_dialog;
static gboolean set_external_diff;

static gchar *config_file;

static gboolean enable_cvs;
static gboolean enable_git;
static gboolean enable_svn;
static gboolean enable_svk;
static gboolean enable_bzr;
static gboolean enable_hg;

#ifdef USE_GTKSPELL
static gchar *lang;
#endif

static GSList *VC = NULL;

/* The addresses of these strings act as enums, their contents are not used. */
const gchar DIRNAME[] = "*DIRNAME*";
const gchar FILENAME[] = "*FILENAME*";
const gchar BASE_FILENAME[] = "*BASE_FILENAME*";
const gchar FILE_LIST[] = "*FILE_LIST*";
const gchar MESSAGE[] = "*MESSAGE*";

/* this string is used when action require to run several commands */
const gchar CMD_SEPARATOR[] = "*CMD-SEPARATOR*";
const gchar CMD_FUNCTION[] = "*CUSTOM_FUNCTION*";

/* commit status */
const gchar FILE_STATUS_MODIFIED[] = "Modified";
const gchar FILE_STATUS_ADDED[] = "Added";
const gchar FILE_STATUS_DELETED[] = "Deleted";
const gchar FILE_STATUS_UNKNOWN[] = "Unknown";

void *NO_ENV[] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

static void registrate();

GSList *
get_commit_files_null(G_GNUC_UNUSED const gchar * dir)
{
	return NULL;
}

static void
free_text_list(GSList * lst)
{
	GSList *tmp;
	if (!lst)
		return;
	for (tmp = lst; tmp != NULL; tmp = g_slist_next(tmp))
	{
		g_free((CommitItem *) (tmp->data));
	}
	g_slist_free(lst);
}

static void
free_commit_list(GSList * lst)
{
	GSList *tmp;
	if (!lst)
		return;
	for (tmp = lst; tmp != NULL; tmp = g_slist_next(tmp))
	{
		g_free(((CommitItem *) (tmp->data))->path);
		g_free((CommitItem *) (tmp->data));
	}
	g_slist_free(lst);
}

gchar *
find_subdir_path(const gchar * filename, const gchar * subdir)
{
	gboolean ret = FALSE;
	gchar *base;
	gchar *gitdir;
	gchar *base_prev = g_strdup(":");

	if (g_file_test(filename, G_FILE_TEST_IS_DIR))
		base = g_strdup(filename);
	else
		base = g_path_get_dirname(filename);

	while (strcmp(base, base_prev) != 0)
	{
		gitdir = g_build_filename(base, subdir, NULL);
		ret = g_file_test(gitdir, G_FILE_TEST_IS_DIR);
		g_free(gitdir);
		if (ret)
			break;
		g_free(base_prev);
		base_prev = base;
		base = g_path_get_dirname(base);
	}

	g_free(base_prev);
	if (ret)
		return base;
	g_free(base);
	return NULL;
}

static gboolean
find_subdir(const gchar * filename, const gchar * subdir)
{
	gchar *basedir;
	basedir = find_subdir_path(filename, subdir);
	if (basedir)
	{
		g_free(basedir);
		return TRUE;
	}
	return FALSE;
}

gboolean
find_dir(const gchar * filename, const char *find, gboolean recursive)
{
	gboolean ret;
	gchar *base;
	gchar *dir;

	if (!filename)
		return FALSE;

	if (recursive)
	{
		ret = find_subdir(filename, find);
	}
	else
	{
		if (g_file_test(filename, G_FILE_TEST_IS_DIR))
			base = g_strdup(filename);
		else
			base = g_path_get_dirname(filename);
		dir = g_build_filename(base, find, NULL);

		ret = g_file_test(dir, G_FILE_TEST_IS_DIR);

		g_free(base);
		g_free(dir);
	}
	return ret;
}


static const VC_RECORD *
find_vc(const char *filename)
{
	GSList *tmp;

	for (tmp = VC; tmp != NULL; tmp = g_slist_next(tmp))
	{
		if (((VC_RECORD *) tmp->data)->in_vc(filename))
		{
			return (VC_RECORD *) tmp->data;
		}
	}
	return NULL;
}

static void *
find_cmd_env(gint cmd_type, gboolean cmd, const gchar * filename)
{
	const VC_RECORD *vc;
	vc = find_vc(filename);
	if (vc)
	{
		if (cmd)
			return vc->commands[cmd_type];
		else
			return vc->envs[cmd_type];
	}
	return NULL;
}

/* Get list of commands for given command spec*/
static GSList *
get_cmd(const gchar ** argv, const gchar * filename, GSList * filelist, const gchar * message)
{
	gint i, j;
	gint len = 0;
	gchar **ret;
	gchar *dir;
	gchar *base_filename;
	GSList *head = NULL;
	GSList *tmp;
	GString *repl;

	if (g_file_test(filename, G_FILE_TEST_IS_DIR))
	{
		dir = g_strdup(filename);
	}
	else
	{
		dir = g_path_get_dirname(filename);
	}
	base_filename = g_path_get_basename(filename);

	while (1)
	{
		if (argv[len] == NULL)
			break;
		len++;
	}
	if (filelist)
		ret = g_malloc0(sizeof(gchar *) * (len * g_slist_length(filelist) + 1));
	else
		ret = g_malloc0(sizeof(gchar *) * (len + 1));

	head = g_slist_alloc();
	head->data = ret;

	for (i = 0, j = 0; i < len; i++, j++)
	{
		if (argv[i] == CMD_SEPARATOR)
		{
			if (filelist)
				ret = g_malloc0(sizeof(gchar *) *
						(len * g_slist_length(filelist) + 1));
			else
				ret = g_malloc0(sizeof(gchar *) * (len + 1));
			j = -1;
			head = g_slist_append(head, ret);
		}
		else if (argv[i] == DIRNAME)
		{
			ret[j] = p_utils->get_locale_from_utf8(dir);
		}
		else if (argv[i] == FILENAME)
		{
			ret[j] = p_utils->get_locale_from_utf8(filename);
		}
		else if (argv[i] == BASE_FILENAME)
		{
			ret[j] = p_utils->get_locale_from_utf8(base_filename);
		}
		else if (argv[i] == FILE_LIST)
		{
			for (tmp = filelist; tmp != NULL; tmp = g_slist_next(tmp))
			{
				ret[j] = p_utils->get_locale_from_utf8((gchar *) tmp->data);
				j++;
			}
			j--;
		}
		else if (argv[i] == MESSAGE)
		{
			ret[j] = p_utils->get_locale_from_utf8(message);
		}
		else
		{
			repl = g_string_new(argv[i]);
			p_utils->string_replace_all(repl, P_DIRNAME, dir);
			p_utils->string_replace_all(repl, P_FILENAME, filename);
			p_utils->string_replace_all(repl, P_BASE_FILENAME, base_filename);
			ret[j] = g_string_free(repl, FALSE);
			setptr(ret[j], p_utils->get_locale_from_utf8(ret[j]));
		}
	}
	g_free(dir);
	g_free(base_filename);
	return head;
}


/* name should be in UTF-8, and can have a path. */
static void
show_output(const gchar * std_output, const gchar * name, const gchar * force_encoding)
{
	gint page;
	GtkNotebook *book;
	GeanyDocument *doc, *cur_doc;

	if (std_output)
	{
		cur_doc = p_document->get_current();
		doc = p_document->find_by_filename(name);
		if (doc == NULL)
		{
			doc = p_document->new_file(name, NULL, std_output);
		}
		else
		{
			p_sci->set_text(doc->editor->sci, std_output);
			book = GTK_NOTEBOOK(geany->main_widgets->notebook);
			page = gtk_notebook_page_num(book, GTK_WIDGET(doc->editor->sci));
			gtk_notebook_set_current_page(book, page);
		}
		p_document->set_text_changed(doc, set_changed_flag);
		p_document->set_encoding(doc, (force_encoding ? force_encoding : "UTF-8"));
		p_navqueue->goto_line(cur_doc, doc, 1);
	}
	else
	{
		p_ui->set_statusbar(FALSE, _("Could not parse the output of command"));
	}
}

/*
 * Execute command by command spec, return std_out std_err
 *
 * @argv - command spec
 * @env - envirounment
 * @std_out - if not NULL here will be returned standard output converted to utf8 of last command in spec
 * @std_err - if not NULL here will be returned standard error converted to utf8 of last command in spec
 * @filename - filename for spec, commands will be running in it's basedir . Used to replace FILENAME, BASE_FILENAME in spec
 * @list - used to replace FILE_LIST in spec
 * @message - used to replace MESSAGE in spec
 *
 * @return - exit code of last command in spec
 */
gint
execute_custom_command(const gchar ** argv, const gchar ** env, gchar ** std_out, gchar ** std_err,
		       const gchar * filename, GSList * list, const gchar * message)
{
	gint exit_code;
	gchar *dir;
	GString *tmp;
	GSList *cur;
	GSList *largv = get_cmd(argv, filename, list, message);
	GError *error = NULL;

	if (std_out)
		*std_out = NULL;
	if (std_err)
		*std_err = NULL;

	if (!largv)
	{
		return 0;
	}

	if (g_file_test(filename, G_FILE_TEST_IS_DIR))
	{
		dir = g_strdup(filename);
	}
	else
	{
		dir = g_path_get_dirname(filename);
	}

	for (cur = largv; cur != NULL; cur = g_slist_next(cur))
	{
		argv = cur->data;
		if (cur != g_slist_last(largv))
		{
			p_utils->spawn_sync(dir, cur->data, (gchar **) env,
					    G_SPAWN_SEARCH_PATH | G_SPAWN_STDOUT_TO_DEV_NULL |
					    G_SPAWN_STDERR_TO_DEV_NULL, NULL, NULL, NULL, NULL,
					    &exit_code, &error);
		}
		else
		{
			p_utils->spawn_sync(dir, cur->data, (gchar **) env,
					    G_SPAWN_SEARCH_PATH | (std_out ? 0 :
								   G_SPAWN_STDOUT_TO_DEV_NULL) |
					    (std_err ? 0 : G_SPAWN_STDERR_TO_DEV_NULL), NULL, NULL,
					    std_out, std_err, &exit_code, &error);
		}
		if (error)
		{
			fprintf(stderr, "geanyvc: s_spawn_sync error: %s\n", error->message);
			p_ui->set_statusbar(FALSE, _("geanyvc: s_spawn_sync error: %s"),
					    error->message);
			g_error_free(error);
		}

		// need to convert output text from the encoding of the original file into
		// UTF-8 because internally Geany always needs UTF-8
		if (std_out && *std_out)
		{
			tmp = g_string_new(*std_out);
			p_utils->string_replace_all(tmp, "\r\n", "\n");
			p_utils->string_replace_all(tmp, "\r", "\n");
			setptr(*std_out, g_string_free(tmp, FALSE));

			if (!g_utf8_validate(*std_out, -1, NULL))
			{
				setptr(*std_out, p_encodings->convert_to_utf8(*std_out,
									      strlen(*std_out),
									      NULL));
			}
			if (!NZV(*std_out))
			{
				g_free(*std_out);
				*std_out = NULL;
			}
		}
		if (std_err && *std_err)
		{
			tmp = g_string_new(*std_err);
			p_utils->string_replace_all(tmp, "\r\n", "\n");
			p_utils->string_replace_all(tmp, "\r", "\n");
			setptr(*std_err, g_string_free(tmp, FALSE));

			if (!g_utf8_validate(*std_err, -1, NULL))
			{
				setptr(*std_err, p_encodings->convert_to_utf8(*std_err,
									      strlen(*std_err),
									      NULL));
			}
			if (!NZV(*std_err))
			{
				g_free(*std_err);
				*std_err = NULL;
			}
		}
		g_strfreev(cur->data);
	}
	g_free(dir);
	g_slist_free(largv);
	return exit_code;
}

gint
execute_command(const VC_RECORD * vc, gchar ** std_out, gchar ** std_err, const gchar * filename,
		gint cmd, GSList * list, const gchar * message)
{
	if (std_out)
		*std_out = NULL;
	if (std_err)
		*std_err = NULL;

	if (((gchar **) vc->commands[cmd])[0] == CMD_FUNCTION)
	{
		typedef gint(*cmd_function) (gchar **, gchar **, const gchar *, GSList *,
					     const gchar *);
		return ((cmd_function *) vc->commands[cmd])[1] (std_out, std_err, filename, list,
								message);
	}
	return execute_custom_command(vc->commands[cmd], vc->envs[cmd], std_out, std_err, filename,
				      list, message);
}

/* Callback if menu item for a single file was activated */
static void
vcdiff_file_activated(G_GNUC_UNUSED GtkMenuItem * menuitem, G_GNUC_UNUSED gpointer gdata)
{
	gchar *text = NULL;
	gchar *new, *old;
	gchar *name;
	gchar *localename;
	const VC_RECORD *vc;
	GeanyDocument *doc;

	doc = p_document->get_current();
	g_return_if_fail(doc != NULL && doc->file_name != NULL);

	if (doc->changed)
	{
		p_document->save_file(doc, FALSE);
	}

	vc = find_vc(doc->file_name);
	g_return_if_fail(vc);

	execute_command(vc, &text, NULL, doc->file_name, VC_COMMAND_DIFF_FILE, NULL, NULL);
	if (text)
	{
		if (set_external_diff && get_external_diff_viewer())
		{
			g_free(text);

			/*  1) rename file to file.geany.~NEW~
			   2) revert file
			   3) rename file to file.geanyvc.~BASE~
			   4) rename file.geany.~NEW~ to origin file
			   5) show diff
			 */
			localename = p_utils->get_locale_from_utf8(doc->file_name);

			new = g_strconcat(doc->file_name, ".geanyvc.~NEW~", NULL);
			setptr(new, p_utils->get_locale_from_utf8(new));

			old = g_strconcat(doc->file_name, ".geanyvc.~BASE~", NULL);
			setptr(old, p_utils->get_locale_from_utf8(old));

			if (g_rename(localename, new) != 0)
			{
				fprintf(stderr,
					_
					("geanyvc: vcdiff_file_activated: Unable to rename '%s' to '%s'\n"),
					localename, new);
				goto end;
			}

			execute_command(vc, NULL, NULL, doc->file_name,
					VC_COMMAND_REVERT_FILE, NULL, NULL);

			if (g_rename(localename, old) != 0)
			{
				fprintf(stderr,
					_
					("geanyvc: vcdiff_file_activated: Unable to rename '%s' to '%s'\n"),
					localename, old);
				g_rename(new, localename);
				goto end;
			}
			g_rename(new, localename);

			vc_external_diff(old, localename);
			g_unlink(old);
		      end:
			g_free(old);
			g_free(new);
			g_free(localename);
			return;
		}
		else
		{
			name = g_strconcat(doc->file_name, ".vc.diff", NULL);
			show_output(text, name, doc->encoding);
			g_free(text);
			g_free(name);
		}

	}
	else
	{
		p_ui->set_statusbar(FALSE, _("No changes were made."));
	}
}

/* Make a diff from the current directory */
static void
vcdiff_dir_activated(G_GNUC_UNUSED GtkMenuItem * menuitem, G_GNUC_UNUSED gpointer gdata)
{
	gchar *base_name = NULL;
	gchar *text = NULL;
	const VC_RECORD *vc;
	GeanyDocument *doc;

	doc = p_document->get_current();
	g_return_if_fail(doc != NULL && doc->file_name != NULL);

	if (doc->changed)
	{
		p_document->save_file(doc, FALSE);
	}

	base_name = g_path_get_dirname(doc->file_name);

	vc = find_vc(base_name);
	g_return_if_fail(vc);

	execute_command(vc, &text, NULL, base_name, VC_COMMAND_DIFF_DIR, NULL, NULL);
	if (text)
	{
		setptr(base_name, g_strconcat(base_name, ".vc.diff", NULL));
		show_output(text, base_name, NULL);
		g_free(text);
	}
	else
	{
		p_ui->set_statusbar(FALSE, _("No changes were made."));
	}

	g_free(base_name);
}


/* Callback if menu item for the current project was activated */
static void
vcdiff_project_activated(G_GNUC_UNUSED GtkMenuItem * menuitem, G_GNUC_UNUSED gpointer gdata)
{
	gchar *text = NULL;
	gchar *name;
	const VC_RECORD *vc;
	GeanyDocument *doc;

	GeanyProject *project = geany->app->project;

	doc = p_document->get_current();

	g_return_if_fail(geany->app->project != NULL && NZV(geany->app->project->base_path));

	if (doc && doc->changed && doc->file_name != NULL)
	{
		p_document->save_file(doc, FALSE);
	}

	vc = find_vc(geany->app->project->base_path);
	g_return_if_fail(vc);

	execute_command(vc, &text, NULL, geany->app->project->base_path, VC_COMMAND_DIFF_DIR, NULL,
			NULL);
	if (text)
	{
		name = g_strconcat(project->name, ".vc.diff", NULL);
		show_output(text, name, NULL);
		g_free(text);
		g_free(name);
	}
	else
	{
		p_ui->set_statusbar(FALSE, _("No changes were made."));
	}
}

static void
vcblame_activated(G_GNUC_UNUSED GtkMenuItem * menuitem, G_GNUC_UNUSED gpointer gdata)
{
	gchar *text = NULL;
	const VC_RECORD *vc;
	GeanyDocument *doc;

	doc = p_document->get_current();
	g_return_if_fail(doc != NULL && doc->file_name != NULL);

	vc = find_vc(doc->file_name);
	g_return_if_fail(vc);

	execute_command(vc, &text, NULL, doc->file_name, VC_COMMAND_BLAME, NULL, NULL);
	if (text)
	{
		show_output(text, "*VC-BLAME*", NULL);
		g_free(text);
	}
	else
	{
		p_ui->set_statusbar(FALSE, _("No history avaible"));
	}
}


static void
vclog_file_activated(G_GNUC_UNUSED GtkMenuItem * menuitem, G_GNUC_UNUSED gpointer gdata)
{
	gchar *output = NULL;
	const VC_RECORD *vc;
	GeanyDocument *doc;

	doc = p_document->get_current();
	g_return_if_fail(doc != NULL && doc->file_name != NULL);

	p_ui->set_statusbar(TRUE, doc->file_name);

	vc = find_vc(doc->file_name);
	g_return_if_fail(vc);

	execute_command(vc, &output, NULL, doc->file_name, VC_COMMAND_LOG_FILE, NULL, NULL);
	if (output)
	{
		show_output(output, "*VC-LOG*", NULL);
		g_free(output);
	}
}

static void
vclog_dir_activated(G_GNUC_UNUSED GtkMenuItem * menuitem, G_GNUC_UNUSED gpointer gdata)
{
	gchar *base_name = NULL;
	gchar *text = NULL;
	const VC_RECORD *vc;
	GeanyDocument *doc;

	doc = p_document->get_current();
	g_return_if_fail(doc != NULL && doc->file_name != NULL);

	base_name = g_path_get_dirname(doc->file_name);

	vc = find_vc(base_name);
	g_return_if_fail(vc);

	execute_command(vc, &text, NULL, base_name, VC_COMMAND_LOG_DIR, NULL, NULL);
	if (text)
	{
		show_output(text, "*VC-LOG*", NULL);
		g_free(text);
	}

	g_free(base_name);
}

static void
vclog_project_activated(G_GNUC_UNUSED GtkMenuItem * menuitem, G_GNUC_UNUSED gpointer gdata)
{
	gchar *text = NULL;
	const VC_RECORD *vc;
	GeanyProject *project = geany->app->project;

	g_return_if_fail(project != NULL && NZV(geany->app->project->base_path));

	vc = find_vc(geany->app->project->base_path);
	g_return_if_fail(vc);

	execute_command(vc, &text, NULL, geany->app->project->base_path, VC_COMMAND_LOG_DIR, NULL,
			NULL);
	if (text)
	{
		show_output(text, "*VC-LOG*", NULL);
		g_free(text);
	}
}

/* Show status from the current directory */
static void
vcstatus_activated(G_GNUC_UNUSED GtkMenuItem * menuitem, G_GNUC_UNUSED gpointer gdata)
{
	gchar *base_name = NULL;
	gchar *text = NULL;
	const VC_RECORD *vc;
	GeanyDocument *doc;

	doc = p_document->get_current();
	g_return_if_fail(doc != NULL && doc->file_name != NULL);

	if (doc->changed)
	{
		p_document->save_file(doc, FALSE);
	}

	base_name = g_path_get_dirname(doc->file_name);

	vc = find_vc(base_name);
	g_return_if_fail(vc);

	execute_command(vc, &text, NULL, base_name, VC_COMMAND_STATUS, NULL, NULL);
	if (text)
	{
		show_output(text, "*VC-STATUS*", NULL);
		g_free(text);
	}

	g_free(base_name);
}

static gboolean
command_with_question_activated(G_GNUC_UNUSED GtkMenuItem * menuitem, G_GNUC_UNUSED gpointer gdata,
				gint cmd, const gchar * question, gboolean ask)
{
	gchar *text;
	GtkWidget *dialog;
	gint result;
	gchar *dir;
	const VC_RECORD *vc;
	GeanyDocument *doc;

	doc = p_document->get_current();
	g_return_val_if_fail(doc != NULL && doc->file_name != NULL, FALSE);

	if (doc->changed)
	{
		p_document->save_file(doc, FALSE);
	}

	if (ask)
	{
		dialog = gtk_message_dialog_new(GTK_WINDOW(geany->main_widgets->window),
						GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_MESSAGE_QUESTION,
						GTK_BUTTONS_YES_NO, question, doc->file_name);
		result = gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	}
	else
	{
		result = GTK_RESPONSE_YES;
	}

	if (result == GTK_RESPONSE_YES)
	{
		dir = g_path_get_dirname(doc->file_name);
		vc = find_vc(dir);
		g_return_val_if_fail(vc, FALSE);

		execute_command(vc, &text, NULL, doc->file_name, cmd, NULL, NULL);
		if (text)
		{
			g_free(text);
		}
		g_free(dir);
	}
	return (result == GTK_RESPONSE_YES) ? TRUE : FALSE;
}

static void
vcrevert_activated(GtkMenuItem * menuitem, gpointer gdata)
{
	gboolean ret;
	GeanyDocument *doc;

	doc = p_document->get_current();
	g_return_if_fail(doc != NULL && doc->file_name != NULL);

	ret = command_with_question_activated(menuitem, gdata, VC_COMMAND_REVERT_FILE,
					      _("Do you really want to revert: %s"), TRUE);
	if (ret)
	{
		p_document->reload_file(doc, NULL);
	}
}


static void
vcadd_activated(GtkMenuItem * menuitem, gpointer gdata)
{
	command_with_question_activated(menuitem, gdata, VC_COMMAND_ADD,
					_("Do you really want to add: %s"), set_add_confirmation);
}

static void
vcremove_activated(GtkMenuItem * menuitem, gpointer gdata)
{
	gboolean ret;

	ret = command_with_question_activated(menuitem, gdata, VC_COMMAND_REMOVE,
					      _("Do you really want to remove: %s"), TRUE);
	if (ret)
	{
		p_document->
			remove_page(gtk_notebook_get_current_page
				    (GTK_NOTEBOOK(geany->main_widgets->notebook)));
	}
}

enum
{
	COLUMN_COMMIT,
	COLUMN_STATUS,
	COLUMN_PATH,
	NUM_COLUMNS
};

static GtkTreeModel *
create_commit_model(const GSList * commit)
{
	GtkListStore *store;
	GtkTreeIter iter;
	const GSList *cur;

	/* create list store */
	store = gtk_list_store_new(NUM_COLUMNS, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING);

	/* add data to the list store */

	for (cur = commit; cur != NULL; cur = g_slist_next(cur))
	{
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
				   COLUMN_COMMIT, TRUE,
				   COLUMN_STATUS, ((CommitItem *) (cur->data))->status,
				   COLUMN_PATH, ((CommitItem *) (cur->data))->path, -1);
	}

	return GTK_TREE_MODEL(store);
}

static gboolean
get_commit_files_foreach(GtkTreeModel * model, G_GNUC_UNUSED GtkTreePath * path, GtkTreeIter * iter,
			 gpointer data)
{
	GSList **files = (GSList **) data;
	gboolean commit;
	gchar *filename;

	gtk_tree_model_get(model, iter, COLUMN_COMMIT, &commit, -1);
	if (!commit)
		return FALSE;

	gtk_tree_model_get(model, iter, COLUMN_PATH, &filename, -1);
	*files = g_slist_prepend(*files, filename);
	return FALSE;
}

static gboolean
get_commit_diff_foreach(GtkTreeModel * model, G_GNUC_UNUSED GtkTreePath * path, GtkTreeIter * iter,
			gpointer data)
{
	gchar **diff = (gchar **) data;
	gboolean commit;
	gchar *filename;
	gchar *tmp = NULL;
	gchar *status;
	const VC_RECORD *vc;

	gtk_tree_model_get(model, iter, COLUMN_COMMIT, &commit, -1);
	if (!commit)
		return FALSE;

	gtk_tree_model_get(model, iter, COLUMN_STATUS, &status, -1);

	if (strcmp(status, FILE_STATUS_MODIFIED) != 0)
	{
		g_free(status);
		return FALSE;
	}

	gtk_tree_model_get(model, iter, COLUMN_PATH, &filename, -1);

	vc = find_vc(filename);
	g_return_val_if_fail(vc, FALSE);

	execute_command(vc, &tmp, NULL, filename, VC_COMMAND_DIFF_FILE, NULL, NULL);
	if (tmp)
	{
		setptr(*diff, g_strdup_printf("%s%s", *diff, tmp));
		g_free(tmp);
	}
	else
	{
		fprintf(stderr, "error: geanyvc: get_commit_diff_foreach: empty diff output\n");
	}
	g_free(filename);
	return FALSE;
}

static gchar *
get_commit_diff(GtkTreeView * treeview)
{
	GtkTreeModel *model = gtk_tree_view_get_model(treeview);
	gchar *ret = g_strdup("");

	gtk_tree_model_foreach(model, get_commit_diff_foreach, &ret);
	return ret;
}

static void
set_diff_buff(GtkTextBuffer * buffer, const gchar * txt)
{
	GtkTextIter start, end;
	const gchar *tagname = "";

	const gchar *p = txt;
	gtk_text_buffer_set_text(buffer, txt, -1);

	gtk_text_buffer_get_start_iter(buffer, &start);
	gtk_text_buffer_get_end_iter(buffer, &end);

	gtk_text_buffer_remove_all_tags(buffer, &start, &end);

	while (p)
	{
		if (*p == '-')
		{
			tagname = "deleted";
		}
		else if (*p == '+')
		{
			tagname = "added";
		}
		else if (*p == ' ')
		{
			tagname = "";
		}
		else
		{
			tagname = "default";
		}
		gtk_text_buffer_get_iter_at_offset(buffer, &start,
						   g_utf8_pointer_to_offset(txt, p));

		p = strchr(p, '\n');
		if (p)
		{
			if (*tagname)
			{
				gtk_text_buffer_get_iter_at_offset(buffer, &end,
								   g_utf8_pointer_to_offset(txt,
											    p));
				gtk_text_buffer_apply_tag_by_name(buffer, tagname, &start, &end);
			}
			p++;
		}
	}
}

static void
commit_toggled(G_GNUC_UNUSED GtkCellRendererToggle * cell, gchar * path_str, gpointer data)
{
	GtkTreeView *treeview = GTK_TREE_VIEW(data);
	GtkWidget *diffView = p_support->lookup_widget(GTK_WIDGET(treeview), "textDiff");
	GtkTreeModel *model = gtk_tree_view_get_model(treeview);
	GtkTreeIter iter;
	GtkTreePath *path = gtk_tree_path_new_from_string(path_str);
	gboolean fixed;
	gchar *diff;

	/* get toggled iter */
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter, COLUMN_COMMIT, &fixed, -1);

	/* do something with the value */
	fixed ^= 1;

	/* set new value */
	gtk_list_store_set(GTK_LIST_STORE(model), &iter, COLUMN_COMMIT, fixed, -1);

	diff = get_commit_diff(GTK_TREE_VIEW(treeview));
	set_diff_buff(gtk_text_view_get_buffer(GTK_TEXT_VIEW(diffView)), diff);

	/* clean up */
	gtk_tree_path_free(path);
	g_free(diff);
}

static void
add_commit_columns(GtkTreeView * treeview)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	/* column for fixed toggles */
	renderer = gtk_cell_renderer_toggle_new();
	g_signal_connect(renderer, "toggled", G_CALLBACK(commit_toggled), treeview);

	column = gtk_tree_view_column_new_with_attributes("Commit?",
							  renderer, "active", COLUMN_COMMIT, NULL);

	/* set this column to a fixed sizing (of 50 pixels) */
	gtk_tree_view_column_set_sizing(GTK_TREE_VIEW_COLUMN(column), GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width(GTK_TREE_VIEW_COLUMN(column), 70);
	gtk_tree_view_append_column(treeview, column);

	/* column for bug numbers */
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Status",
							  renderer, "text", COLUMN_STATUS, NULL);
	gtk_tree_view_column_set_sort_column_id(column, COLUMN_STATUS);
	gtk_tree_view_append_column(treeview, column);

	/* column for severities */
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Path",
							  renderer, "text", COLUMN_PATH, NULL);
	gtk_tree_view_column_set_sort_column_id(column, COLUMN_PATH);
	gtk_tree_view_append_column(treeview, column);
}

static GdkColor *
get_diff_color(G_GNUC_UNUSED GeanyDocument * doc, gint style)
{
	static GdkColor c = { 0, 0, 0, 0 };
	const GeanyLexerStyle *s;

	s = p_highlighting->get_style(GEANY_FILETYPES_DIFF, style);
	c.red = (s->foreground % 256) * 257;
	c.green = s->foreground & -16711936;
	c.blue = (s->foreground & 0xff0000) / 256;

	return &c;
}

#define GLADE_HOOKUP_OBJECT(component,widget,name) \
  g_object_set_data_full (G_OBJECT (component), name, \
    gtk_widget_ref (widget), (GDestroyNotify) gtk_widget_unref)

#define GLADE_HOOKUP_OBJECT_NO_REF(component,widget,name) \
  g_object_set_data (G_OBJECT (component), name, widget)

GtkWidget *
create_commitDialog(void)
{
	GtkWidget *commitDialog;
	GtkWidget *dialog_vbox1;
	GtkWidget *vpaned1;
	GtkWidget *scrolledwindow1;
	GtkWidget *treeSelect;
	GtkWidget *vpaned2;
	GtkWidget *scrolledwindow2;
	GtkWidget *textDiff;
	GtkWidget *frame1;
	GtkWidget *alignment1;
	GtkWidget *scrolledwindow3;
	GtkWidget *textCommitMessage;
	GtkWidget *label1;
	GtkWidget *dialog_action_area1;
	GtkWidget *btnCancel;
	GtkWidget *btnCommit;

	gchar *rcstyle = g_strdup_printf("style \"geanyvc-diff-font\"\n"
					 "{\n"
					 "    font_name=\"%s\"\n"
					 "}\n"
					 "widget \"*.GeanyVCCommitDialogDiff\" style \"geanyvc-diff-font\"",
					 geany_data->interface_prefs->editor_font);

	gtk_rc_parse_string(rcstyle);
	g_free(rcstyle);

	commitDialog = gtk_dialog_new();
	gtk_container_set_border_width(GTK_CONTAINER(commitDialog), 5);
	gtk_widget_set_events(commitDialog,
			      GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK |
			      GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
	gtk_window_set_title(GTK_WINDOW(commitDialog), _("Commit"));
	gtk_window_set_position(GTK_WINDOW(commitDialog), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_modal(GTK_WINDOW(commitDialog), TRUE);
	gtk_window_set_destroy_with_parent(GTK_WINDOW(commitDialog), TRUE);
	gtk_window_set_type_hint(GTK_WINDOW(commitDialog), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_dialog_set_has_separator(GTK_DIALOG(commitDialog), FALSE);

	dialog_vbox1 = GTK_DIALOG(commitDialog)->vbox;
	gtk_widget_show(dialog_vbox1);

	vpaned1 = gtk_vpaned_new();
	gtk_widget_show(vpaned1);
	gtk_box_pack_start(GTK_BOX(dialog_vbox1), vpaned1, TRUE, TRUE, 0);

	scrolledwindow1 = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolledwindow1);
	gtk_paned_pack1(GTK_PANED(vpaned1), scrolledwindow1, FALSE, TRUE);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow1), GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);

	treeSelect = gtk_tree_view_new();
	gtk_widget_show(treeSelect);
	gtk_container_add(GTK_CONTAINER(scrolledwindow1), treeSelect);
	gtk_widget_set_events(treeSelect,
			      GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK |
			      GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

	vpaned2 = gtk_vpaned_new();
	gtk_widget_show(vpaned2);
	gtk_paned_pack2(GTK_PANED(vpaned1), vpaned2, TRUE, TRUE);

	scrolledwindow2 = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolledwindow2);
	gtk_paned_pack1(GTK_PANED(vpaned2), scrolledwindow2, TRUE, TRUE);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow2), GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledwindow2), GTK_SHADOW_IN);

	textDiff = gtk_text_view_new();
	gtk_widget_set_name(textDiff, "GeanyVCCommitDialogDiff");
	gtk_widget_show(textDiff);
	gtk_container_add(GTK_CONTAINER(scrolledwindow2), textDiff);
	gtk_widget_set_events(textDiff,
			      GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK |
			      GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(textDiff), FALSE);

	frame1 = gtk_frame_new(NULL);
	gtk_widget_show(frame1);
	gtk_paned_pack2(GTK_PANED(vpaned2), frame1, TRUE, TRUE);
	gtk_frame_set_shadow_type(GTK_FRAME(frame1), GTK_SHADOW_NONE);

	alignment1 = gtk_alignment_new(0.5, 0.5, 1, 1);
	gtk_widget_show(alignment1);
	gtk_container_add(GTK_CONTAINER(frame1), alignment1);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment1), 0, 0, 12, 0);

	scrolledwindow3 = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolledwindow3);
	gtk_container_add(GTK_CONTAINER(alignment1), scrolledwindow3);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow3), GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledwindow3), GTK_SHADOW_IN);

	textCommitMessage = gtk_text_view_new();
	gtk_widget_show(textCommitMessage);
	gtk_container_add(GTK_CONTAINER(scrolledwindow3), textCommitMessage);
	gtk_widget_set_events(textCommitMessage,
			      GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK |
			      GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

	label1 = gtk_label_new(_("<b>Commit message:</b>"));
	gtk_widget_show(label1);
	gtk_frame_set_label_widget(GTK_FRAME(frame1), label1);
	gtk_label_set_use_markup(GTK_LABEL(label1), TRUE);

	dialog_action_area1 = GTK_DIALOG(commitDialog)->action_area;
	gtk_widget_show(dialog_action_area1);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(dialog_action_area1), GTK_BUTTONBOX_END);

	btnCancel = gtk_button_new_from_stock("gtk-cancel");
	gtk_widget_show(btnCancel);
	gtk_dialog_add_action_widget(GTK_DIALOG(commitDialog), btnCancel, GTK_RESPONSE_CANCEL);

	btnCommit = gtk_button_new_with_mnemonic(_("_Commit"));
	gtk_widget_show(btnCommit);
	gtk_dialog_add_action_widget(GTK_DIALOG(commitDialog), btnCommit, GTK_RESPONSE_APPLY);

	/* Store pointers to all widgets, for use by lookup_widget(). */
	GLADE_HOOKUP_OBJECT_NO_REF(commitDialog, commitDialog, "commitDialog");
	GLADE_HOOKUP_OBJECT_NO_REF(commitDialog, dialog_vbox1, "dialog_vbox1");
	GLADE_HOOKUP_OBJECT(commitDialog, vpaned1, "vpaned1");
	GLADE_HOOKUP_OBJECT(commitDialog, scrolledwindow1, "scrolledwindow1");
	GLADE_HOOKUP_OBJECT(commitDialog, treeSelect, "treeSelect");
	GLADE_HOOKUP_OBJECT(commitDialog, vpaned2, "vpaned2");
	GLADE_HOOKUP_OBJECT(commitDialog, scrolledwindow2, "scrolledwindow2");
	GLADE_HOOKUP_OBJECT(commitDialog, textDiff, "textDiff");
	GLADE_HOOKUP_OBJECT(commitDialog, frame1, "frame1");
	GLADE_HOOKUP_OBJECT(commitDialog, alignment1, "alignment1");
	GLADE_HOOKUP_OBJECT(commitDialog, scrolledwindow3, "scrolledwindow3");
	GLADE_HOOKUP_OBJECT(commitDialog, textCommitMessage, "textCommitMessage");
	GLADE_HOOKUP_OBJECT(commitDialog, label1, "label1");
	GLADE_HOOKUP_OBJECT_NO_REF(commitDialog, dialog_action_area1, "dialog_action_area1");
	GLADE_HOOKUP_OBJECT(commitDialog, btnCancel, "btnCancel");
	GLADE_HOOKUP_OBJECT(commitDialog, btnCommit, "btnCommit");

	return commitDialog;
}

static void
vccommit_activated(G_GNUC_UNUSED GtkMenuItem * menuitem, G_GNUC_UNUSED gpointer gdata)
{
	GeanyDocument *doc;
	gint result;
	const VC_RECORD *vc;
	GSList *lst;
	GtkTreeModel *model;
	GtkWidget *commit = create_commitDialog();
	GtkWidget *treeview = p_support->lookup_widget(commit, "treeSelect");
	GtkWidget *diffView = p_support->lookup_widget(commit, "textDiff");
	GtkWidget *messageView = p_support->lookup_widget(commit, "textCommitMessage");
	GtkWidget *vpaned1 = p_support->lookup_widget(commit, "vpaned1");
	GtkWidget *vpaned2 = p_support->lookup_widget(commit, "vpaned2");

	GtkTextBuffer *mbuf;
	GtkTextBuffer *diffbuf;

	GtkTextIter begin;
	GtkTextIter end;
	GSList *selected_files = NULL;

	gchar *dir;
	gchar *message;
	gchar *diff;

	gint height;

#ifdef USE_GTKSPELL
	GtkSpell *speller = NULL;
	GError *spellcheck_error = NULL;
#endif

	doc = p_document->get_current();
	g_return_if_fail(doc);
	g_return_if_fail(doc->file_name);
	dir = g_path_get_dirname(doc->file_name);

	vc = find_vc(dir);
	g_return_if_fail(vc);

	lst = vc->get_commit_files(dir);
	if (!lst)
	{
		g_free(dir);
		p_ui->set_statusbar(FALSE, _("Nothing to commit."));
		return;
	}

	model = create_commit_model(lst);
	gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), model);
	g_object_unref(model);

	/* add columns to the tree view */
	add_commit_columns(GTK_TREE_VIEW(treeview));

	diff = get_commit_diff(GTK_TREE_VIEW(treeview));
	diffbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(diffView));

	gtk_text_buffer_create_tag(diffbuf, "deleted", "foreground-gdk",
				   get_diff_color(doc, SCE_DIFF_DELETED), NULL);

	gtk_text_buffer_create_tag(diffbuf, "added", "foreground-gdk",
				   get_diff_color(doc, SCE_DIFF_ADDED), NULL);

	gtk_text_buffer_create_tag(diffbuf, "default", "foreground-gdk",
				   get_diff_color(doc, SCE_DIFF_POSITION), NULL);

	set_diff_buff(diffbuf, diff);

	if (set_maximize_commit_dialog)
	{
		gtk_window_maximize(GTK_WINDOW(commit));
		gtk_widget_show_now(commit);
		gtk_window_get_size(GTK_WINDOW(commit), NULL, &height);
		gtk_paned_set_position(GTK_PANED(vpaned1), height * 30 / 100);
		gtk_paned_set_position(GTK_PANED(vpaned2), height * 15 / 100);
	}
	else
	{
		gtk_widget_set_size_request(commit, 700, 500);
		gtk_widget_show_now(commit);
		gtk_window_get_size(GTK_WINDOW(commit), NULL, &height);
		gtk_paned_set_position(GTK_PANED(vpaned1), height * 30 / 100);
		gtk_paned_set_position(GTK_PANED(vpaned2), height * 55 / 100);
	}

#ifdef USE_GTKSPELL
	speller = gtkspell_new_attach(GTK_TEXT_VIEW(messageView), NULL, &spellcheck_error);
	if (speller == NULL)
	{
		p_ui->set_statusbar(FALSE, _("Error initializing spell checking: %s"),
				    spellcheck_error->message);
		g_error_free(spellcheck_error);
		spellcheck_error = NULL;
	}
	else if (lang != NULL)
	{
		gtkspell_set_language(speller, lang, &spellcheck_error);
		if (spellcheck_error != NULL)
		{
			p_ui->set_statusbar(TRUE,
					    _
					    ("Error while setting up language for spellchecking. Please check configuration. Error message was: %s"),
					    spellcheck_error->message);
			g_error_free(spellcheck_error);
			spellcheck_error = NULL;
		}
	}
#endif

	/* put the input focus to the commit message text view */
	gtk_widget_grab_focus(messageView);

	result = gtk_dialog_run(GTK_DIALOG(commit));
	if (result == GTK_RESPONSE_APPLY)
	{
		mbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(messageView));
		gtk_text_buffer_get_start_iter(mbuf, &begin);
		gtk_text_buffer_get_end_iter(mbuf, &end);
		message = gtk_text_buffer_get_text(mbuf, &begin, &end, FALSE);
		gtk_tree_model_foreach(model, get_commit_files_foreach, &selected_files);
		if (NZV(message) && selected_files)
		{
			execute_command(vc, NULL, NULL, dir, VC_COMMAND_COMMIT, selected_files,
					message);
			free_text_list(selected_files);
		}
		g_free(message);
	}

	gtk_widget_destroy(commit);
	free_commit_list(lst);
	g_free(dir);
	g_free(diff);
}

static GtkWidget *menu_vc_diff_file = NULL;
static GtkWidget *menu_vc_diff_dir = NULL;
static GtkWidget *menu_vc_diff_project = NULL;
static GtkWidget *menu_vc_blame = NULL;
static GtkWidget *menu_vc_log_file = NULL;
static GtkWidget *menu_vc_log_dir = NULL;
static GtkWidget *menu_vc_log_project = NULL;
static GtkWidget *menu_vc_status = NULL;
static GtkWidget *menu_vc_revert_file = NULL;
static GtkWidget *menu_vc_add_file = NULL;
static GtkWidget *menu_vc_remove_file = NULL;
static GtkWidget *menu_vc_commit = NULL;

static void
update_menu_items()
{
	GeanyDocument *doc;
	GeanyProject *project = geany->app->project;

	gboolean have_file;
	gboolean p_have_vc = FALSE;
	gboolean d_have_vc = FALSE;
	gboolean f_have_vc = FALSE;

	gchar *dir;

	doc = p_document->get_current();
	have_file = doc && doc->file_name && g_path_is_absolute(doc->file_name);

	if (have_file)
	{
		dir = g_path_get_dirname(doc->file_name);
		if (find_cmd_env(VC_COMMAND_DIFF_FILE, TRUE, dir))
			d_have_vc = TRUE;

		if (find_cmd_env(VC_COMMAND_DIFF_FILE, TRUE, doc->file_name))
			f_have_vc = TRUE;
		g_free(dir);
	}

	if (project != NULL && NZV(geany->app->project->base_path) &&
	    find_cmd_env(VC_COMMAND_DIFF_DIR, TRUE, geany->app->project->base_path))
		p_have_vc = TRUE;

	gtk_widget_set_sensitive(menu_vc_diff_file, f_have_vc);
	gtk_widget_set_sensitive(menu_vc_diff_dir, d_have_vc);
	gtk_widget_set_sensitive(menu_vc_diff_project, p_have_vc);

	gtk_widget_set_sensitive(menu_vc_blame, f_have_vc);

	gtk_widget_set_sensitive(menu_vc_log_file, f_have_vc);
	gtk_widget_set_sensitive(menu_vc_log_dir, d_have_vc);
	gtk_widget_set_sensitive(menu_vc_log_project, p_have_vc);

	gtk_widget_set_sensitive(menu_vc_status, d_have_vc);

	gtk_widget_set_sensitive(menu_vc_revert_file, f_have_vc);

	gtk_widget_set_sensitive(menu_vc_remove_file, f_have_vc);
	gtk_widget_set_sensitive(menu_vc_add_file, d_have_vc && !f_have_vc);

	gtk_widget_set_sensitive(menu_vc_commit, d_have_vc);
}

static struct
{
	GtkWidget *cb_changed_flag;
	GtkWidget *cb_confirm_add;
	GtkWidget *cb_max_commit;
	GtkWidget *cb_external_diff;
	GtkWidget *cb_cvs;
	GtkWidget *cb_git;
	GtkWidget *cb_svn;
	GtkWidget *cb_svk;
	GtkWidget *cb_bzr;
	GtkWidget *cb_hg;
#ifdef USE_GTKSPELL
	GtkWidget *spellcheck_lang_textbox;
#endif
}
widgets;

static void
on_configure_response(G_GNUC_UNUSED GtkDialog * dialog, gint response,
		      G_GNUC_UNUSED gpointer user_data)
{
	if (response == GTK_RESPONSE_OK || response == GTK_RESPONSE_APPLY)
	{
		GKeyFile *config = g_key_file_new();
		gchar *data;
		gchar *config_dir = g_path_get_dirname(config_file);

		set_changed_flag =
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widgets.cb_changed_flag));
		set_add_confirmation =
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widgets.cb_confirm_add));
		set_maximize_commit_dialog =
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widgets.cb_max_commit));

		set_external_diff =
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widgets.cb_external_diff));

		enable_cvs = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widgets.cb_cvs));
		enable_git = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widgets.cb_git));
		enable_svn = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widgets.cb_svn));
		enable_svk = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widgets.cb_svk));
		enable_bzr = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widgets.cb_bzr));
		enable_hg = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widgets.cb_hg));

#ifdef USE_GTKSPELL
		g_free(lang);
		lang = g_strdup(gtk_entry_get_text(GTK_ENTRY(widgets.spellcheck_lang_textbox)));
#endif

		g_key_file_load_from_file(config, config_file, G_KEY_FILE_NONE, NULL);

		g_key_file_set_boolean(config, "VC", "set_changed_flag", set_changed_flag);
		g_key_file_set_boolean(config, "VC", "set_add_confirmation", set_add_confirmation);
		g_key_file_set_boolean(config, "VC", "set_external_diff", set_external_diff);
		g_key_file_set_boolean(config, "VC", "set_maximize_commit_dialog",
				       set_maximize_commit_dialog);

		g_key_file_set_boolean(config, "VC", "enable_cvs", enable_cvs);
		g_key_file_set_boolean(config, "VC", "enable_git", enable_git);
		g_key_file_set_boolean(config, "VC", "enable_svn", enable_svn);
		g_key_file_set_boolean(config, "VC", "enable_svk", enable_svk);
		g_key_file_set_boolean(config, "VC", "enable_bzr", enable_bzr);
		g_key_file_set_boolean(config, "VC", "enable_hg", enable_hg);

#ifdef USE_GTKSPELL
		g_key_file_set_string(config, "VC", "spellchecking_language", lang);
#endif

		if (!g_file_test(config_dir, G_FILE_TEST_IS_DIR)
		    && p_utils->mkdir(config_dir, TRUE) != 0)
		{
			p_dialogs->show_msgbox(GTK_MESSAGE_ERROR,
					       _
					       ("Plugin configuration directory could not be created."));
		}
		else
		{
			// write config to file
			data = g_key_file_to_data(config, NULL, NULL);
			p_utils->write_file(config_file, data);
			g_free(data);
		}

		g_free(config_dir);
		g_key_file_free(config);

		registrate();
	}
}

GtkWidget *
plugin_configure(GtkDialog * dialog)
{
	GtkWidget *vbox;

#ifdef USE_GTKSPELL
	GtkWidget *label_spellcheck_lang;
#endif

	GtkTooltips *tooltip = NULL;

	tooltip = gtk_tooltips_new();
	vbox = gtk_vbox_new(FALSE, 6);

	widgets.cb_changed_flag =
		gtk_check_button_new_with_label(_
						("Set Changed-flag for document tabs created by the plugin"));
	gtk_tooltips_set_tip(tooltip, widgets.cb_changed_flag,
			     _
			     ("If this option is activated, every new by the VC-plugin created document tab "
			      "will be marked as changed. Even this option is useful in some cases, it could cause "
			      "a big number of annoying \"Do you want to save\"-dialogs."), NULL);
	gtk_button_set_focus_on_click(GTK_BUTTON(widgets.cb_changed_flag), FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widgets.cb_changed_flag), set_changed_flag);
	gtk_box_pack_start(GTK_BOX(vbox), widgets.cb_changed_flag, FALSE, FALSE, 2);

	widgets.cb_confirm_add =
		gtk_check_button_new_with_label(_("Confirm adding new files to a VCS"));
	gtk_tooltips_set_tip(tooltip, widgets.cb_confirm_add,
			     _
			     ("Shows a confirmation dialog on adding a new (created) file to VCS."),
			     NULL);
	gtk_button_set_focus_on_click(GTK_BUTTON(widgets.cb_confirm_add), FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widgets.cb_confirm_add),
				     set_add_confirmation);
	gtk_box_pack_start(GTK_BOX(vbox), widgets.cb_confirm_add, TRUE, FALSE, 2);

	widgets.cb_max_commit = gtk_check_button_new_with_label(_("Maximize commit dialog"));
	gtk_tooltips_set_tip(tooltip, widgets.cb_max_commit, _("Show commit dialog maximize."),
			     NULL);
	gtk_button_set_focus_on_click(GTK_BUTTON(widgets.cb_max_commit), FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widgets.cb_max_commit),
				     set_maximize_commit_dialog);
	gtk_box_pack_start(GTK_BOX(vbox), widgets.cb_max_commit, TRUE, FALSE, 2);

	widgets.cb_external_diff = gtk_check_button_new_with_label(_("Use external diff viewer"));
	gtk_tooltips_set_tip(tooltip, widgets.cb_external_diff,
			     _("Use external diff viewer for file diff."), NULL);
	gtk_button_set_focus_on_click(GTK_BUTTON(widgets.cb_external_diff), FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widgets.cb_external_diff),
				     set_external_diff);
	gtk_box_pack_start(GTK_BOX(vbox), widgets.cb_external_diff, TRUE, FALSE, 2);

	widgets.cb_cvs = gtk_check_button_new_with_label(_("Enable CVS"));
	gtk_button_set_focus_on_click(GTK_BUTTON(widgets.cb_cvs), FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widgets.cb_cvs), enable_cvs);
	gtk_box_pack_start(GTK_BOX(vbox), widgets.cb_cvs, TRUE, FALSE, 2);

	widgets.cb_git = gtk_check_button_new_with_label(_("Enable GIT"));
	gtk_button_set_focus_on_click(GTK_BUTTON(widgets.cb_git), FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widgets.cb_git), enable_git);
	gtk_box_pack_start(GTK_BOX(vbox), widgets.cb_git, TRUE, FALSE, 2);

	widgets.cb_svn = gtk_check_button_new_with_label(_("Enable SVN"));
	gtk_button_set_focus_on_click(GTK_BUTTON(widgets.cb_svn), FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widgets.cb_svn), enable_svn);
	gtk_box_pack_start(GTK_BOX(vbox), widgets.cb_svn, TRUE, FALSE, 2);

	widgets.cb_svk = gtk_check_button_new_with_label(_("Enable SVK"));
	gtk_button_set_focus_on_click(GTK_BUTTON(widgets.cb_svk), FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widgets.cb_svk), enable_svk);
	gtk_box_pack_start(GTK_BOX(vbox), widgets.cb_svk, TRUE, FALSE, 2);

	widgets.cb_bzr = gtk_check_button_new_with_label(_("Enable Bazaar"));
	gtk_button_set_focus_on_click(GTK_BUTTON(widgets.cb_bzr), FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widgets.cb_bzr), enable_bzr);
	gtk_box_pack_start(GTK_BOX(vbox), widgets.cb_bzr, TRUE, FALSE, 2);

	widgets.cb_hg = gtk_check_button_new_with_label(_("Enable Mercurial"));
	gtk_button_set_focus_on_click(GTK_BUTTON(widgets.cb_hg), FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widgets.cb_hg), enable_hg);
	gtk_box_pack_start(GTK_BOX(vbox), widgets.cb_hg, TRUE, FALSE, 2);

#ifdef USE_GTKSPELL
	label_spellcheck_lang = gtk_label_new(_("Spellcheck language"));
	widgets.spellcheck_lang_textbox = gtk_entry_new();
	gtk_widget_show(widgets.spellcheck_lang_textbox);
	if (lang != NULL)
		gtk_entry_set_text(GTK_ENTRY(widgets.spellcheck_lang_textbox), lang);

	gtk_misc_set_alignment(GTK_MISC(label_spellcheck_lang), 0, 0.5);
	gtk_container_add(GTK_CONTAINER(vbox), label_spellcheck_lang);
	gtk_container_add(GTK_CONTAINER(vbox), widgets.spellcheck_lang_textbox);
#endif
	gtk_widget_show_all(vbox);

	g_signal_connect(dialog, "response", G_CALLBACK(on_configure_response), NULL);
	return vbox;
}

static void
load_config()
{
	GError *error = NULL;
	GKeyFile *config = g_key_file_new();

	g_key_file_load_from_file(config, config_file, G_KEY_FILE_NONE, NULL);
	set_changed_flag = g_key_file_get_boolean(config, "VC", "set_changed_flag", &error);
	if (error != NULL)
	{
		// Set default value
		set_changed_flag = FALSE;
		g_error_free(error);
		error = NULL;
	}
	set_add_confirmation = g_key_file_get_boolean(config, "VC", "set_add_confirmation", &error);
	if (error != NULL)
	{
		// Set default value
		set_add_confirmation = TRUE;
		g_error_free(error);
		error = NULL;
	}
	set_maximize_commit_dialog =
		g_key_file_get_boolean(config, "VC", "set_maximize_commit_dialog", &error);
	if (error != NULL)
	{
		// Set default value
		set_maximize_commit_dialog = FALSE;
		g_error_free(error);
		error = NULL;
	}
	set_external_diff = g_key_file_get_boolean(config, "VC", "set_external_diff", &error);
	if (error != NULL)
	{
		// Set default value
		set_external_diff = TRUE;
		g_error_free(error);
		error = NULL;
	}

	enable_cvs = g_key_file_get_boolean(config, "VC", "enable_cvs", &error);
	if (error != NULL)
	{
		// Set default value
		enable_cvs = TRUE;
		g_error_free(error);
		error = NULL;
	}

	enable_git = g_key_file_get_boolean(config, "VC", "enable_git", &error);
	if (error != NULL)
	{
		// Set default value
		enable_git = TRUE;
		g_error_free(error);
		error = NULL;
	}

	enable_svn = g_key_file_get_boolean(config, "VC", "enable_svn", &error);
	if (error != NULL)
	{
		// Set default value
		enable_svn = TRUE;
		g_error_free(error);
		error = NULL;
	}

	enable_svk = g_key_file_get_boolean(config, "VC", "enable_svk", &error);
	if (error != NULL)
	{
		// Set default value
		enable_svk = TRUE;
		g_error_free(error);
		error = NULL;
	}

	enable_bzr = g_key_file_get_boolean(config, "VC", "enable_bzr", &error);
	if (error != NULL)
	{
		// Set default value
		enable_bzr = TRUE;
		g_error_free(error);
		error = NULL;
	}

	enable_hg = g_key_file_get_boolean(config, "VC", "enable_hg", &error);
	if (error != NULL)
	{
		// Set default value
		enable_hg = TRUE;
		g_error_free(error);
		error = NULL;
	}

#ifdef USE_GTKSPELL
	lang = g_key_file_get_string(config, "VC", "spellchecking_language", &error);
	if (error != NULL)
	{
		// Set default value. Using system standard language.
		lang = NULL;
		g_error_free(error);
		error = NULL;
	}
#endif

	g_key_file_free(config);
}

static void
registrate()
{
	gchar *path;
	if (VC)
	{
		g_slist_free(VC);
		VC = NULL;
	}
	REGISTER_VC(GIT, enable_git);
	REGISTER_VC(SVN, enable_svn);
	REGISTER_VC(CVS, enable_cvs);
	REGISTER_VC(SVK, enable_svk);
	REGISTER_VC(BZR, enable_bzr);
	REGISTER_VC(HG, enable_hg);
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

/* Called by Geany to initialize the plugin */
void
plugin_init(G_GNUC_UNUSED GeanyData * data)
{
	GtkWidget *menu_vc = NULL;
	GtkWidget *menu_vc_menu = NULL;
	GtkTooltips *tooltips = NULL;

	config_file =
		g_strconcat(geany->app->configdir, G_DIR_SEPARATOR_S, "plugins", G_DIR_SEPARATOR_S,
			    "VC", G_DIR_SEPARATOR_S, "VC.conf", NULL);

	load_config();
	registrate();

	locale_init();

	tooltips = gtk_tooltips_new();

	menu_vc = gtk_image_menu_item_new_with_mnemonic(_("_VC"));
	gtk_container_add(GTK_CONTAINER(geany->main_widgets->tools_menu), menu_vc);

	g_signal_connect((gpointer) menu_vc, "activate", G_CALLBACK(update_menu_items), NULL);

	menu_vc_menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_vc), menu_vc_menu);

	// Single file
	menu_vc_diff_file = gtk_menu_item_new_with_mnemonic(_("Diff From Current File"));
	gtk_container_add(GTK_CONTAINER(menu_vc_menu), menu_vc_diff_file);
	gtk_tooltips_set_tip(tooltips, menu_vc_diff_file,
			     _("Make a diff from the current active file"), NULL);

	g_signal_connect((gpointer) menu_vc_diff_file, "activate",
			 G_CALLBACK(vcdiff_file_activated), NULL);

	// Directory
	menu_vc_diff_dir = gtk_menu_item_new_with_mnemonic(_("Diff From Current Directory"));
	gtk_container_add(GTK_CONTAINER(menu_vc_menu), menu_vc_diff_dir);
	gtk_tooltips_set_tip(tooltips, menu_vc_diff_dir,
			     _("Make a diff from the directory of the current active file"), NULL);

	g_signal_connect((gpointer) menu_vc_diff_dir, "activate",
			 G_CALLBACK(vcdiff_dir_activated), NULL);

	// Project
	menu_vc_diff_project = gtk_menu_item_new_with_mnemonic(_("Diff From Current Project"));
	gtk_container_add(GTK_CONTAINER(menu_vc_menu), menu_vc_diff_project);
	gtk_tooltips_set_tip(tooltips, menu_vc_diff_project,
			     _("Make a diff from the current project's base path"), NULL);

	g_signal_connect((gpointer) menu_vc_diff_project, "activate",
			 G_CALLBACK(vcdiff_project_activated), NULL);

	gtk_container_add(GTK_CONTAINER(menu_vc_menu), gtk_separator_menu_item_new());

	// Blame for current file
	menu_vc_blame = gtk_menu_item_new_with_mnemonic(_("Blame From Current File"));
	gtk_container_add(GTK_CONTAINER(menu_vc_menu), menu_vc_blame);
	gtk_tooltips_set_tip(tooltips, menu_vc_blame,
			     _("Shows the changes made at one file per revision and author."),
			     NULL);

	g_signal_connect((gpointer) menu_vc_blame, "activate", G_CALLBACK(vcblame_activated), NULL);

	gtk_container_add(GTK_CONTAINER(menu_vc_menu), gtk_separator_menu_item_new());

	// Log
	menu_vc_log_file = gtk_menu_item_new_with_mnemonic(_("Log Of Current File"));
	gtk_container_add(GTK_CONTAINER(menu_vc_menu), menu_vc_log_file);
	gtk_tooltips_set_tip(tooltips, menu_vc_log_file,
			     _("Shows the log of the current file"), NULL);

	g_signal_connect((gpointer) menu_vc_log_file, "activate",
			 G_CALLBACK(vclog_file_activated), NULL);

	menu_vc_log_dir = gtk_menu_item_new_with_mnemonic(_("Log Of Current Directory"));
	gtk_container_add(GTK_CONTAINER(menu_vc_menu), menu_vc_log_dir);
	gtk_tooltips_set_tip(tooltips, menu_vc_log_dir,
			     _("Shows the log of the current directory"), NULL);

	g_signal_connect((gpointer) menu_vc_log_dir, "activate",
			 G_CALLBACK(vclog_dir_activated), NULL);

	menu_vc_log_project = gtk_menu_item_new_with_mnemonic(_("Log Of Current Project"));
	gtk_container_add(GTK_CONTAINER(menu_vc_menu), menu_vc_log_project);
	gtk_tooltips_set_tip(tooltips, menu_vc_log_project,
			     _("Shows the log of the current project"), NULL);

	g_signal_connect((gpointer) menu_vc_log_project, "activate",
			 G_CALLBACK(vclog_project_activated), NULL);

	gtk_container_add(GTK_CONTAINER(menu_vc_menu), gtk_separator_menu_item_new());

	// Status
	menu_vc_status = gtk_menu_item_new_with_mnemonic(_("_Status"));
	gtk_container_add(GTK_CONTAINER(menu_vc_menu), menu_vc_status);
	gtk_tooltips_set_tip(tooltips, menu_vc_status, _("Show status."), NULL);

	g_signal_connect((gpointer) menu_vc_status, "activate",
			 G_CALLBACK(vcstatus_activated), NULL);

	gtk_container_add(GTK_CONTAINER(menu_vc_menu), gtk_separator_menu_item_new());

	// Revert file
	menu_vc_revert_file = gtk_menu_item_new_with_mnemonic(_("_Revert File"));
	gtk_container_add(GTK_CONTAINER(menu_vc_menu), menu_vc_revert_file);
	gtk_tooltips_set_tip(tooltips, menu_vc_revert_file,
			     _("Restore pristine working copy file (undo local edits)."), NULL);

	g_signal_connect((gpointer) menu_vc_revert_file, "activate",
			 G_CALLBACK(vcrevert_activated), NULL);

	gtk_container_add(GTK_CONTAINER(menu_vc_menu), gtk_separator_menu_item_new());

	// Add file
	menu_vc_add_file = gtk_menu_item_new_with_mnemonic(_("_Add File"));
	gtk_container_add(GTK_CONTAINER(menu_vc_menu), menu_vc_add_file);
	gtk_tooltips_set_tip(tooltips, menu_vc_add_file, _("Add file to repository."), NULL);

	g_signal_connect((gpointer) menu_vc_add_file, "activate",
			 G_CALLBACK(vcadd_activated), NULL);

	// Remove file
	menu_vc_remove_file = gtk_menu_item_new_with_mnemonic(_("Remove File"));
	gtk_container_add(GTK_CONTAINER(menu_vc_menu), menu_vc_remove_file);
	gtk_tooltips_set_tip(tooltips, menu_vc_remove_file,
			     _("Remove file from repository."), NULL);

	g_signal_connect((gpointer) menu_vc_remove_file, "activate",
			 G_CALLBACK(vcremove_activated), NULL);

	gtk_container_add(GTK_CONTAINER(menu_vc_menu), gtk_separator_menu_item_new());

	// Commit
	menu_vc_commit = gtk_menu_item_new_with_mnemonic(_("_Commit"));
	gtk_container_add(GTK_CONTAINER(menu_vc_menu), menu_vc_commit);
	gtk_tooltips_set_tip(tooltips, menu_vc_commit, _("Commit changes."), NULL);

	g_signal_connect((gpointer) menu_vc_commit, "activate",
			 G_CALLBACK(vccommit_activated), NULL);

	gtk_widget_show_all(menu_vc);

	plugin_fields->menu_item = menu_vc;
	plugin_fields->flags = PLUGIN_IS_DOCUMENT_SENSITIVE;
}


/* Called by Geany before unloading the plugin. */
void
plugin_cleanup()
{
	// remove the menu item added in init()
	gtk_widget_destroy(plugin_fields->menu_item);
	g_slist_free(VC);
	VC = NULL;
	g_free(config_file);
}
