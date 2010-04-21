/*
 *  
 *  Copyright © 2010 Colomban "Ban" Wendling <ban@herbesfolles.org>
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
 *  
 */

#include "ggd-plugin.h"

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gdk/gdkkeysyms.h> /* for the key bindings */
#include <ctpl/ctpl.h>
#include <geanyplugin.h>

#include "ggd.h"
#include "ggd-file-type.h"
#include "ggd-file-type-manager.h"
#include "ggd-tag-utils.h"
#include "ggd-options.h"



/*
 * Questions:
 *  * how to update tag list? (tm_source_file_buffer_update() is not found in
 *    symbols table)
 *  * how to know click position for the popup menu to appear?
 */

/* These items are set by Geany before plugin_init() is called. */
GeanyPlugin     *geany_plugin;
GeanyData       *geany_data;
GeanyFunctions  *geany_functions;

/* TODO check minimum requierment */
PLUGIN_VERSION_CHECK (172)

PLUGIN_SET_INFO (GGD_PLUGIN_NAME,
                 _("Generates documentation comments basis from source code"),
                 VERSION,
                 "Colomban \"Ban\" Wendling <ban@herbesfolles.org>")

enum
{
  KB_INSERT,
  NUM_KB
};

PLUGIN_KEY_GROUP (GGD_PLUGIN_ONAME, NUM_KB)

typedef struct _PluginData
{
  GgdOptGroup *config;
  
  GtkWidget  *separator_item;
  GtkWidget  *edit_menu_item;
  GtkWidget  *tools_menu_item;
  gulong      edit_menu_item_hid;
} PluginData;

#define plugin (&plugin_data)
static PluginData plugin_data = {
  NULL, NULL, NULL, NULL, 0l
};

static gchar     *OPT_doctype         = NULL;
static gboolean   OPT_save_to_refresh = FALSE;




/* FIXME: tm_source_file_buffer_update() is not found in symbols table */
static void
refresh_tag_list (TMWorkObject    *tm_wo,
                  ScintillaObject *sci,
                  GeanyDocument   *doc)
{
  /*
  gint    len;
  guchar *buf;
  
  len = sci_get_length (sci);
  buf = g_malloc (len + 1);
  sci_get_text (sci, len + 1, (gchar *)buf);
  tm_source_file_buffer_update (tm_wo, buf, len, TRUE);
  g_free (buf);
  //*/
  if (OPT_save_to_refresh) {
    document_save_file (doc, FALSE);
  }
}

/* tries to insert a comment in the current document */
static void
insert_comment (void)
{
  GeanyDocument *doc;
  
  doc = document_get_current ();
  if (DOC_VALID (doc)) {
    /* try to ensure tags corresponds to the actual state of the file */
    refresh_tag_list (doc->tm_file, doc->editor->sci, doc);
    ggd_insert_comment (doc, sci_get_current_line (doc->editor->sci),
                        OPT_doctype);
  }
}

/* tries to insert comments for all the document */
static void
insert_all_comments (void)
{
  GeanyDocument *doc;
  
  doc = document_get_current ();
  if (DOC_VALID (doc)) {
    /* try to ensure tags corresponds to the actual state of the file */
    refresh_tag_list (doc->tm_file, doc->editor->sci, doc);
    ggd_insert_all_comments (doc, OPT_doctype);
  }
}

static gboolean
load_configuration (void)
{
  gboolean  success = FALSE;
  gchar    *conffile;
  GError   *err = NULL;
  
  plugin->config = ggd_opt_group_new ("General");
  ggd_opt_group_add_string (plugin->config, &OPT_doctype, "doctype");
  ggd_opt_group_add_boolean (plugin->config, &OPT_save_to_refresh, "save_to_refresh");
  conffile = ggd_get_config_file ("ggd.conf", NULL, GGD_PERM_R, &err);
  if (conffile) {
    success = ggd_opt_group_load_from_file (plugin->config, conffile, &err);
  }
  if (err) {
    g_warning (_("Failed to load configuration: %s"), err->message);
    g_error_free (err);
  }
  g_free (conffile);
  /* init filetype manager */
  ggd_file_type_manager_init ();
  
  return success;
}

static void
unload_configuration (void)
{
  gchar    *conffile;
  GError   *err = NULL;
  
  conffile = ggd_get_config_file ("ggd.conf", NULL, GGD_PERM_RW, &err);
  if (conffile) {
    ggd_opt_group_write_to_file (plugin->config, conffile, &err);
  }
  if (err) {
    g_warning (_("Failed to save configuration: %s"), err->message);
    g_error_free (err);
  }
  g_free (conffile);
  ggd_opt_group_free (plugin->config, TRUE);
  plugin->config = NULL;
  /* uninit filetype manager */
  ggd_file_type_manager_uninit ();
}

/* forces reloading of configuration files */
static gboolean
reload_configuration (void)
{
  unload_configuration ();
  return load_configuration ();
}


/* actual Geany interaction */

static void
edit_menu_acivated_handler (GtkMenuItem *menu_item,
                            gpointer     data)
{
  (void)menu_item;
  (void)data;
  
  insert_comment ();
}

static void
insert_comment_keybinding_handler (guint key_id)
{
  (void)key_id;
  
  insert_comment ();
}

/* FIXME: make menu item appear in the edit menu too */
static void
add_edit_menu_item (PluginData *pdata)
{
  GtkWidget *parent_menu;
  
  parent_menu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (
    ui_lookup_widget (geany->main_widgets->editor_menu, "comments")));
  if (! parent_menu) {
    parent_menu = geany->main_widgets->editor_menu;
    pdata->separator_item = gtk_separator_menu_item_new ();
    gtk_menu_shell_append (GTK_MENU_SHELL (parent_menu), pdata->separator_item);
    gtk_widget_show (pdata->separator_item);
  }
  pdata->edit_menu_item = gtk_menu_item_new_with_label (_("Insert documentation comment"));
  pdata->edit_menu_item_hid = g_signal_connect (pdata->edit_menu_item, "activate",
                                                G_CALLBACK (edit_menu_acivated_handler),
                                                NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (parent_menu), pdata->edit_menu_item);
  gtk_widget_show (pdata->edit_menu_item);
  /* make item document-presence sensitive */
  ui_add_document_sensitive (pdata->edit_menu_item);
  /* and attach a keybinding */
  keybindings_set_item (plugin_key_group, KB_INSERT, insert_comment_keybinding_handler,
                        GDK_d, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
                        "instert_doc", _("Insert documentation comment"),
                        pdata->edit_menu_item);
}

static void
remove_edit_menu_item (PluginData *pdata)
{
  g_signal_handler_disconnect (pdata->edit_menu_item, pdata->edit_menu_item_hid);
  pdata->edit_menu_item_hid = 0l;
  if (pdata->separator_item) {
    gtk_widget_destroy (pdata->separator_item);
  }
  gtk_widget_destroy (pdata->edit_menu_item);
}

static void
open_current_filetype_conf_handler (GtkWidget  *widget,
                                    gpointer    data)
{
  GeanyDocument *doc;
  
  (void)widget;
  (void)data;
  
  doc = document_get_current ();
  if (DOC_VALID (doc)) {
    gchar  *path;
    GError *err = NULL;
    
    path = ggd_file_type_manager_get_conf_path (doc->file_type->id,
                                                GGD_PERM_R | GGD_PERM_W, &err);
    if (! path) {
      msgwin_status_add (_("Failed to find configuration file "
                           "for file type \"%s\": %s"),
                         doc->file_type->name, err->message);
      g_error_free (err);
    } else {
      document_open_file (path, FALSE, NULL, NULL);
      g_free (path);
    }
  }
}

static void
reload_configuration_hanlder (GtkWidget  *widget,
                              gpointer    data)
{
  (void)widget;
  (void)data;
  
  reload_configuration ();
}

static GtkWidget *
create_tools_menu_item (void)
{
  GtkWidget  *menu;
  GtkWidget  *item;
  
  /* build submenu */
  menu = gtk_menu_new ();
  /* build "document current symbol" item */
  item = gtk_menu_item_new_with_mnemonic (_("_Document current symbol"));
  g_signal_connect (item, "activate", G_CALLBACK (insert_comment), NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  /* build "document all" item */
  item = gtk_menu_item_new_with_mnemonic (_("Document _all symbols"));
  g_signal_connect (item, "activate", G_CALLBACK (insert_all_comments), NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  /* separator */
  item = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  /* build "reload" item */
  item = gtk_image_menu_item_new_with_mnemonic (_("_Reload configuration files"));
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item),
                                 gtk_image_new_from_stock (GTK_STOCK_REFRESH,
                                                           GTK_ICON_SIZE_MENU));
  g_signal_connect (item, "activate",
                    G_CALLBACK (reload_configuration_hanlder), NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  /* language filetypes opener */
  item = gtk_menu_item_new_with_mnemonic (_("_Edit current language configuration"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  g_signal_connect (item, "activate",
                    G_CALLBACK (open_current_filetype_conf_handler), NULL);
  /* build tools menu item */
  item = gtk_menu_item_new_with_mnemonic (_("_Documentation generator"));
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
  gtk_widget_show_all (item);
  
  return item;
}

static void
build_menus (PluginData *pdata)
{
  add_edit_menu_item (pdata);
  pdata->tools_menu_item = create_tools_menu_item ();
  gtk_menu_shell_append (GTK_MENU_SHELL (geany->main_widgets->tools_menu),
                         pdata->tools_menu_item);
}

static void
destroy_menus (PluginData *pdata)
{
  gtk_widget_destroy (pdata->tools_menu_item);
  pdata->tools_menu_item = NULL;
  remove_edit_menu_item (pdata);
}

void
plugin_init (GeanyData *data G_GNUC_UNUSED)
{
  main_locale_init (LOCALEDIR, GETTEXT_PACKAGE);
  load_configuration ();
  build_menus (plugin);
}

void
plugin_cleanup (void)
{
  destroy_menus (plugin);
  unload_configuration ();
}



static void
conf_dialog_response_handler (GtkDialog  *dialog,
                              gint        response_id,
                              PluginData *pdata)
{
  switch (response_id) {
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_APPLY:
    case GTK_RESPONSE_OK:
    case GTK_RESPONSE_YES:
      ggd_opt_group_sync_from_proxies (pdata->config);
      break;
    
    default: break;
  }
}

GtkWidget *
plugin_configure (GtkDialog *dialog)
{
  GtkWidget  *box;
  GtkWidget  *hbox;
  GtkWidget  *widget;
  GtkWidget  *label;
  
  g_signal_connect (dialog, "response",
                    G_CALLBACK (conf_dialog_response_handler), plugin);
  
  box = gtk_vbox_new (FALSE, 6);
  /* documentation type */
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);
  label = gtk_label_new_with_mnemonic (_("Documentation _type:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  widget = gtk_entry_new ();
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), widget);
  ggd_opt_group_set_proxy_gtkentry (plugin->config, &OPT_doctype, widget);
  gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);
  /* auto-save */
  widget = gtk_check_button_new_with_mnemonic (_("_Save file before generating comment"));
  gtk_widget_set_tooltip_text (widget,
                               _("Whether to automatically save the file in "
                                 "which insert comment before generating the "
                                 "comment. This is currently needed to have an "
                                 "up-to-date tag list. If you disable this "
                                 "option and ask for comment generation on a "
                                 "modified file, the behavior may be "
                                 "surprising (since the comment will be "
                                 "generated for the last saved state)."));
  ggd_opt_group_set_proxy_gtktogglebutton (plugin->config, &OPT_save_to_refresh,
                                           widget);
  gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);
  
  gtk_widget_show_all (box);
  
  return box;
}
