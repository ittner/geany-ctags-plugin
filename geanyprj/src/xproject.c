/*
 *  geanyprj - Alternative project support for geany light IDE.
 *
 *  Copyright 2007 Frank Lanitz <frank(at)frank(dot)uvena(dot)de>
 *  Copyright 2007 Enrico Tröger <enrico.troeger@uvena.de>
 *  Copyright 2007 Nick Treleaven <nick.treleaven@btinternet.com>
 *  Copyright 2007,2008 Yura Siamashka <yurand2@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <sys/time.h>

#include "geany.h"
#include "support.h"
#include "prefs.h"
#include "plugindata.h"
#include "document.h"
#include "filetypes.h"
#include "utils.h"
#include "pluginmacros.h"

#include "project.h"

#include "geanyprj.h"

extern GeanyData *geany_data;
extern GeanyFunctions *geany_functions;


struct GeanyPrj *g_current_project = NULL;
static GPtrArray *g_projects = NULL;

static void
add_tag(G_GNUC_UNUSED gpointer key, gpointer value, G_GNUC_UNUSED gpointer user_data)
{
	debug("%s file=%s\n", __FUNCTION__, (const gchar *) key);
	p_tm->workspace_add_object((TMWorkObject *) value);
}

static void
remove_tag(G_GNUC_UNUSED gpointer key, gpointer value, G_GNUC_UNUSED gpointer user_data)
{
	debug("%s file=%s\n", __FUNCTION__, (const gchar *) key);
	p_tm->workspace_remove_object((TMWorkObject *) value, FALSE, FALSE);
}

/* This fonction should keep in sync with geany code */
void
xproject_close(gboolean cache)
{
	GeanyProject *project = geany->app->project;

	debug("%s\n", __FUNCTION__);
	g_return_if_fail(project != NULL);

	p_ui->set_statusbar(TRUE, _("Project \"%s\" closed."), project->name);

	g_free(project->name);
	g_free(project->description);
	g_free(project->file_name);
	g_free(project->base_path);
	g_free(project->run_cmd);

	g_free(project);
	project = NULL;

	if (!g_current_project)
		return;

	if (cache)
	{
		g_hash_table_foreach(g_current_project->tags, remove_tag, NULL);
		g_ptr_array_add(g_projects, g_current_project);
	}
	else
	{
		geany_project_free(g_current_project);
	}

	g_current_project = NULL;
	sidebar_refresh();
}


void
xproject_open(const gchar * path)
{
	GeanyProject *project = geany->app->project;

	guint i;
	struct GeanyPrj *p = NULL;
	debug("%s\n", __FUNCTION__);

	for (i = 0; i < g_projects->len; i++)
	{
		if (strcmp(path, ((struct GeanyPrj *) g_projects->pdata[i])->path) == 0)
		{
			p = (struct GeanyPrj *) g_projects->pdata[i];
			g_ptr_array_remove_index(g_projects, i);
			break;
		}
	}
	if (!p)
		p = geany_project_load(path);

	if (!p)
		return;

	p_ui->set_statusbar(TRUE, _("Project \"%s\" opened."), p->name);

	project = g_new0(struct GeanyProject, 1);
	project->type = PROJECT_TYPE;
	project->name = g_strdup(p->name);
	project->description = g_strdup(p->description);
	project->base_path = g_strdup(p->base_path);
	project->run_cmd = g_strdup(p->run_cmd);
	project->make_in_base_path = TRUE;

	g_hash_table_foreach(p->tags, add_tag, NULL);

	g_current_project = p;
	sidebar_refresh();
}

void
xproject_update_tag(const gchar * filename)
{
	guint i;
	TMWorkObject *tm_obj;

	if (g_current_project)
	{
		tm_obj = g_hash_table_lookup(g_current_project->tags, filename);
		if (tm_obj)
		{
			p_tm->source_file_update(tm_obj, TRUE, FALSE, TRUE);
		}
	}

	for (i = 0; i < g_projects->len; i++)
	{
		tm_obj = (TMWorkObject *)
			g_hash_table_lookup(((struct GeanyPrj *) (g_projects->pdata[i]))->tags,
					    filename);
		if (tm_obj)
		{
			p_tm->source_file_update(tm_obj, TRUE, FALSE, TRUE);
		}
	}
}

gboolean
xproject_add_file(const gchar * path)
{
	TMWorkObject *tm_obj;

	debug("%s path=%s\n", __FUNCTION__, path);

	if (!g_current_project)
		return FALSE;

	if (geany_project_add_file(g_current_project, path))
	{
		tm_obj = (TMWorkObject *) g_hash_table_lookup(g_current_project->tags, path);
		if (tm_obj)
		{
			p_tm->workspace_add_object((TMWorkObject *) tm_obj);
		}
		sidebar_refresh();
		return TRUE;
	}
	return FALSE;
}

gboolean
xproject_remove_file(const gchar * path)
{
	TMWorkObject *tm_obj;

	debug("%s path=%s\n", __FUNCTION__, path);

	if (!g_current_project)
		return FALSE;

	tm_obj = (TMWorkObject *) g_hash_table_lookup(g_current_project->tags, path);
	if (tm_obj)
	{
		p_tm->workspace_remove_object(tm_obj, FALSE, FALSE);
	}

	if (geany_project_remove_file(g_current_project, path))
	{
		sidebar_refresh();
		return TRUE;
	}
	return FALSE;
}

void
xproject_init()
{
	g_projects = g_ptr_array_sized_new(10);
	g_current_project = NULL;
}

void
xproject_cleanup()
{
	guint i;
	for (i = 0; i < g_projects->len; i++)
	{
		geany_project_free(((struct GeanyPrj *) (g_projects->pdata[i])));
	}
	g_ptr_array_free(g_projects, TRUE);
	g_projects = NULL;
}
