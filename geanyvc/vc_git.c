/*
 *      Copyright 2007 Frank Lanitz <frank(at)frank(dot)uvena(dot)de>
 *      Copyright 2007 Enrico Tröger <enrico.troeger@uvena.de>
 *      Copyright 2007 Nick Treleaven <nick.treleaven@btinternet.com>
 *      Copyright 2007 Yura Siamashka <yurand2@gmail.com>
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

#include <string.h>

#include "geany.h"
#include "support.h"
#include "plugindata.h"
#include "document.h"
#include "filetypes.h"
#include "utils.h"
#include "project.h"
#include "pluginmacros.h"

#include "geanyvc.h"

extern GeanyData *geany_data;


static gint
git_commit(G_GNUC_UNUSED gchar ** std_out, G_GNUC_UNUSED gchar ** std_err, const gchar * filename,
	   GSList * list, const gchar * message)
{
	gchar *base_dir = find_subdir_path(filename, ".git");
	gint len = strlen(base_dir);
	GSList *commit = NULL;
	GSList *tmp = NULL;
	const gchar *argv[] = { "git", "commit", "-m", MESSAGE, "--", FILE_LIST, NULL };
	gint ret;

	g_return_val_if_fail(base_dir, -1);

	for (tmp = list; tmp != NULL; tmp = g_slist_next(tmp))
	{
		commit = g_slist_prepend(commit, (gchar *) tmp->data + len + 1);
	}

	ret = execute_custom_command(argv, NULL, NULL, NULL, base_dir, commit, message);
	g_slist_free(commit);
	g_free(base_dir);
	return ret;
}

static const gchar *GIT_ENV_SHOW[] = { "PAGER=cat", NULL };
static gint
git_show(gchar ** std_out, gchar ** std_err, const gchar * filename,
	 GSList * list, const gchar * message)
{
	gchar *base_dir = find_subdir_path(filename, ".git");
	gint len = strlen(base_dir);
	const gchar *argv[] = { "git", "show", NULL, NULL };
	gint ret;

	g_return_val_if_fail(base_dir, -1);

	argv[2] = g_strdup_printf("HEAD:%s", filename + len + 1);

	ret = execute_custom_command(argv, GIT_ENV_SHOW, std_out, std_err, base_dir, list, message);
	g_free(base_dir);
	g_free((gchar *) argv[2]);
	return ret;
}



static const gchar *GIT_CMD_DIFF_FILE[] = { "git", "diff", "HEAD", "--", BASE_FILENAME, NULL };
static const gchar *GIT_CMD_DIFF_DIR[] = { "git", "diff", "HEAD", NULL };
static const gchar *GIT_CMD_REVERT_FILE[] = { "git", "checkout", "--", BASE_FILENAME, NULL };
static const gchar *GIT_CMD_STATUS[] = { "git", "status", NULL };
static const gchar *GIT_CMD_ADD[] = { "git", "add", "--", BASE_FILENAME, NULL };
static const gchar *GIT_CMD_REMOVE[] =
	{ "git", "rm", "-f", "--", BASE_FILENAME, CMD_SEPARATOR, "git", "reset", "HEAD", "--",
	BASE_FILENAME, NULL
};
static const gchar *GIT_CMD_LOG_FILE[] = { "git", "log", "--", BASE_FILENAME, NULL };
static const gchar *GIT_CMD_LOG_DIR[] = { "git", "log", NULL };
static const void *GIT_CMD_COMMIT[] = { CMD_FUNCTION, git_commit };
static const gchar *GIT_CMD_BLAME[] = { "git", "blame", "--", BASE_FILENAME, NULL };
static const void *GIT_CMD_SHOW[] = { CMD_FUNCTION, git_show };

static const gchar *GIT_ENV_DIFF_FILE[] = { "PAGER=cat", NULL };
static const gchar *GIT_ENV_DIFF_DIR[] = { "PAGER=cat", NULL };
static const gchar *GIT_ENV_REVERT_FILE[] = { "PAGER=cat", NULL };
static const gchar *GIT_ENV_STATUS[] = { "PAGER=cat", NULL };
static const gchar *GIT_ENV_ADD[] = { "PAGER=cat", NULL };
static const gchar *GIT_ENV_REMOVE[] = { "PAGER=cat", NULL };
static const gchar *GIT_ENV_LOG_FILE[] = { "PAGER=cat", NULL };
static const gchar *GIT_ENV_LOG_DIR[] = { "PAGER=cat", NULL };
static const gchar *GIT_ENV_BLAME[] = { "PAGER=cat", NULL };

#define GIT_ENV_COMMIT NULL

static void *GIT_COMMANDS[VC_COMMAND_COUNT] = { GIT_CMD_DIFF_FILE,
	GIT_CMD_DIFF_DIR,
	GIT_CMD_REVERT_FILE,
	GIT_CMD_STATUS,
	GIT_CMD_ADD,
	GIT_CMD_REMOVE,
	GIT_CMD_LOG_FILE,
	GIT_CMD_LOG_DIR,
	GIT_CMD_COMMIT,
	GIT_CMD_BLAME,
	GIT_CMD_SHOW
};

static void *GIT_ENV[] = { GIT_ENV_DIFF_FILE,
	GIT_ENV_DIFF_DIR,
	GIT_ENV_REVERT_FILE,
	GIT_ENV_STATUS,
	GIT_ENV_ADD,
	GIT_ENV_REMOVE,
	GIT_ENV_LOG_FILE,
	GIT_ENV_LOG_DIR,
	GIT_ENV_COMMIT,
	GIT_ENV_BLAME,
	GIT_ENV_SHOW
};

static gboolean
in_vc_git(const gchar * filename)
{
	gint exit_code;
	gchar *argv[] = { "git", "ls-files", "--", NULL, NULL };
	gchar *dir;
	gchar *base_name;
	gboolean ret = FALSE;
	gchar *std_output;

	if (!find_dir(filename, ".git", TRUE))
		return FALSE;

	if (g_file_test(filename, G_FILE_TEST_IS_DIR))
		return TRUE;

	dir = g_path_get_dirname(filename);
	base_name = g_path_get_basename(filename);
	argv[3] = base_name;

	exit_code = execute_custom_command((const gchar **) argv, NULL, &std_output, NULL,
					   dir, NULL, NULL);
	if (NZV(std_output))
	{
		ret = TRUE;
		g_free(std_output);
	}

	g_free(base_name);
	g_free(dir);

	return ret;
}

static GSList *
parse_git_status(GSList * lst, const gchar * base_dir, const gchar * txt, const gchar * s_out,
		 const gchar * status)
{
	gchar *start, *end;
	gchar *base_name;
	gchar *filename;
	CommitItem *item;

	start = strstr(txt, s_out);
	while (start)
	{
		start += strlen(s_out);
		while (*start == ' ' || *start == '\t')
		{
			start++;
		}
		g_return_val_if_fail(*start, NULL);

		end = strchr(start, '\n');
		g_return_val_if_fail(start, NULL);

		base_name = g_malloc0(end - start + 1);
		memcpy(base_name, start, end - start);
		filename = g_build_filename(base_dir, base_name, NULL);
		g_free(base_name);

		item = g_new(CommitItem, 1);
		item->status = status;
		item->path = filename;

		lst = g_slist_append(lst, item);
		start = strstr(start, s_out);
	}
	return lst;
}

static GSList *
get_commit_files_git(const gchar * file)
{
	gint exit_code;
	gchar *argv[] = { "git", "status", NULL };
	gchar *env[] = { "PAGES=cat", NULL };
	gchar *std_out = NULL;
	gchar *base_dir = find_subdir_path(file, ".git");
	GSList *ret = NULL;

	g_return_val_if_fail(base_dir, NULL);

	exit_code =
		execute_custom_command((const gchar **) argv, (const gchar **) env, &std_out, NULL,
				       base_dir, NULL, NULL);
	g_return_val_if_fail(std_out, NULL);

	ret = parse_git_status(ret, base_dir, std_out, "modified:", FILE_STATUS_MODIFIED);
	ret = parse_git_status(ret, base_dir, std_out, "deleted:", FILE_STATUS_DELETED);
	ret = parse_git_status(ret, base_dir, std_out, "new file:", FILE_STATUS_ADDED);

	g_free(std_out);
	g_free(base_dir);

	return ret;
}

VC_RECORD VC_GIT = { GIT_COMMANDS, GIT_ENV, "git", in_vc_git, get_commit_files_git };
