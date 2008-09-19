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

#include "geany.h"
#include "support.h"
#include "plugindata.h"
#include "document.h"
#include "filetypes.h"
#include "utils.h"
#include "project.h"
#include "pluginmacros.h"

#include "geanyvc.h"

static const gchar *CVS_CMD_DIFF_FILE[] = { "cvs", "diff", "-u", BASE_FILENAME, NULL };
static const gchar *CVS_CMD_DIFF_DIR[] = { "cvs", "diff", "-u", NULL };
static const gchar *CVS_CMD_REVERT_FILE[] = { "cvs", "update", "-C", BASE_FILENAME, NULL };
static const gchar *CVS_CMD_STATUS[] = { "cvs", "status", NULL };
static const gchar *CVS_CMD_ADD[] = { "cvs", "add", BASE_FILENAME, NULL };
static const gchar *CVS_CMD_REMOVE[] = { "cvs", "remove", BASE_FILENAME, NULL };
static const gchar *CVS_CMD_LOG_FILE[] = { "cvs", "log", BASE_FILENAME, NULL };
static const gchar *CVS_CMD_LOG_DIR[] = { "cvs", "log", NULL };
static const gchar *CVS_CMD_COMMIT[] = { "cvs", NULL };
static const gchar *CVS_CMD_BLAME[] = { "cvs", "annotate", BASE_FILENAME, NULL };
static const gchar *CVS_CMD_SHOW[] = { "cvs", NULL };

static const VC_COMMAND commands[] = {
	{
	 .startdir = VC_COMMAND_STARTDIR_FILE,
	 .command = CVS_CMD_DIFF_FILE,
	 .env = NULL,
	 .function = NULL},
	{
	 .startdir = VC_COMMAND_STARTDIR_FILE,
	 .command = CVS_CMD_DIFF_DIR,
	 .env = NULL,
	 .function = NULL},
	{
	 .startdir = VC_COMMAND_STARTDIR_FILE,
	 .command = CVS_CMD_REVERT_FILE,
	 .env = NULL,
	 .function = NULL},
	{
	 .startdir = VC_COMMAND_STARTDIR_FILE,
	 .command = CVS_CMD_STATUS,
	 .env = NULL,
	 .function = NULL},
	{
	 .startdir = VC_COMMAND_STARTDIR_FILE,
	 .command = CVS_CMD_ADD,
	 .env = NULL,
	 .function = NULL},
	{
	 .startdir = VC_COMMAND_STARTDIR_FILE,
	 .command = CVS_CMD_REMOVE,
	 .env = NULL,
	 .function = NULL},
	{
	 .startdir = VC_COMMAND_STARTDIR_FILE,
	 .command = CVS_CMD_LOG_FILE,
	 .env = NULL,
	 .function = NULL},
	{
	 .startdir = VC_COMMAND_STARTDIR_FILE,
	 .command = CVS_CMD_LOG_DIR,
	 .env = NULL,
	 .function = NULL},
	{
	 .startdir = VC_COMMAND_STARTDIR_FILE,
	 .command = CVS_CMD_COMMIT,
	 .env = NULL,
	 .function = NULL},
	{
	 .startdir = VC_COMMAND_STARTDIR_FILE,
	 .command = CVS_CMD_BLAME,
	 .env = NULL,
	 .function = NULL},
	{
	 .startdir = VC_COMMAND_STARTDIR_FILE,
	 .command = CVS_CMD_SHOW,
	 .env = NULL,
	 .function = NULL}
};

static gchar *
get_base_dir(const gchar * path)
{
	return find_subdir_path(path, "CVS");
}

static gboolean
in_vc_cvs(const gchar * filename)
{
	return find_dir(filename, "CVS", FALSE);
}

VC_RECORD VC_CVS = {
	.commands = commands,
	.program = "cvs",
	.get_base_dir = get_base_dir,
	.in_vc = in_vc_cvs,
	.get_commit_files = get_commit_files_null,
};
