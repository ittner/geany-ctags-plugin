/*
 *      filetree.c 
 * 
 * 		Robert McGinley - mcginleyr1(at)gmail(dot)com
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
 */


/* Sidebar file tree plugin. */

#include "geany.h"

#include <gdk/gdkkeysyms.h>

#include "support.h"
#include "prefs.h"
#include "document.h"
#include "utils.h"
#include "keybindings.h"
#include "project.h"

#include "plugindata.h"
#include "pluginmacros.h"


PluginFields	*plugin_fields;
GeanyData		*geany_data;


PLUGIN_VERSION_CHECK(26)

PLUGIN_INFO(_("File Tree"), _("Adds a file tree tab to the sidebar."), VERSION,
	_("The Geany developer team"))


/* Keybinding(s) */
enum
{
	KB_FOCUS_FILE_LIST,
	KB_COUNT
};

PLUGIN_KEY_GROUP(file_tree, KB_COUNT)


/* number of characters to skip the root of an absolute path("c:" or "d:" on Windows) */
#ifdef G_OS_WIN32
# define ROOT_OFFSET 2
#else
# define ROOT_OFFSET 0
#endif

enum
{
	FILEVIEW_COLUMN_ICON = 0,
	FILEVIEW_COLUMN_NAME,
	FILEVIEW_N_COLUMNS
};

static gboolean show_hidden_files = FALSE;
static gboolean hide_object_files = TRUE;


static GtkWidget			*file_view_vbox;
static GtkWidget			*file_view;
static GtkTreeStore			*file_store;
static GtkTreeIter			*toplevel, *child;

static gchar		*open_cmd;				/* in locale-encoding */
static gchar		*config_file;

static struct
{
	GtkWidget *open;
	GtkWidget *open_external;
	GtkWidget *find_in_files;
} popup_items;

/* Returns: whether name should be hidden. */
static gboolean check_hidden(const gchar *base_name)
{
	gsize len;

	if (! NZV(base_name))
		return FALSE;

	if (base_name[0] == '.')
		return TRUE;

	len = strlen(base_name);
	if (base_name[len - 1] == '~')
		return TRUE;

	if (hide_object_files)
	{
		const gchar *exts[] = {".o", ".obj", ".so", ".dll", ".a", ".lib"};
		guint i;

		for (i = 0; i < G_N_ELEMENTS(exts); i++)
		{
			const gchar *ext = exts[i];

			if (p_utils->str_equal(&base_name[len - strlen(ext)], ext))
				return TRUE;
		}
	}
	return FALSE;
}


/* name is in locale encoding */
static void add_item(const gchar *name)
{
	GtkTreeIter iter;
	gchar *fname, *utf8_name;
	gboolean dir;

	if (! show_hidden_files && check_hidden(name))
		return;

	fname = g_strconcat(current_dir, G_DIR_SEPARATOR_S, name, NULL);
	dir = g_file_test(fname, G_FILE_TEST_IS_DIR);
	g_free(fname);

	if (dir)
	{
		if (last_dir_iter == NULL)
			gtk_list_store_prepend(file_store, &iter);
		else
		{
			gtk_list_store_insert_after(file_store, &iter, last_dir_iter);
			gtk_tree_iter_free(last_dir_iter);
		}
		last_dir_iter = gtk_tree_iter_copy(&iter);
	}
	else
		gtk_list_store_append(file_store, &iter);

	utf8_name = p_utils->get_utf8_from_locale(name);

	gtk_list_store_set(file_store, &iter,
		FILEVIEW_COLUMN_ICON, (dir) ? GTK_STOCK_DIRECTORY : GTK_STOCK_FILE,
		FILEVIEW_COLUMN_NAME, utf8_name, -1);
	g_free(utf8_name);
}


static void clear(void)
{
	gtk_list_store_clear(file_store);

	/* reset the directory item pointer */
	if (last_dir_iter != NULL)
		gtk_tree_iter_free(last_dir_iter);
	last_dir_iter = NULL;
}


/* recreate the tree model from current_dir. */
static void refresh(void)
{
	gchar *utf8_dir;
	GSList *list;

	/* don't clear when the new path doesn't exist */
	if (! g_file_test(current_dir, G_FILE_TEST_EXISTS))
		return;

	clear();


	list = p_utils->get_file_list(current_dir, NULL, NULL);
	if (list != NULL)
	{
		add_top_level_entry();
		g_slist_foreach(list, (GFunc) add_item, NULL);
		g_slist_foreach(list, (GFunc) g_free, NULL);
		g_slist_free(list);
	}
   	gtk_entry_completion_set_model(entry_completion, GTK_TREE_MODEL(file_store));
}


static void on_expand_all(void)
{
	
}

static void on_collapse_all(void)
{
	
}


static gchar *get_default_dir(void)
{
	const gchar *dir = NULL;

	if (project)
		dir = project->base_path;
	if (NZV(dir))
		return p_utils->get_locale_from_utf8(dir);

	return g_get_current_dir();
}


static void on_current_path(void)
{
	gchar *fname;
	gchar *dir;
	gint idx = p_document->get_cur_idx();

	if (! DOC_IDX_VALID(idx) || doc_list[idx].file_name == NULL ||
		! g_path_is_absolute(doc_list[idx].file_name))
	{
		setptr(current_dir, get_default_dir());
		refresh();
		return;
	}
	fname = doc_list[idx].file_name;
	fname = p_utils->get_locale_from_utf8(fname);
	dir = g_path_get_dirname(fname);
	g_free(fname);

	setptr(current_dir, dir);
	refresh();
}


static gboolean check_single_selection(GtkTreeSelection *treesel)
{
	if (gtk_tree_selection_count_selected_rows(treesel) == 1)
		return TRUE;

	p_ui->set_statusbar(FALSE, _("Too many items selected!"));
	return FALSE;
}


/* Returns: TRUE if at least one of selected_items is a folder. */
static gboolean is_folder_selected(GList *selected_items)
{
	GList *item;
	GtkTreeModel *model = GTK_TREE_MODEL(file_store);
	gboolean dir_found = FALSE;

	for (item = selected_items; item != NULL; item = g_list_next(item))
	{
		gchar *icon;
		GtkTreeIter iter;
		GtkTreePath *treepath;

		treepath = (GtkTreePath*) item->data;
		gtk_tree_model_get_iter(model, &iter, treepath);
		gtk_tree_model_get(model, &iter, FILEVIEW_COLUMN_ICON, &icon, -1);

		if (p_utils->str_equal(icon, GTK_STOCK_DIRECTORY))
		{
			dir_found = TRUE;
			g_free(icon);
			break;
		}
		g_free(icon);
	}
	return dir_found;
}


/* Returns: the full filename in locale encoding. */
static gchar *get_tree_path_filename(GtkTreePath *treepath)
{
	GtkTreeModel *model = GTK_TREE_MODEL(file_store);
	GtkTreeIter iter;
	gchar *name, *fname;

	gtk_tree_model_get_iter(model, &iter, treepath);
	gtk_tree_model_get(model, &iter, FILEVIEW_COLUMN_NAME, &name, -1);

	if (p_utils->str_equal(name, ".."))
	{
		fname = g_path_get_dirname(current_dir);
	}
	else
	{
		setptr(name, p_utils->get_locale_from_utf8(name));
		fname = g_build_filename(current_dir, name, NULL);
	}
	g_free(name);

	return fname;
}


static void open_external(const gchar *fname, gboolean dir_found)
{
	gchar *cmd;
	gchar *locale_cmd;
	gchar *dir;
	GString *cmd_str = g_string_new(open_cmd);
	GError *error = NULL;

	if (! dir_found)
		dir = g_path_get_dirname(fname);
	else
		dir = g_strdup(fname);

	p_utils->string_replace_all(cmd_str, "%f", fname);
	p_utils->string_replace_all(cmd_str, "%d", dir);

	cmd = g_string_free(cmd_str, FALSE);
	locale_cmd = p_utils->get_locale_from_utf8(cmd);
	if (! g_spawn_command_line_async(locale_cmd, &error))
	{
		gchar *c = strchr(cmd, ' ');

		if (c != NULL)
			*c = '\0';
		p_ui->set_statusbar(TRUE,
			_("Could not execute configured external command '%s' (%s)."),
			cmd, error->message);
		g_error_free(error);
	}
	g_free(locale_cmd);
	g_free(cmd);
	g_free(dir);
}


static void on_external_open(GtkMenuItem *menuitem, gpointer user_data)
{
	GtkTreeSelection *treesel;
	GtkTreeModel *model;
	GList *list;
	gboolean dir_found;

	treesel = gtk_tree_view_get_selection(GTK_TREE_VIEW(file_view));

	list = gtk_tree_selection_get_selected_rows(treesel, &model);
	dir_found = is_folder_selected(list);

	if (! dir_found || check_single_selection(treesel))
	{
		GList *item;

		for (item = list; item != NULL; item = g_list_next(item))
		{
			GtkTreePath *treepath = item->data;
			gchar *fname = get_tree_path_filename(treepath);

			open_external(fname, dir_found);
			g_free(fname);
		}
	}

	g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);
	g_list_free(list);
}


/* We use p_document->open_files() as it's more efficient. */
static void open_selected_files(GList *list)
{
	GSList *files = NULL;
	GList *item;

	for (item = list; item != NULL; item = g_list_next(item))
	{
		GtkTreePath *treepath = item->data;
		gchar *fname = get_tree_path_filename(treepath);

		files = g_slist_append(files, fname);
	}
	p_document->open_files(files, FALSE, NULL, NULL);
	g_slist_foreach(files, (GFunc) g_free, NULL);	/* free filenames */
	g_slist_free(files);
}


static void open_folder(GtkTreePath *treepath)
{
	gchar *fname = get_tree_path_filename(treepath);

	setptr(current_dir, fname);
	refresh();
}


static void on_open_clicked(GtkMenuItem *menuitem, gpointer user_data)
{
	GtkTreeSelection *treesel;
	GtkTreeModel *model;
	GList *list;
	gboolean dir_found;

	treesel = gtk_tree_view_get_selection(GTK_TREE_VIEW(file_view));

	list = gtk_tree_selection_get_selected_rows(treesel, &model);
	dir_found = is_folder_selected(list);

	if (dir_found)
	{
		if (check_single_selection(treesel))
		{
			GtkTreePath *treepath = list->data;	/* first selected item */

			expand_folder(treepath);
		}
	}
	else
		open_selected_files(list);

	g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);
	g_list_free(list);
}


static void on_hidden_files_clicked(GtkCheckMenuItem *item)
{
	show_hidden_files = gtk_check_menu_item_get_active(item);
	refresh();
}


static void on_hide_sidebar(void)
{
	p_keybindings->send_command(GEANY_KEY_GROUP_VIEW, GEANY_KEYS_VIEW_SIDEBAR);
}


static GtkWidget *create_popup_menu(void)
{
	GtkWidget *item, *menu, *image;

	menu = gtk_menu_new();

	item = gtk_image_menu_item_new_from_stock(GTK_STOCK_OPEN, NULL);
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(menu), item);
	g_signal_connect((gpointer) item, "activate",
		G_CALLBACK(on_open_clicked), NULL);
	popup_items.open = item;

	image = gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU);
	gtk_widget_show(image);
	item = gtk_image_menu_item_new_with_mnemonic(_("Open _externally"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), image);
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(menu), item);
	g_signal_connect((gpointer) item, "activate",
		G_CALLBACK(on_external_open), NULL);
	popup_items.open_external = item;




	item = gtk_separator_menu_item_new();
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(menu), item);

	item = gtk_check_menu_item_new_with_mnemonic(_("Show _Hidden Files"));
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(menu), item);
	g_signal_connect((gpointer) item, "activate", G_CALLBACK(on_hidden_files_clicked), NULL);

	item = gtk_separator_menu_item_new();
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(menu), item);

	item = gtk_image_menu_item_new_with_mnemonic(_("H_ide Sidebar"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
		gtk_image_new_from_stock("gtk-close", GTK_ICON_SIZE_MENU));
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(menu), item);
	g_signal_connect((gpointer) item, "activate", G_CALLBACK(on_hide_sidebar), NULL);

	return menu;
}


static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	if (event->button == 1 && event->type == GDK_2BUTTON_PRESS)
		on_open_clicked(NULL, NULL);
	return FALSE;
}


static void update_popup_menu(GtkWidget *popup_menu)
{
	GtkTreeSelection *treesel = gtk_tree_view_get_selection(GTK_TREE_VIEW(file_view));
	gboolean have_sel = (gtk_tree_selection_count_selected_rows(treesel) > 0);
	gboolean multi_sel = (gtk_tree_selection_count_selected_rows(treesel) > 1);

	gtk_widget_set_sensitive(popup_items.open, have_sel);
	gtk_widget_set_sensitive(popup_items.open_external, have_sel);
	gtk_widget_set_sensitive(popup_items.find_in_files, have_sel && ! multi_sel);
}


/* delay updating popup menu until the selection has been set */
static gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	if (event->button == 3)
	{
		static GtkWidget *popup_menu = NULL;

		if (popup_menu == NULL)
			popup_menu = create_popup_menu();

		update_popup_menu(popup_menu);

		gtk_menu_popup(GTK_MENU(popup_menu), NULL, NULL, NULL, NULL,
			event->button, event->time);
	}
	return FALSE;
}


static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	if (event->keyval == GDK_Return
		|| event->keyval == GDK_ISO_Enter
		|| event->keyval == GDK_KP_Enter
		|| event->keyval == GDK_space)
		on_open_clicked(NULL, NULL);

	if ((event->keyval == GDK_Up ||
		event->keyval == GDK_KP_Up) &&
		(event->state & GDK_MOD1_MASK))	/* FIXME: Alt-Up doesn't seem to work! */
		on_go_up();
	return FALSE;
}



static void prepare_file_view(void)
{
	GtkCellRenderer *text_renderer, *icon_renderer, *expand_renderer;
	GtkTreeViewColumn *expandColumn *column;
	GtkTreeSelection *select;
	PangoFontDescription *pfd;

	file_store = gtk_tree_store_new(FILEVIEW_N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);

	//gtk_tree_view_set_model(GTK_TREE_VIEW(file_view), GTK_TREE_MODEL(file_store));

//	icon_renderer = gtk_cell_renderer_pixbuf_new();
//	text_renderer = gtk_cell_renderer_text_new();
//	column = gtk_tree_view_column_new();
	
	
	
//	gtk_tree_view_column_pack_start(column, NULL , FALSE);
//	gtk_tree_view_set_expander_column(column);
	
	
//    gtk_tree_view_column_pack_start(column, icon_renderer, FALSE);
 // 	gtk_tree_view_column_set_attributes(column, icon_renderer, "stock-id", FILEVIEW_COLUMN_ICON, NULL);
  //  gtk_tree_view_column_pack_start(column, text_renderer, TRUE);
 // 	gtk_tree_view_column_set_attributes(column, text_renderer, "text", FILEVIEW_COLUMN_NAME, NULL);
//	gtk_tree_view_append_column(GTK_TREE_VIEW(file_view), column);
//	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(file_view), FALSE);

//	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(file_view), TRUE);
//	gtk_tree_view_set_search_column(GTK_TREE_VIEW(file_view), FILEVIEW_COLUMN_NAME);

//	pfd = pango_font_description_from_string(prefs->tagbar_font);
//	gtk_widget_modify_font(file_view, pfd);
//	pango_font_description_free(pfd);

	/* selection handling */
//	select = gtk_tree_view_get_selection(GTK_TREE_VIEW(file_view));
//	gtk_tree_selection_set_mode(select, GTK_SELECTION_MULTIPLE);

//	g_signal_connect(G_OBJECT(file_view), "realize", G_CALLBACK(on_current_path), NULL);
	g_signal_connect(G_OBJECT(file_view), "button-press-event",
		G_CALLBACK(on_button_press), NULL);
	g_signal_connect(G_OBJECT(file_view), "button-release-event",
		G_CALLBACK(on_button_release), NULL);
	g_signal_connect(G_OBJECT(file_view), "key-press-event",
		G_CALLBACK(on_key_press), NULL);
}


static GtkWidget *make_toolbar(void)
{
	GtkWidget *wid, *toolbar;
	GtkTooltips *tooltips = GTK_TOOLTIPS(p_support->lookup_widget(
		app->window, "tooltips"));

	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_MENU);
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);

	wid = (GtkWidget *) gtk_tool_button_new_from_stock(GTK_STOCK_GO_DOWN);
	gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(wid), tooltips,
		_("Expand All"), NULL);
	g_signal_connect(G_OBJECT(wid), "clicked", G_CALLBACK(on_expand_all), NULL);
	gtk_container_add(GTK_CONTAINER(toolbar), wid);

	wid = (GtkWidget *) gtk_tool_button_new_from_stock(GTK_STOCK_GO_UP);
	gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(wid), tooltips,
		_("Collapse All"), NULL);
	g_signal_connect(G_OBJECT(wid), "clicked", G_CALLBACK(on_collapse_all), NULL);
	gtk_container_add(GTK_CONTAINER(toolbar), wid);

	return toolbar;
}

#define CHECK_READ_SETTING(var, error, tmp) \
	if ((error) != NULL) \
	{ \
		g_error_free((error)); \
		(error) = NULL; \
	} \
	else \
		(var) = (tmp);

static void load_settings(void)
{
	GKeyFile *config = g_key_file_new();
	GError *error = NULL;
	gboolean tmp;

	config_file = g_strconcat(app->configdir, G_DIR_SEPARATOR_S, "plugins", G_DIR_SEPARATOR_S,
		"filetree", G_DIR_SEPARATOR_S, "filetree.conf", NULL);
	g_key_file_load_from_file(config, config_file, G_KEY_FILE_NONE, NULL);
	open_cmd = g_key_file_get_string(config, "filetree", "open_command", &error);
	if (error != NULL)
	{
		open_cmd = g_strdup("nautilus \"%d\"");
		g_error_free(error);
		error = NULL;
	}
	tmp = g_key_file_get_boolean(config, "filetree", "show_hidden_files", &error);
	CHECK_READ_SETTING(show_hidden_files, error, tmp);
	tmp = g_key_file_get_boolean(config, "filetree", "hide_object_files", &error);
	CHECK_READ_SETTING(hide_object_files, error, tmp);

	g_key_file_free(config);
}


static void kb_activate(guint key_id)
{
	switch (key_id)
	{
		case KB_FOCUS_FILE_LIST:
			gtk_widget_grab_focus(file_view);
			break;
	}
}


void init(GeanyData *data)
{
	GtkWidget *scrollwin, *toolbar;

	file_view_vbox = gtk_vbox_new(FALSE, 0);
	toolbar = make_toolbar();
	gtk_box_pack_start(GTK_BOX(file_view_vbox), toolbar, FALSE, FALSE, 0);

	file_view = gtk_tree_view_new();
//	prepare_file_view();

	scrollwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(
		GTK_SCROLLED_WINDOW(scrollwin),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrollwin), file_view);
	gtk_container_add(GTK_CONTAINER(file_view_vbox), scrollwin);

	gtk_widget_show_all(file_view_vbox);
	gtk_notebook_append_page(GTK_NOTEBOOK(app->treeview_notebook), file_view_vbox,
		gtk_label_new(_("Files")));

	load_settings();

	/* setup keybindings */
	p_keybindings->set_item(plugin_key_group, KB_FOCUS_FILE_LIST, kb_activate,
		0, 0, "focus_file_list", _("Focus File List"), NULL);
}


void configure(GtkWidget *parent)
{
	GtkWidget *dialog, *label, *entry, *checkbox_of, *checkbox_hf, *vbox;
	GtkTooltips *tooltips = gtk_tooltips_new();

	dialog = gtk_dialog_new_with_buttons(_("File Browser"),
		GTK_WINDOW(parent), GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
	vbox = p_ui->dialog_vbox_new(GTK_DIALOG(dialog));
	gtk_widget_set_name(dialog, "GeanyDialog");
	gtk_box_set_spacing(GTK_BOX(vbox), 6);

	label = gtk_label_new(_("External open command:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_container_add(GTK_CONTAINER(vbox), label);

	entry = gtk_entry_new();
	gtk_widget_show(entry);
	if (open_cmd != NULL)
		gtk_entry_set_text(GTK_ENTRY(entry), open_cmd);
	gtk_tooltips_set_tip(tooltips, entry,
		_("The command to execute when using \"Open with\". You can use %f and %d wildcards.\n"
		  "%f will be replaced with the filename including full path\n"
		  "%d will be replaced with the path name of the selected file without the filename"),
		  NULL);
	gtk_container_add(GTK_CONTAINER(vbox), entry);

	checkbox_hf = gtk_check_button_new_with_label(_("Show hidden files"));
	gtk_button_set_focus_on_click(GTK_BUTTON(checkbox_hf), FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox_hf), show_hidden_files);
	gtk_box_pack_start(GTK_BOX(vbox), checkbox_hf, FALSE, FALSE, 5);

	checkbox_of = gtk_check_button_new_with_label(_("Hide object files"));
	gtk_button_set_focus_on_click(GTK_BUTTON(checkbox_of), FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox_of), hide_object_files);
	gtk_tooltips_set_tip(tooltips, checkbox_of,
		_("Don't show generated object files in the file browser, this includes "
		  "*.o, *.obj. *.so, *.dll, *.a, *.lib"),
		  NULL);
	gtk_box_pack_start(GTK_BOX(vbox), checkbox_of, FALSE, FALSE, 5);


	gtk_widget_show_all(vbox);

	/* run the dialog and check for the response code */
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		GKeyFile *config = g_key_file_new();
		gchar *data;
		gchar *config_dir = g_path_get_dirname(config_file);

		g_free(open_cmd);
		open_cmd = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
		show_hidden_files = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbox_hf));
		hide_object_files = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbox_of));

		g_key_file_load_from_file(config, config_file, G_KEY_FILE_NONE, NULL);

		g_key_file_set_string(config, "filebrowser", "open_command", open_cmd);
		g_key_file_set_boolean(config, "filebrowser", "show_hidden_files", show_hidden_files);
		g_key_file_set_boolean(config, "filebrowser", "hide_object_files", hide_object_files);

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

		/* apply the changes */
		refresh();

		g_free(config_dir);
		g_key_file_free(config);
	}
	gtk_widget_destroy(dialog);
}


void cleanup(void)
{
	g_free(config_file);
	g_free(open_cmd);
	gtk_widget_destroy(file_view_vbox);
	g_object_unref(G_OBJECT(entry_completion));
}
