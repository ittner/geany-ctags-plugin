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


static const gchar *BZR_CMD_DIFF_FILE[] = { "bzr", "diff", BASE_FILENAME, NULL };
static const gchar *BZR_CMD_DIFF_DIR[] = { "bzr", "diff", DIRNAME, NULL };
static const gchar *BZR_CMD_REVERT_FILE[] = { "bzr", "revert", BASE_FILENAME, NULL };
static const gchar *BZR_CMD_STATUS[] = { "bzr", "status", NULL };
static const gchar *BZR_CMD_ADD[] = { "bzr", "add", BASE_FILENAME, NULL };
static const gchar *BZR_CMD_REMOVE[] = { "bzr", "remove", BASE_FILENAME, NULL };
static const gchar *BZR_CMD_LOG_FILE[] = { "bzr", "log", BASE_FILENAME, NULL };
static const gchar *BZR_CMD_LOG_DIR[] = { "bzr", "log", DIRNAME, NULL };
static const gchar *BZR_CMD_COMMIT[] = { "bzr", "commit", "-m", MESSAGE, FILE_LIST, NULL };
static const gchar *BZR_CMD_BLAME[] = { "bzr", "blame", "--all", "--long", BASE_FILENAME, NULL };
static const gchar *BZR_CMD_SHOW[] = { "bzr", "cat", BASE_FILENAME, NULL };

static void *BZR_COMMANDS[VC_COMMAND_COUNT] = { BZR_CMD_DIFF_FILE,
	BZR_CMD_DIFF_DIR,
	BZR_CMD_REVERT_FILE,
	BZR_CMD_STATUS,
	BZR_CMD_ADD,
	BZR_CMD_REMOVE,
	BZR_CMD_LOG_FILE,
	BZR_CMD_LOG_DIR,
	BZR_CMD_COMMIT,
	BZR_CMD_BLAME,
	BZR_CMD_SHOW
};

static gboolean
in_vc_bzr(const gchar * filename)
{
	gint exit_code;
	gchar *argv[] = { "bzr", "ls", NULL, NULL };
	gchar *dir;
	gchar *base_name;
	gboolean ret = FALSE;
	gchar *std_output;

	if (!find_dir(filename, ".bzr", TRUE))
		return FALSE;

	if (g_file_test(filename, G_FILE_TEST_IS_DIR))
		return TRUE;

	dir = g_path_get_dirname(filename);
	base_name = g_path_get_basename(filename);
	argv[2] = base_name;

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

/* parse "bzr status --short" output, see "bzr help status-flags" for details */
static GSList *
get_commit_files_bzr(const gchar * dir)
{
	enum
	{
		FIRST_CHAR,
		SECOND_CHAR,
		THIRD_CHAR,
		SKIP_SPACE,
		FILE_NAME,
	};

	gchar *txt;
	GSList *ret = NULL;
	gint pstatus = FIRST_CHAR;
	const gchar *p;
	gchar *base_name;
	gchar *base_dir = find_subdir_path(dir, ".bzr");
	const gchar *start = NULL;
	CommitItem *item;

	const gchar *status;
	gchar *filename;
	const char *argv[] = { "bzr", "status", "--short", NULL };

	g_return_val_if_fail(base_dir, NULL);

	execute_custom_command(argv, NULL, &txt, NULL, base_dir, NULL, NULL);
	if (!NZV(txt))
	{
		g_free(base_dir);
		g_free(txt);
		return NULL;
	}
	p = txt;

	while (*p)
	{
		if (*p == '\r')
		{
		}
		else if (pstatus == FIRST_CHAR)
		{
			if (*p == '+')
				status = FILE_STATUS_ADDED;
			else if (*p == '-')
				status = FILE_STATUS_DELETED;
			// rename
			//else if (*p == 'R')
			else if (*p == '?')
				status = FILE_STATUS_UNKNOWN;
			// conflicts
			//else if (*p == 'C')
			// pending merge
			//else if (*p == 'P')
			pstatus = SECOND_CHAR;
		}
		else if (pstatus == SECOND_CHAR)
		{
			if (*p == 'N')
				status = FILE_STATUS_ADDED;
			else if (*p == 'D')
				status = FILE_STATUS_DELETED;
			// file kind changed
			//else if (*p == 'K')
			else if (*p == 'M')
				status = FILE_STATUS_MODIFIED;
			pstatus = THIRD_CHAR;
		}
		else if (pstatus == THIRD_CHAR)
		{
			// execute bit change
			//if (*p == '*')
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
					filename = g_build_filename(base_dir, base_name, NULL);
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
	g_free(base_dir);
	return ret;
}

VC_RECORD VC_BZR = { BZR_COMMANDS, NO_ENV, "bzr", in_vc_bzr, get_commit_files_bzr };
