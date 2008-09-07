
/*
 * gdb-io-envir.c - Environment settings for GDB wrapper library. 
 *
 * See the file "gdb-io.h" for license information.
 *
 */


#include <string.h>
#include <glib.h>
#include "gdb-io-priv.h"


static GdbEnvironFunc gdbio_environ_func = NULL;

static GdbEnvironInfo env_info = { NULL, NULL, NULL, NULL };


static void
free_env_info()
{
	g_free(env_info.cwd);
	g_free(env_info.path);
	g_free(env_info.args);
	g_free(env_info.dirs);
	memset(&env_info, '\0', sizeof(env_info));
}



static gchar *
unquote(gchar * quoted)
{
	gint len = quoted ? strlen(quoted) : 0;
	if (len && (quoted[0] == '"') && (quoted[len - 1] == '"'))
	{
		gchar *tmp = g_strndup(quoted + 1, len - 2);
		gchar *rv = g_strcompress(tmp);
		g_free(tmp);
		return rv;
	}
	else
		return NULL;
}



static void
get_env_args(gint seq, gchar ** list, gchar * resp)
{
	gchar *args;
	gint i;
	gdbio_pop_seq(seq);
	for (i = 0; list[i]; i++)
	{
		if (strncmp(list[i], "~\"", 2) == 0)
		{
			args = unquote(list[i] + 1);
			if (args && *args)
			{
				g_strstrip(args);
				gchar *quote = strchr(args, '"');
				if (quote)
				{
					memmove(args, quote + 1, strlen(quote));
					quote = strrchr(args, '"');
					if (quote && g_str_equal(quote, "\"."))
					{
						*quote = '\0';
						break;
					}
				}
			}
			g_free(args);
			args = NULL;
		}
	}
	env_info.args = args;
	if (gdbio_environ_func)
	{
		gdbio_environ_func(&env_info);
	}
}



static void
get_env_dirs(gint seq, gchar ** list, gchar * resp)
{
	GHashTable *h = gdbio_get_results(resp, list);
	HSTR(h, source_path);
	gdbio_pop_seq(seq);
	if (source_path)
	{
		gchar *p;
		env_info.dirs = g_strdup(source_path);
		p = strstr(env_info.dirs, "$cdir:$cwd");
		if (p)
		{
			memmove(p, p + 10, strlen(p + 10) + 1);
		}
		p = strchr(env_info.dirs, '\0');
		if (p)
		{
			while (p > env_info.dirs)
			{
				p--;
				if (*p == ':')
				{
					*p = '\0';
				}
				else
				{
					break;
				}
			}
		}
	}
	else
	{
		gdbio_info_func("Failed to retrieve source search path setting from GDB.");
//    gdblx_dump_table(h);
	}
	if (h)
		g_hash_table_destroy(h);
	gdbio_send_seq_cmd(get_env_args, "show args\n");
}


static void
get_env_path(gint seq, gchar ** list, gchar * resp)
{
	GHashTable *h = gdbio_get_results(resp, list);
	HSTR(h, path);
	gdbio_pop_seq(seq);
	if (path)
	{
		env_info.path = g_strdup(path);
	}
	else
	{
		gdbio_info_func("Failed to retrieve executable search path setting from GDB.");
//    gdblx_dump_table(h);
	}
	if (h)
		g_hash_table_destroy(h);
	gdbio_send_seq_cmd(get_env_dirs, "-environment-directory\n");
}


static void
get_env_cwd(gint seq, gchar ** list, gchar * resp)
{
	GHashTable *h = gdbio_get_results(resp, list);
	HSTR(h, cwd);
	gdbio_pop_seq(seq);
	free_env_info();
	if (cwd)
	{
		env_info.cwd = g_strdup(cwd);
	}
	else
	{
		gdbio_info_func("Failed to retrieve working directory setting from GDB.");
//    gdblx_dump_table(h);
	}
	if (h)
		g_hash_table_destroy(h);
	gdbio_send_seq_cmd(get_env_path, "-environment-path\n");
}


void
gdbio_get_env(GdbEnvironFunc func)
{
	gdbio_environ_func = func;
	if (func)
	{
		gdbio_send_seq_cmd(get_env_cwd, "-environment-pwd\n");
	}
}
