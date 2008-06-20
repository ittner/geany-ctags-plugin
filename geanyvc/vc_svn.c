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


static const gchar *SVN_CMD_DIFF_FILE[] = { "svn", "diff", "--non-interactive", FILENAME, NULL };
static const gchar *SVN_CMD_DIFF_DIR[] = { "svn", "diff", "--non-interactive", DIRNAME, NULL };
static const gchar *SVN_CMD_REVERT_FILE[] = { "svn", "revert", BASE_FILENAME, NULL };
static const gchar *SVN_CMD_STATUS[] = { "svn", "status", NULL };
static const gchar *SVN_CMD_ADD[] = { "svn", "add", BASE_FILENAME, NULL };
static const gchar *SVN_CMD_REMOVE[] = { "svn", "rm", BASE_FILENAME, NULL };
static const gchar *SVN_CMD_LOG_FILE[] = { "svn", "log", BASE_FILENAME, NULL };
static const gchar *SVN_CMD_LOG_DIR[] = { "svn", "log", DIRNAME, NULL };
static const gchar *SVN_CMD_COMMIT[] =
	{ "svn", "commit", "--non-interactive", "-m", MESSAGE, FILE_LIST, NULL };
static const gchar *SVN_CMD_BLAME[] = { "svn", "blame", BASE_FILENAME, NULL };
static const gchar *SVN_CMD_SHOW[] = { "svn", "cat", BASE_FILENAME, NULL };

static void *SVN_COMMANDS[] = { SVN_CMD_DIFF_FILE,
	SVN_CMD_DIFF_DIR,
	SVN_CMD_REVERT_FILE,
	SVN_CMD_STATUS,
	SVN_CMD_ADD,
	SVN_CMD_REMOVE,
	SVN_CMD_LOG_FILE,
	SVN_CMD_LOG_DIR,
	SVN_CMD_COMMIT,
	SVN_CMD_BLAME,
	SVN_CMD_SHOW
};

static gboolean
in_vc_svn(const gchar * filename)
{
	gint exit_code;
	gchar *argv[] = { "svn", "info", "--non-interactive", NULL, NULL };
	gchar *dir;
	gchar *base_name;
	gboolean ret = FALSE;
	gchar *std_output;

	if (!find_dir(filename, ".svn", FALSE))
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
get_commit_files_svn(const gchar * dir)
{
	enum
	{
		FIRST_CHAR,
		SKIP_SPACE,
		FILE_NAME,
	};

	gchar *txt;
	GSList *ret = NULL;
	gint pstatus = FIRST_CHAR;
	const gchar *p;
	gchar *base_name;
	const gchar *start = NULL;
	CommitItem *item;

	const gchar *status;
	gchar *filename;
	const char *argv[] = { "svn", "status", NULL };

	execute_custom_command(argv, NULL, &txt, NULL, dir, NULL, NULL);
	if (!NZV(txt))
		return NULL;
	p = txt;

	while (*p)
	{
		if (*p == '\r')
		{
		}
		else if (pstatus == FIRST_CHAR)
		{
			if (*p == '?')
				status = FILE_STATUS_UNKNOWN;
			else if (*p == 'M')
				status = FILE_STATUS_MODIFIED;
			else if (*p == 'D')
				status = FILE_STATUS_DELETED;
			else if (*p == 'A')
				status = FILE_STATUS_ADDED;
			pstatus = SKIP_SPACE;
		}
		else if (pstatus == SKIP_SPACE)
		{
			if (*p == ' ' || *p == '\t')
			{
			}
			else
			{
				start = p;
				pstatus = FILE_NAME;
			}
		}
		else if (pstatus == FILE_NAME)
		{
			if (*p == '\n')
			{
				if (status != FILE_STATUS_UNKNOWN)
				{
					base_name = g_malloc0(p - start + 1);
					memcpy(base_name, start, p - start);
					filename = g_build_filename(dir, base_name, NULL);
					g_free(base_name);
					item = g_new(CommitItem, 1);
					item->status = status;
					item->path = filename;
					ret = g_slist_append(ret, item);
				}
				pstatus = FIRST_CHAR;
			}
		}
		p++;
	}
	g_free(txt);
	return ret;
}

VC_RECORD VC_SVN = { SVN_COMMANDS, NO_ENV, "svn", in_vc_svn, get_commit_files_svn };
