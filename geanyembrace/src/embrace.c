/*
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

#include "geany.h"
#include "support.h"
#include "document.h"
#include "editor.h"
#include "ui_utils.h"
#include "utils.h" 
#include "keybindings.h"
#include "plugindata.h"
#include "geanyfunctions.h"
#include "pluginmacros.h"
#include <string.h>
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>

#define PLUGIN_NAME "geanyembrace"
#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN PLUGIN_NAME

PLUGIN_VERSION_CHECK(140)

/* These items are set by Geany before plugin_init() is called. */
GeanyPlugin		*geany_plugin;
GeanyData		*geany_data;
GeanyFunctions	*geany_functions;

/* All plugins must set name, description, version and author. */
PLUGIN_SET_INFO(_("GeanyEmbrace"), _("Inserts configurable text around selection."), VERSION,
	_("Andras Prim"));


static GeanyKeyBinding *plugin_keys = NULL;
    /* We have to declare plugin_key_group as a single element array.
     * Declaring as a pointer to a struct doesn't work with g_module_symbol(). */
GeanyKeyGroup plugin_key_group[1] =
{
	{PLUGIN_NAME, NULL, 0, NULL}
};


void on_document_switch(GObject *obj, GeanyDocument *doc, gpointer user_data);

PluginCallback plugin_callbacks[] =
{
	{ "document-open", (GCallback) &on_document_switch, TRUE, NULL },
//	{ "document-new", (GCallback) &on_document_switch, TRUE, NULL },
	{ "document-activate", (GCallback) &on_document_switch, TRUE, NULL },
	{ NULL, NULL, FALSE, NULL }
};


typedef struct _EmbraceSettings {
	gboolean create_toolbar;
	gboolean show_dialog;
}EmbraceSettings;

static EmbraceSettings embrace_settings = {
	TRUE, /* create_toolbar */
	TRUE  /*show_dialog */
};



typedef enum {
	EMBRACE_VARIABLE_TYPE_NONE,
	EMBRACE_VARIABLE_TYPE_COUNT,
	EMBRACE_VARIABLE_TYPE_ENTRY,
	EMBRACE_VARIABLE_TYPE_FILE, 
	EMBRACE_VARIABLE_TYPE_C_STATIC,
	EMBRACE_VARIABLE_TYPE_C_ENTRY,
	EMBRACE_VARIABLE_TYPE_NUM
} EmbraceVariableType;

typedef struct _EmbraceVariableTypeDef {
	gchar *label;
	EmbraceVariableType type;
}EmbraceVariableTypeDef;

/// config file name => type enum translator array
EmbraceVariableTypeDef embrace_var_type_def[] = {
	{"count", EMBRACE_VARIABLE_TYPE_COUNT},
	{"entry", EMBRACE_VARIABLE_TYPE_ENTRY},
	{"file", EMBRACE_VARIABLE_TYPE_FILE}, 
	{"c_static", EMBRACE_VARIABLE_TYPE_C_STATIC},
	{"c_entry", EMBRACE_VARIABLE_TYPE_C_ENTRY},
	{NULL, EMBRACE_VARIABLE_TYPE_NONE}
};

#define EMBRACE_VARIABLE_FLAG_COUNT_CHECK   1

#define EMBRACE_VARIABLE_FLAG_FILE_RELATIVE 1
#define EMBRACE_VARIABLE_FLAG_FILE_SLASHSEP 2

#define EMBRACE_VARIABLE_FLAG_CSTATIC_LABEL 1

typedef guint8 EmbraceFlag;
typedef struct _EmbraceFlagTranslator{
	const gchar *name;
	EmbraceFlag value;
}EmbraceFlagTranslator;

EmbraceFlagTranslator translator_null[] = {{NULL, 0}};
EmbraceFlagTranslator translator_count[] = {
	{"check", EMBRACE_VARIABLE_FLAG_COUNT_CHECK}, 
	{NULL, 0}};
EmbraceFlagTranslator translator_file[] = {
	{"relative", EMBRACE_VARIABLE_FLAG_FILE_RELATIVE}, 
	{"dir_sep_slash", EMBRACE_VARIABLE_FLAG_FILE_SLASHSEP}, 
	{NULL, 0}};
EmbraceFlagTranslator translator_cstatic[] = {
	{"label", EMBRACE_VARIABLE_FLAG_CSTATIC_LABEL}, 
	{NULL, 0}};

/** Each array in embrace_flag_translator has config file name => flag value
 * translation data, the last element of each array is filled with 0 */
EmbraceFlagTranslator *embrace_flag_translator[EMBRACE_VARIABLE_TYPE_NUM] = {
	// EMBRACE_VARIABLE_TYPE_NONE
	translator_null, 
	// EMBRACE_VARIABLE_TYPE_COUNT
	translator_count, 
	// EMBRACE_VARIABLE_TYPE_ENTRY
	translator_null, 
	// EMBRACE_VARIABLE_TYPE_FILE
	translator_file, 
	// EMBRACE_VARIABLE_TYPE_C_STATIC
	translator_cstatic, 
	// EMBRACE_VARIABLE_TYPE_C_ENTRY
	translator_null
};

typedef struct _EmbraceVariable{
	EmbraceVariableType type;
	gchar *label;
	EmbraceFlag flags;
}EmbraceVariable;

typedef struct _EmbraceVariableCount{
	EmbraceVariable variable;
	gint last_count;
}EmbraceVariableCount;

typedef struct _EmbraceVariableSimple{
	EmbraceVariable variable;
	gchar *last_text;
}EmbraceVariableSimple;

typedef struct _EmbraceVariableCombo{
	EmbraceVariable variable;
	GtkListStore *store;
	GtkTreeIter last_iter;
	gboolean iter_valid;
}EmbraceVariableCombo;

typedef enum {
	EMBRACE_TN_TYPE_ROOTBLOCK,
	EMBRACE_TN_TYPE_BLOCK,
	EMBRACE_TN_TYPE_TEXT,
	EMBRACE_TN_TYPE_VARIABLE,
	EMBRACE_TN_TYPE_NEWLINE,
	EMBRACE_TN_TYPE_WS,
	EMBRACE_TN_TYPE_CURSOR,
	EMBRACE_TN_TYPE_SELECTION
} EmbraceTNType;

typedef struct _EmbraceTemplateNode{
	EmbraceTNType type;
}EmbraceTemplateNode;

#define EMBRACE_GET_NODE_TYPE(n) ((EmbraceTemplateNode*)(n))->type
#define EMBRACE_SET_NODE_TYPE(n, t) ((EmbraceTemplateNode*)(n))->type=(t)

typedef struct _EmbraceTemplateNodeText{
	EmbraceTemplateNode node;
	gchar *text;
}EmbraceTemplateNodeText;

typedef struct _EmbraceTemplateNodeVar{
	EmbraceTemplateNode node;
	EmbraceVariable *variable;
}EmbraceTemplateNodeVar;

typedef struct _EmbraceTemplateNodeBlock{
	EmbraceTemplateNode node;
	GSList *nodes; ///< list of EmbraceTemplateNode
	/** this variable controls the repeat of this node 
	 * (only for EMBRACE_TN_TYPE_BLOCK nodes) */
	EmbraceVariableCount *count_variable;
}EmbraceTemplateNodeBlock;


typedef struct _EmbraceGroup{
	gchar *name;
	/** a NULL terminated array of pointers that point to elements of 
	 * the global filetypes array
	 * if this is NULL, then this group applies to all documents */
	GeanyFiletype **filetypes; 
	/** list of GtkWidget* 
	 * one item can be a button or a dropdown list of sub_buttons 
	 * sub_buttons are not in tool_items 
	 * the userdata argument of a button's "clicked" event is a pointer
	 * to an EmbraceTemplate, that is in the templates GSlist.
	 * This is initially empty, filled only when this group is first used. */
	GSList *tool_items; 
	/** list of GeanyKeyBinding */
	GSList *keybindings;
	/** list of EmbraceTemplate 
	 * This is initially empty, filled only when this group is first used. */
	GSList *templates;
	/** keyed list of EmbraceVariable 
	 * This is initially empty, items are added on demand. */ 
	GData *variables;
	GtkWidget *menu_item;
	GtkWidget *menu;
}EmbraceGroup;


typedef struct _EmbraceTemplate{
	EmbraceGroup *group;
	gchar *name;
	/// label is visible for users
	gchar *label;
	/** TRUE, if there is at least one variable in the template */
	gboolean need_dialog; 
	/** dialog window 
	 * This is initially NULL, dialog is created on demand.
	 * Widgets in the dialog are connected to their EmbraceVariable with 
	 * g_object_set_data(widget, "var", variable); */
	GtkWidget *dialog;
	EmbraceTemplateNode *root_node; 
}EmbraceTemplate;

typedef struct _EmbraceMenu EmbraceMenu;

typedef struct _EmbraceMenuItem{
	gint index;
	gchar *label;
	GtkWidget *icon;
	EmbraceTemplate *template;
	EmbraceMenu *menu;
}EmbraceMenuItem;

struct _EmbraceMenu {
	EmbraceMenuItem *items;
	gint item_num;
	gint current_item;
	GtkWidget *button;
};

/* these are my globals */
GtkWidget *embrace_toolbar = NULL;
/// this is the "embrace" item in the tools menu
GtkWidget *embrace_menu_item = NULL;
/// this is the submenu of embrace_menu_item
GtkWidget *embrace_menu = NULL;
/// used config file
GKeyFile *embrace_config = NULL;
/// each group represents a group in config file
EmbraceGroup *embrace_groups = NULL; 
gint embrace_group_num = 0; ///< length of embrace_groups
/// path to config dir
gchar *embrace_conf_dir = NULL;
/** when a keybinding is fired (see on_kb_activate()), it knows only it's index.
 * This array binds useful data to that index; */
EmbraceTemplate **embrace_kb_data = NULL;

static void on_button_clicked(GtkToolButton *toolbutton, gpointer gdata);
static void on_item_activate(GtkMenuItem *menuitem, EmbraceTemplate *template);
static void on_menu_button_clicked(GtkMenuToolButton *toolbutton, EmbraceMenu *menu);
static void on_sub_item_activate(GtkMenuItem *menuitem, EmbraceMenuItem *menu_item);
static void on_file_open_clicked (GtkButton *button, GtkEntry *entry);
static void on_kb_activate(G_GNUC_UNUSED guint key_id);


void embrace_variable_free (EmbraceVariable* variable) {
	switch (variable->type) {
		case EMBRACE_VARIABLE_TYPE_C_ENTRY:
		case EMBRACE_VARIABLE_TYPE_C_STATIC:
			gtk_list_store_clear( ((EmbraceVariableCombo*)variable)->store);
			break;
		case EMBRACE_VARIABLE_TYPE_ENTRY:
		case EMBRACE_VARIABLE_TYPE_FILE:
			g_free( ((EmbraceVariableSimple*)variable)->last_text);
			break;
	}
	g_free(variable->label);
	g_free(variable);
}

GSList *embrace_nodes_prepend_block(GSList *nodes, GSList *block_nodes, EmbraceVariableCount* count_var)
{
	EmbraceTemplateNodeBlock *node = g_new0(EmbraceTemplateNodeBlock, 1);
	EMBRACE_SET_NODE_TYPE(node, EMBRACE_TN_TYPE_BLOCK);
	node->nodes = block_nodes;
	node->count_variable = count_var;
	return g_slist_prepend(nodes, node);
}

GSList *embrace_nodes_prepend_text(GSList *nodes, const gchar* text)
{
	if ((NULL == text) || (0 == text[0])) {
		return nodes;
	}
	EmbraceTemplateNodeText *node = g_new0(EmbraceTemplateNodeText, 1);
	EMBRACE_SET_NODE_TYPE(node, EMBRACE_TN_TYPE_TEXT);
	node->text = g_strdup(text);
	return g_slist_prepend(nodes, node);
}

GSList *embrace_nodes_prepend_simple(GSList *nodes, EmbraceTNType type)
{
	EmbraceTemplateNode *node = g_new0(EmbraceTemplateNode, 1);
	EMBRACE_SET_NODE_TYPE(node, type);
	return g_slist_prepend(nodes, node);
}


#define embrace_nodes_prepend_newline(n) embrace_nodes_prepend_simple((n), EMBRACE_TN_TYPE_NEWLINE)
#define embrace_nodes_prepend_ws(n) embrace_nodes_prepend_simple((n), EMBRACE_TN_TYPE_WS)
#define embrace_nodes_prepend_cursor(n) embrace_nodes_prepend_simple((n), EMBRACE_TN_TYPE_CURSOR)
#define embrace_nodes_prepend_selection(n) embrace_nodes_prepend_simple((n), EMBRACE_TN_TYPE_SELECTION)

GSList *embrace_nodes_prepend_variable(GSList *nodes, EmbraceVariable *variable)
{
	EmbraceTemplateNodeVar *node = g_new0(EmbraceTemplateNodeVar, 1);
	EMBRACE_SET_NODE_TYPE(node, EMBRACE_TN_TYPE_VARIABLE);
	node->variable = variable;
	return g_slist_prepend(nodes, node);
}

/// It returns the variable "name" in "group"
/// If it does not already exist, creates, and puts in group->variables
EmbraceVariable* embrace_get_variable(EmbraceGroup *group, const gchar *name)
{
	EmbraceVariable *variable = NULL;
	EmbraceVariableType type = EMBRACE_VARIABLE_TYPE_NONE;
	GtkListStore *store;
	GtkTreeIter iter;
	gchar **values, **lvalues, *key, **store_values, *flagstr, *typefield;
	gint ivalue, nvalues, ilvalue, nlvalues, i, typelen, ncols;
	EmbraceFlag flags; 
	
	variable = g_datalist_get_data(&(group->variables), name);
	if (NULL != variable) {
		return variable;
	}
	
	values = g_key_file_get_string_list(embrace_config, 
		group->name,
		name,
		&nvalues,
		NULL);
	if ((NULL == values) || (nvalues < 1)) {
		g_warning ("no variable definition for [%s] %s", group->name, name);
		return NULL;
	}
	typefield = values[0];
	flagstr = strchr(typefield, '|');
	typelen = (NULL == flagstr) ? strlen(typefield) : flagstr - typefield;
	
	for ( i = 0; embrace_var_type_def[i].label != NULL; i++) {
		if (0 == strncmp(typefield, embrace_var_type_def[i].label, typelen)) {
			type = embrace_var_type_def[i].type;
		}
	}
	if (EMBRACE_VARIABLE_TYPE_NONE == type) {
		g_warning("unknown variable type %s at [%s] %s", values[0], group->name, name);
		g_strfreev(values);
		return NULL;
	}
	
	// parse the flags
	if (NULL != flagstr) {
		EmbraceFlagTranslator *translator;
		gchar *flagend;
		gint flaglen;
		translator = embrace_flag_translator[type];
		
		flagstr ++; // skip '|'
		while (0 != *flagstr) {
			gboolean gotcha;
			// search next flag start
			for(;' ' == *flagstr; flagstr++);
			for(flagend = flagstr; (' ' != *flagend) && (0 != *flagend); flagend++);
			flaglen = flagend - flagstr;
			
			if (flaglen) {
				for (i = 0, gotcha = FALSE; !gotcha && (translator[i].name != NULL); i++) {
					if (0 == strncmp(translator[i].name, flagstr, flaglen) ) {
						flags |= translator[i].value;
						gotcha = TRUE;
					}
				}
				if (!gotcha) {
					g_warning("unknown variable flag %.*s at [%s] %s", flaglen, flagstr, group->name, name);
				}
			}
			flagstr = flagend;
		} // while (0 != *flagstr)
	} // if (NULL != flagstr)
	
	// localised strings
	key = g_strconcat(name, "_label", NULL);
	lvalues = g_key_file_get_locale_string_list(embrace_config,
		group->name,
		key,
		NULL,
		&nlvalues,
		NULL);
	g_free(key);
	
	switch (type) {
		case EMBRACE_VARIABLE_TYPE_COUNT:
			variable = (EmbraceVariable *) g_new0(EmbraceVariableCount, 1);
			break;
		case EMBRACE_VARIABLE_TYPE_ENTRY:
		case EMBRACE_VARIABLE_TYPE_FILE:
			variable = (EmbraceVariable *) g_new0(EmbraceVariableSimple, 1);
			break;
		case EMBRACE_VARIABLE_TYPE_C_STATIC:
		case EMBRACE_VARIABLE_TYPE_C_ENTRY:
			variable = (EmbraceVariable *) g_new0(EmbraceVariableCombo, 1);
			ncols = (flags & EMBRACE_VARIABLE_FLAG_CSTATIC_LABEL) ? 2 : 1;
			store = gtk_list_store_new(ncols, G_TYPE_STRING, G_TYPE_STRING);
			if ((NULL != lvalues) && (nlvalues >= 2) ) {
				store_values = &(lvalues[1]);
			} else if (nvalues >= 3 ){
				store_values = &(values[2]);
			} else {
				store_values = NULL;
			}
			// got to have an empty row for "select nothing"
			gtk_list_store_append(store, &iter);
			gtk_list_store_set(store, &iter, 0, "", -1);
			if (ncols > 1) {
				gtk_list_store_set(store, &iter, 1, "", -1);
			}
			
			if (store_values) {
				while (store_values[0] != NULL) {
					gtk_list_store_append(store, &iter);
					
					gtk_list_store_set(store, &iter,  0, store_values[0], -1);
					if ((ncols > 1) && (NULL != store_values[1])) {
						gtk_list_store_set(store, &iter,  1, store_values[1], -1);
					}
					store_values += ncols;
				}
			}
			((EmbraceVariableCombo*)variable)->store = store;
			if (EMBRACE_VARIABLE_TYPE_C_ENTRY == type) {
				gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store),
					0, GTK_SORT_ASCENDING);
			}
			break;
	} // switch (type) 
	variable->flags = flags;
	
	
	if ((NULL != lvalues) && (nlvalues > 0)) {
		variable->label = g_strdup(lvalues[0]);
	} else if (nvalues > 1){
		variable->label = g_strdup(values[1]);
	}
	variable->type = type;
	
	// group->variables = g_slist_prepend(group->variables, variable);
	g_datalist_set_data_full(&(group->variables), name, variable, (GDestroyNotify) embrace_variable_free);
	g_strfreev(values);
	g_strfreev(lvalues);
	return variable;
}

GtkWidget* embrace_create_button (
	EmbraceGroup *group, 
	const gchar *label, 
	const gchar *icon_name, 
	EmbraceTemplate* template)
{
	gchar *icon_path = g_build_filename(embrace_conf_dir, icon_name, NULL);
	GtkTooltips *tooltips = GTK_TOOLTIPS(p_ui->lookup_widget(geany->main_widgets->window, "tooltips"));
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(icon_path, NULL);
	GtkWidget *icon;
	GtkWidget *button = NULL;
	
	g_free(icon_path);
	
	if (NULL == pixbuf) {
		button = GTK_WIDGET(gtk_tool_button_new(NULL, label));
	} else {
		icon = gtk_image_new_from_pixbuf(pixbuf);
		gtk_widget_show(icon);
		button = GTK_WIDGET(gtk_tool_button_new(icon, label));
	}
	if (button) {
		gtk_tooltips_set_tip(tooltips, button, label, NULL);
		g_signal_connect(button, "clicked",
			G_CALLBACK(on_button_clicked), template);
		gtk_box_pack_start(GTK_BOX(embrace_toolbar), button, FALSE, FALSE, 0);
		group->tool_items = g_slist_prepend(group->tool_items, button);
	}

	return button;
}

void embrace_create_keybinding(
	EmbraceGroup *group, 
	const gchar* name, 
	const gchar* label, 
	const gchar* accel_string,
	EmbraceTemplate* template)
{
	GeanyKeyBinding *kb = g_new0(GeanyKeyBinding, 1);
	
	kb->name = g_strdup_printf("%s_%s", group->name, name);
	kb->label = g_strdup(label);
	if (NULL != accel_string)
		gtk_accelerator_parse(accel_string, &(kb->key), &(kb->mods));
	kb->callback = on_kb_activate;

	kb->menu_item = gtk_menu_item_new_with_label(label);
	gtk_widget_show(kb->menu_item);
	gtk_container_add(GTK_CONTAINER(group->menu), kb->menu_item);
	g_signal_connect(kb->menu_item, "activate",
		G_CALLBACK(on_item_activate), template);

	group->keybindings = g_slist_prepend(group->keybindings, kb);
}

void embrace_init_simple_group(EmbraceGroup *group)
{
	gchar **keys, **values;
	gint nkeys, ikey, nvalues, ivalue;
	EmbraceTemplate *template;
	EmbraceTemplateNode *node;
	EmbraceTemplateNodeBlock *root_node;
	GSList *nodes;
	
	keys = g_key_file_get_keys (embrace_config, group->name, &nkeys, NULL);
	for (ikey = 0; ikey < nkeys; ikey++) {
		if ( strcmp(keys[ikey], "type") && strcmp(keys[ikey], "filetype") ) {
			// a line in the config:
			// <name>=<label>;<text before cursor>;<text after cursor>;<icon>
			// keys[ikey] = <name>
			// values[] = {<label>, ... }
			values = g_key_file_get_locale_string_list (embrace_config, 
				group->name, 
				keys[ikey], 
				NULL, // use current locale
				&nvalues, 
				NULL);
			if(nvalues < 2) {
				g_warning("config file - too few parameters in [%s] %s", group->name, keys[ikey]);
			} else {
				GString *gstr = g_string_sized_new(strlen(values[1]));
				gchar *pch;
				
				template = g_new0(EmbraceTemplate, 1);
				template->name = g_strdup(keys[ikey]);
				template->label = g_strdup(values[0]);
				template->group = group;
				template->need_dialog = FALSE;
				root_node = g_new0(EmbraceTemplateNodeBlock, 1);
				
				EMBRACE_SET_NODE_TYPE(root_node, EMBRACE_TN_TYPE_ROOTBLOCK);
				nodes = NULL;
				pch = values[1];
				while (0 != (*pch)) {
					if ('\t' == (*pch)) {
						nodes = embrace_nodes_prepend_text(nodes, gstr->str);
						nodes = embrace_nodes_prepend_ws(nodes);
						g_string_assign(gstr, "");
					} else if ('\n' == (*pch)) {
						nodes = embrace_nodes_prepend_text(nodes, gstr->str);
						nodes = embrace_nodes_prepend_newline(nodes);
						g_string_assign(gstr, "");
					} else {
						g_string_append_c(gstr, *pch);
					}
					pch ++;
				}
				nodes = embrace_nodes_prepend_text(nodes, gstr->str);
				g_string_assign(gstr, "");
			
				nodes = embrace_nodes_prepend_selection(nodes);
				if (nvalues > 2) {
					pch = values[2];
					while (0 != (*pch)) {
						if ('\t' == (*pch)) {
							nodes = embrace_nodes_prepend_text(nodes, gstr->str);
							nodes = embrace_nodes_prepend_ws(nodes);
							g_string_assign(gstr, "");
						} else if ('\n' == (*pch)) {
							nodes = embrace_nodes_prepend_text(nodes, gstr->str);
							nodes = embrace_nodes_prepend_newline(nodes);
							g_string_assign(gstr, "");
						} else {
							g_string_append_c(gstr, *pch);
						}
						pch ++;
					}
					nodes = embrace_nodes_prepend_text(nodes, gstr->str);
				}
				g_string_free(gstr, TRUE);
				
				root_node->nodes = g_slist_reverse(nodes);
				template->root_node = (EmbraceTemplateNode*) root_node;
				group->templates = g_slist_prepend(group->templates, template);
				
				// create the button
				if (nvalues > 3) {
					embrace_create_button(group, values[0], values[3], template);
				} else {
					embrace_create_button(group, values[0], NULL, template);
				}
				if (nvalues > 4) {
					embrace_create_keybinding(group, keys[ikey], values[0], values[4], template);
				} else {
					embrace_create_keybinding(group, keys[ikey], values[0], NULL, template);
				}
			} // end 'got all four parameters of this key'
			g_strfreev(values);
		} // end key != "type" && key != "filetype"
	} // end for ikey = 0 .. nkeys-1
	g_strfreev(keys);
}


GSList *embrace_template_parse(EmbraceGroup *group, 
	EmbraceVariableCount **count_var, 
	const gchar *str, 
	gint *index)
{
	GSList *nodes = NULL, *nodes2 = NULL, *block_nodes;
	gint i = *index;
	gchar ch, ch2;
	GString *gstr = g_string_sized_new(100);
	EmbraceVariableCount *my_count_var;
	EmbraceVariable *variable;
	EmbraceTemplateNodeVar *node;
	
	*count_var = NULL;
	
	enum {
		TEXT, VAR
	} state = TEXT;
	
	do {
		ch = str[i++];
		if ('%' == ch) {
			if (VAR == state) { // second '%' reached
				if (0 == strcmp("newline", gstr->str)) {
					nodes = embrace_nodes_prepend_newline(nodes);
				} else if (0 == strcmp("ws", gstr->str)) {
					nodes = embrace_nodes_prepend_ws(nodes);
				} else if (0 == strcmp("cursor", gstr->str)) {
					nodes = embrace_nodes_prepend_cursor(nodes);
				} else if (0 == strcmp("selection", gstr->str)) {
					nodes = embrace_nodes_prepend_selection(nodes);
				} else  { 
					variable = embrace_get_variable(group, gstr->str);
					if (NULL == variable) {
						// pass: the warning is printed already
					} else if (variable->type == EMBRACE_VARIABLE_TYPE_COUNT) {
						// if this block already has a count variable with a different name
						// than that stays in group->variables, but will not be used in this block
						*count_var = (EmbraceVariableCount*) variable;
					} else {
						nodes = embrace_nodes_prepend_variable(nodes, variable);
					}
				}
				g_string_assign(gstr, "");
				state = TEXT;
			} else {
				// i already points to the next unparsed gchar
				ch2 = str[i];
				if ( (ch2 == '%') || (ch2 == '[') || (ch2 == ']') ) {
					g_string_append_c(gstr, ch2);
					i++;
				} else { // begginning of a variable name
					nodes = embrace_nodes_prepend_text(nodes, gstr->str);
					g_string_assign(gstr, "");
					state = VAR;
				}
			}
		} else if ('[' == ch) {
			nodes = embrace_nodes_prepend_text(nodes, gstr->str);
			g_string_assign(gstr, "");
			block_nodes = embrace_template_parse(group, &my_count_var, str, &i);
			nodes = embrace_nodes_prepend_block(nodes, block_nodes, my_count_var);
			
		} else if ('\n' == ch) {
			nodes = embrace_nodes_prepend_text(nodes, gstr->str);
			nodes = embrace_nodes_prepend_newline(nodes);
			g_string_assign(gstr, "");
		} else if ('\t' == ch) {
			nodes = embrace_nodes_prepend_text(nodes, gstr->str);
			nodes = embrace_nodes_prepend_ws(nodes);
			g_string_assign(gstr, "");
		} else if ( (ch != 0) && (ch != ']') ){
			g_string_append_c(gstr, ch);
		}
	} while ((ch != 0) && (ch != ']'));

	nodes = embrace_nodes_prepend_text(nodes, gstr->str);
	
	g_string_free(gstr, TRUE);
	// index should point to the next unparsed byte
	// if EOS zero reached, index should point at that
	*index = (ch == 0) ? i - 1 : i;
	nodes = g_slist_reverse(nodes);
	return nodes;
}

gboolean embrace_has_variable_rec(GSList *nodes) 
{
	EmbraceTemplateNode* node;
	gboolean ret;
	
	while (NULL != nodes) {
		node = (EmbraceTemplateNode*) nodes->data;
		if (EMBRACE_TN_TYPE_VARIABLE == node->type) {
			return TRUE;
		} else if (EMBRACE_TN_TYPE_BLOCK == node->type) {
			ret = embrace_has_variable_rec(
				((EmbraceTemplateNodeBlock*)node)->nodes);
			if (ret) 
				return TRUE;
		}
		nodes = nodes->next;
	}
	return FALSE;
}

EmbraceTemplate* embrace_template_new(EmbraceGroup *group, const gchar *name, const gchar *template_string)
{
	EmbraceTemplate *template;
	EmbraceTemplateNode *node;
	EmbraceTemplateNodeBlock *root_node;
	EmbraceVariableCount *count_var;
	GSList *nodes;
	gint index;

	if ((NULL == group) || (NULL == name) || (NULL == template_string)) {
		return NULL;
	}
	template = g_new0(EmbraceTemplate, 1);
	template->name = g_strdup(name);
	template->group = group;
	root_node = g_new0(EmbraceTemplateNodeBlock, 1);
	
	EMBRACE_SET_NODE_TYPE(root_node, EMBRACE_TN_TYPE_ROOTBLOCK);
	index = 0;
	nodes = embrace_template_parse(group, &count_var, template_string, &index);
	
	template->need_dialog = embrace_has_variable_rec(nodes);
	root_node->nodes = nodes;
	root_node->count_variable = count_var;
	template->root_node = (EmbraceTemplateNode*) root_node;

	return template;
}

void embrace_init_menu_button(EmbraceGroup *group, gchar **menudef, gint nmenudef)
{
	gchar **itemdef = NULL;
	gchar *label, *name, *key, *template_string;
	gint imenu, idef, ndef, iitem, i;
	gint nitems = nmenudef-1; // first element is "menu", that is not an item
	EmbraceTemplate *template;
	GtkWidget *menubutton, *menushell, *menu_item;
	gchar *icon_path = NULL;
	//GtkTooltips *tooltips = GTK_TOOLTIPS(p_ui->lookup_widget(geany->main_widgets->window, "tooltips"));
	GdkPixbuf *pixbuf = NULL;
	GtkWidget *icon = NULL;
	EmbraceMenu *menu = g_new0(EmbraceMenu, 1);
	menu->items = g_new0(EmbraceMenuItem, nitems);
	menu->current_item = -1;
	iitem = 0;
	
	menushell = gtk_menu_new();
	
	// menudef[0] must be "menu", so skip it
	for (imenu = 1; imenu < nmenudef; imenu ++) {
		name = menudef[imenu];
		itemdef = g_key_file_get_string_list(embrace_config, group->name, name, &ndef, NULL);
		
		if ( (ndef >= 2) && (0 == strcmp(itemdef[0], "button"))) {
			key = g_strconcat(name, "_template", NULL);
			template_string = g_key_file_get_locale_string(embrace_config, group->name, key, NULL, NULL);
			g_free(key);
			template = embrace_template_new(group, name, template_string);

			if (NULL != template) {
				key = g_strconcat(name, "_label", NULL);
				label = g_key_file_get_locale_string(embrace_config, group->name, key, NULL, NULL);
				g_free(key);
				label = (NULL == label) ? itemdef[1] : label;
				template->label = g_strdup(label);
				group->templates = g_slist_prepend(group->templates, template);
				
				if (ndef >= 3) {
					icon_path = g_build_filename(embrace_conf_dir, itemdef[2], NULL);
					pixbuf = gdk_pixbuf_new_from_file(icon_path, NULL);
				} else {
					pixbuf = NULL;
				}
				if (NULL == pixbuf) {
					icon = NULL;
					menu->items[iitem].icon = NULL;
				} else {
					icon = gtk_image_new_from_pixbuf(pixbuf);
					menu->items[iitem].icon = gtk_image_new_from_pixbuf(pixbuf);
					gtk_widget_show(icon);
					gtk_widget_show(menu->items[iitem].icon);
					// I want to keep it 
					g_object_ref(G_OBJECT(menu->items[iitem].icon));
				}
				menu->items[iitem].index = iitem;
				menu->items[iitem].label = g_strdup(label);
				menu->items[iitem].template = template;
				menu->items[iitem].menu = menu;
				g_free(icon_path);
				
				
				menu_item = gtk_image_menu_item_new_with_label(label);
				if (icon) {
					gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item), icon);
				}
				g_object_set_data_full(G_OBJECT(menu_item), "label", g_strdup(label), g_free);
				g_signal_connect(menu_item, "activate",
					G_CALLBACK(on_sub_item_activate), &(menu->items[iitem]));
				gtk_widget_show(menu_item);
				gtk_menu_shell_append(GTK_MENU_SHELL(menushell), menu_item);
				
				if (ndef >= 4) {
					embrace_create_keybinding(group, name, label, itemdef[3], template);
				} else {
					embrace_create_keybinding(group, name, label, NULL, template);
				}
				iitem ++;
			} else {
				g_warning("config file: missing or unparsable template [%s] %s_template", group->name, name);
			}
		} else {
			g_warning("config file: unparsable item: [%s] %s", group->name, name);
		}
	}

	if (iitem > 0) {
		GtkTooltips *tooltips = GTK_TOOLTIPS(p_ui->lookup_widget(geany->main_widgets->window, "tooltips"));
		menu->item_num = iitem;
		menu->current_item = 0;
		menubutton = GTK_WIDGET(gtk_menu_tool_button_new(menu->items[0].icon, menu->items[0].label));
		gtk_tooltips_set_tip(tooltips, menubutton, menu->items[0].label, NULL);
		menu->button = menubutton;
		
		
		gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(menubutton), menushell);
		g_object_set_data(G_OBJECT(menubutton), "menu", menu);
		g_signal_connect(menubutton, "clicked",
			G_CALLBACK(on_menu_button_clicked), menu);
		gtk_widget_show(menushell);
		group->tool_items = g_slist_prepend(group->tool_items, menubutton);
		gtk_box_pack_start(GTK_BOX(embrace_toolbar), menubutton, FALSE, FALSE, 0);
	} else {
		g_free(menu->items);
		g_free(menu);
	}
}

void embrace_init_advanced_group(EmbraceGroup *group)
{
	gchar **items = NULL, **itemdef = NULL;
	gchar *label, *name, *key, *template_string;
	gint nitems, iitem, ndef;
	EmbraceTemplate *template;
	
	items = g_key_file_get_string_list(embrace_config, group->name, "items", &nitems, NULL);
	for (iitem = 0; iitem < nitems; iitem++) {
		name = items[iitem];
		itemdef = g_key_file_get_string_list(embrace_config, group->name, name, &ndef, NULL);

		// don't care for itemdef, where itemdef[0] == "menu" 
		// but no items are given => (ndef > 1)
		if ((ndef > 1) && (0 == strcmp(itemdef[0], "menu"))) {
			embrace_init_menu_button(group, itemdef, ndef);
		} else if ( (ndef >= 2) && (0 == strcmp(itemdef[0], "button"))) {
			key = g_strconcat(name, "_template", NULL);
			template_string = g_key_file_get_locale_string(embrace_config, group->name, key, NULL, NULL);
			g_free(key);
			template = embrace_template_new(group, name, template_string);

			if (NULL != template) {
				key = g_strconcat(name, "_label", NULL);
				label = g_key_file_get_locale_string(embrace_config, group->name, key, NULL, NULL);
				g_free(key);
				label = (NULL == label) ? itemdef[1] : label;
				template->label = g_strdup(label);
				group->templates = g_slist_prepend(group->templates, template);
				if (ndef >= 3) {
					embrace_create_button(group, label, itemdef[2], template);
				} else {
					embrace_create_button(group, label, NULL, template);
				}
				if (ndef >= 4) {
					embrace_create_keybinding(group, name, label, itemdef[3], template);
				} else {
					embrace_create_keybinding(group, name, label, NULL, template);
				}
			} else {
				g_warning("missing or unparsable template [%s] %s_template", group->name, name);
			}
		} else {
			g_warning("unknown item type or too few arguments: [%s] %s", group->name, name);
		}
		g_strfreev(itemdef);
	}
	g_strfreev(items);
}

void embrace_init_group(EmbraceGroup *group)
{
	// determine type : simple | advanced
	gchar *value = g_key_file_get_string(embrace_config, group->name, "type", NULL);
	group->menu_item = gtk_menu_item_new_with_label(group->name);
	gtk_widget_show(group->menu_item);
	gtk_container_add(GTK_CONTAINER(embrace_menu), group->menu_item);
	group->menu = gtk_menu_new();
	gtk_widget_show(group->menu);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(group->menu_item), group->menu);
	
	g_datalist_init(&(group->variables));
	if (NULL == value) {
		g_message("missing type in [%s], assuming simple", group->name);
		embrace_init_simple_group(group);
	} else {
		if (0 == strcmp(value, "advanced")) {
			embrace_init_advanced_group(group);
		} else if (0 == strcmp(value, "simple")) {
			embrace_init_simple_group(group);
		} else {
			g_message("unknown type=%s in [%s], assuming simple", value, group->name);
			embrace_init_simple_group(group);
		}
		g_free(value);
	}
}

void on_document_switch(GObject *obj, GeanyDocument *doc, gpointer user_data)
{
	// GtkWidget *tooltips = GTK_TOOLTIPS(p_ui->lookup_widget(geany->main_widgets->window, "tooltips"));
	GString *gstr = g_string_sized_new(100);
	GeanyDocument *current_document = p_document->get_current();
	GeanyFiletype *current_filetype;
	GeanyFiletype **group_filetypes;
	// GList* old_toolbar_items = gtk_container_get_children(GTK_CONTAINER(embrace_toolbar));
	// GSList* new_toolbar_items = NULL;
	gint igroup, ikb, nkbs = 0;
	GSList *gslist, *gslist2;
	GList *glist;
	gboolean need_group;
	// @kb@ gboolean *need_groups = g_new(gboolean, embrace_group_num);
	GeanyKeyBinding *kb;
	EmbraceGroup *group;
	
	if (NULL == current_document) return;
	current_filetype = current_document->file_type;
	
	for (igroup = 0; igroup < embrace_group_num; igroup++) {
		group = &(embrace_groups[igroup]);
		group_filetypes = group->filetypes;
		// groups without filetype key are always accessible
		need_group = group_filetypes == NULL ? TRUE : FALSE;
		
		if (group_filetypes) {
			while ( (!need_group) && ((*group_filetypes) != NULL) ) {
				if (current_filetype == (*group_filetypes)) {
					need_group = TRUE;
				}
				group_filetypes++;
			}
		}
		// @kb@ need_groups[igroup] = need_group;
		if (need_group) {
			/* @@ needed when on demand group init comes back
			 * if (NULL == group->templates) {
				embrace_init_group(group);
			} */
			gslist = group->tool_items;
			while (gslist != NULL) {
				// new_toolbar_items = g_slist_prepend(new_toolbar_items, gslist->data);
				gtk_widget_show(GTK_WIDGET(gslist->data));
				gslist = gslist->next;
			}
			// @kb@ nkbs += g_slist_length(group->keybindings);

		} else {
			gslist = group->tool_items;
			while (gslist != NULL) {
				gtk_widget_hide(GTK_WIDGET(gslist->data));
				gslist = gslist->next;
			}
		}
	}

/* @kb@
	g_free(plugin_keys);
	g_free(embrace_kb_data);
	if (nkbs > 0) {
		plugin_keys = g_new(GeanyKeyBinding, nkbs);
		embrace_kb_data = g_new(EmbraceTemplate*, nkbs);
	} else {
		plugin_keys = NULL;
		embrace_kb_data = NULL;
	}
	plugin_key_group->count = nkbs;
	plugin_key_group->keys = plugin_keys;

	
	for (igroup = 0, ikb = 0; igroup < embrace_group_num; igroup++) {
		if (need_groups[igroup]) {
			group = &(embrace_groups[igroup]);
			
			gslist = group->keybindings;
			gslist2 = group->templates;
			while (NULL != gslist) {
				kb = (GeanyKeyBinding*) gslist->data;
				if (gslist2 == NULL) {
					g_warning("internal error: less template than keybinding in [%s]", 
						group->name);
					embrace_kb_data[ikb] = NULL;
				} else {
					embrace_kb_data[ikb] = (EmbraceTemplate*)gslist2->data;
				}
				if (ikb < plugin_key_group->count) {
					p_keybindings->set_item(plugin_key_group, ikb, on_kb_activate,
						kb->key, kb->mods, kb->name, kb->label, kb->menu_item);
				}
				ikb++;
				gslist = gslist->next;
				gslist2 = gslist2->next;
			}

		}
	}
	*/
	g_string_free(gstr, TRUE);
	// @kb@ g_free(need_groups);
}

// puts variables into vars in reverse order
gint embrace_get_template_vars(GSList *nodes, GSList **vars)
{
	EmbraceTemplateNode *node;
	gint varnum = 0;
	
	while (NULL != nodes) {
		node = (EmbraceTemplateNode*)(nodes->data);
		if (EMBRACE_TN_TYPE_VARIABLE == node->type) {
			varnum ++;
			*vars = g_slist_prepend(*vars, 
				((EmbraceTemplateNodeVar*)node)->variable);
		} else if (EMBRACE_TN_TYPE_BLOCK == node->type) {
			if (NULL != ((EmbraceTemplateNodeBlock*)node)->count_variable) {
				varnum ++;
				*vars = g_slist_prepend(*vars,
					((EmbraceTemplateNodeBlock*)node)->count_variable);
			}
			varnum += embrace_get_template_vars(
				((EmbraceTemplateNodeBlock*)node)->nodes, vars);
		}
		nodes = nodes->next;
	}
	return varnum;
}

on_digit_insert_text_before             (GtkEditable     *editable,
                                        gchar           *text,
                                        gint             length,
                                        gpointer         position,
                                        gpointer         user_data)
{
  gint i, nondigit;
  nondigit = 0;

  for (i = 0; i < length; i++) {
	  if (! isdigit((unsigned char) text[i]))
		  nondigit ++;
  }
  
  if (nondigit > 0)
    g_signal_stop_emission_by_name (editable, "insert_text"); 

}

GtkWidget *embrace_dialog_table_new(EmbraceTemplate* template)
{
	gint nvars, irow, icol;
	GtkWidget *table, *widget, *widget2, *widget3, *icon;
	GtkCellRenderer *renderer;
	GSList *vars = NULL, *vars2;
	EmbraceVariable *variable;

	nvars = embrace_get_template_vars(
		((EmbraceTemplateNodeBlock*)template->root_node)->nodes, &vars);
	vars = g_slist_reverse(vars);
	
	table = gtk_table_new(nvars, 2, FALSE);
	gtk_widget_show(table);
	
	vars2 = vars;
	irow = 0;
	while (NULL != vars2) {
		variable = (EmbraceVariable*) (vars2->data);
		widget = gtk_label_new(variable->label);
		gtk_misc_set_alignment(GTK_MISC(widget),1,0.5);
		gtk_widget_show(widget);
		gtk_table_attach(GTK_TABLE(table), widget, 0, 1, irow, irow+1, GTK_FILL, 0, 3, 3);

		switch (variable->type) {
			case EMBRACE_VARIABLE_TYPE_COUNT:
				if (variable->flags & EMBRACE_VARIABLE_FLAG_COUNT_CHECK) {
					widget = gtk_check_button_new();
				} else {
					widget = gtk_entry_new();
					g_signal_connect ((gpointer) widget, "insert_text",
						G_CALLBACK (on_digit_insert_text_before),
						NULL);
				}
				g_object_set_data(G_OBJECT(widget), "var", variable);
				break;
			case EMBRACE_VARIABLE_TYPE_ENTRY:
				widget = gtk_entry_new();
				g_object_set_data(G_OBJECT(widget), "var", variable);
				break;
			case EMBRACE_VARIABLE_TYPE_C_ENTRY:
				widget = gtk_combo_box_entry_new_with_model(
					GTK_TREE_MODEL(((EmbraceVariableCombo*)variable)->store), 0);
				g_object_set_data(G_OBJECT(widget), "var", variable);
				// GtkComboBoxEntry has a built-in text renderer
				break;
			case EMBRACE_VARIABLE_TYPE_C_STATIC:
				widget = gtk_combo_box_new_with_model(
					GTK_TREE_MODEL(((EmbraceVariableCombo*)variable)->store));
				g_object_set_data(G_OBJECT(widget), "var", variable);
				renderer = gtk_cell_renderer_text_new();
				gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(widget), renderer, FALSE);
				icol = (variable->flags & EMBRACE_VARIABLE_FLAG_CSTATIC_LABEL) ? 1 : 0;
				gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(widget), renderer, "text", icol);
				break;
			case EMBRACE_VARIABLE_TYPE_FILE:
				widget = gtk_hbox_new(FALSE, 2);
				widget2 = gtk_entry_new();
				g_object_set_data(G_OBJECT(widget2), "var", variable);
				gtk_widget_show(widget2);
				gtk_container_add(GTK_CONTAINER(widget), widget2);
				widget3 = gtk_button_new();
				icon = gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_BUTTON);
				gtk_widget_show(icon);
				gtk_button_set_image(GTK_BUTTON(widget3), icon);
				g_signal_connect(widget3, "clicked",
					G_CALLBACK(on_file_open_clicked), widget2);
				gtk_widget_show(widget3);
				gtk_container_add(GTK_CONTAINER(widget), widget3);
				break;
		}
		
		gtk_widget_show(widget);
		gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, irow, irow+1);
		
		vars2 = vars2->next;
		irow ++;
	}
	g_slist_free(vars);

	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);
	
	return table;
}

void embrace_dialog_widgets_init(GtkWidget *widget, gpointer dummy)
{
	EmbraceVariable *var;
	GtkListStore *store;
	GtkTreeIter iter;

	if (NULL != (var = g_object_get_data(G_OBJECT(widget), "var"))) {
		switch (var->type) {
			case EMBRACE_VARIABLE_TYPE_COUNT:
				if (var->flags & EMBRACE_VARIABLE_FLAG_COUNT_CHECK) {
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);
				} else {
					gtk_entry_set_text(GTK_ENTRY(widget), "1");
				}
				break;
			case EMBRACE_VARIABLE_TYPE_ENTRY:
			case EMBRACE_VARIABLE_TYPE_FILE:
				gtk_entry_set_text(GTK_ENTRY(widget), "");
				break;
			case EMBRACE_VARIABLE_TYPE_C_ENTRY:
			case EMBRACE_VARIABLE_TYPE_C_STATIC:
				store = ((EmbraceVariableCombo*)var)->store;
				if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter)) {
					gtk_combo_box_set_active_iter(GTK_COMBO_BOX(widget), &iter);
				}
				break;
		}
	} else if (GTK_IS_CONTAINER(widget)) {
		gtk_container_foreach(GTK_CONTAINER(widget), 
			embrace_dialog_widgets_init, 
			NULL);
	}
}

gboolean embrace_store_find_string(GtkListStore *store, GtkTreeIter *iter, const gchar *str)
{
	gchar *ctmp;
	if (! gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), iter) ) {
		return FALSE;
	}
	do {
		gtk_tree_model_get(GTK_TREE_MODEL(store), iter, 0, &ctmp, -1);
		if (0 == strcmp(ctmp, str)) {
			g_free(ctmp);
			return TRUE;
		}
		g_free(ctmp);
	} while (gtk_tree_model_iter_next(GTK_TREE_MODEL(store), iter));
}

void embrace_dialog_widgets_fill_vars(GtkWidget *widget, gpointer dummy)
{
	EmbraceVariable *var;
	const gchar *const_ctmp;
	gchar *ctmp;
	GtkListStore *store;
	GtkTreeIter *iter;

	if (NULL != (var = g_object_get_data(G_OBJECT(widget), "var"))) {
		switch (var->type) {
			case EMBRACE_VARIABLE_TYPE_COUNT:
				if (var->flags & EMBRACE_VARIABLE_FLAG_COUNT_CHECK) {
					((EmbraceVariableCount*)var)->last_count = 
						gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) ? 1 : 0;
				} else {
					const_ctmp = gtk_entry_get_text(GTK_ENTRY(widget));
					((EmbraceVariableCount*)var)->last_count = atoi(const_ctmp);
				}
				break;
			case EMBRACE_VARIABLE_TYPE_ENTRY:
			case EMBRACE_VARIABLE_TYPE_FILE:
				g_free(((EmbraceVariableSimple*)var)->last_text);
				const_ctmp = gtk_entry_get_text(GTK_ENTRY(widget));
				((EmbraceVariableSimple*)var)->last_text = g_strdup(const_ctmp);
				break;
			case EMBRACE_VARIABLE_TYPE_C_ENTRY:
				iter = & (((EmbraceVariableCombo*)var)->last_iter);
				if (! gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), iter)) {
					const_ctmp = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(widget))));
					if ((NULL != const_ctmp) && (0 != const_ctmp[0])) {
						store = ((EmbraceVariableCombo*)var)->store;
						// hmm, maybe this value is already in the store
						if (! embrace_store_find_string(store, iter, const_ctmp)) {
							// this value is not in the store, so create it
							gtk_list_store_append(store, iter);
							gtk_list_store_set(store, iter, 0, const_ctmp, -1);
						}
						((EmbraceVariableCombo*)var)->iter_valid = TRUE;
					} else {
						((EmbraceVariableCombo*)var)->iter_valid = FALSE;
					}
				} else {
					((EmbraceVariableCombo*)var)->iter_valid = TRUE;
				}
				break;
			case EMBRACE_VARIABLE_TYPE_C_STATIC:
				iter = & (((EmbraceVariableCombo*)var)->last_iter);
				if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), iter)) {
					((EmbraceVariableCombo*)var)->iter_valid = TRUE;
				} else {
					((EmbraceVariableCombo*)var)->iter_valid = FALSE;
				}
				break;
		}
	} else if (GTK_IS_CONTAINER(widget)) {
		gtk_container_foreach(GTK_CONTAINER(widget), 
			embrace_dialog_widgets_fill_vars, 
			NULL);
	}
}

gboolean embrace_show_dialog(EmbraceTemplate* template)
{
	gint ret;
	GtkWidget *dialog, *table;
	
	if (NULL == template->dialog) {
		dialog = gtk_dialog_new_with_buttons(template->label,
			GTK_WINDOW(geany->main_widgets->window),
			GTK_DIALOG_MODAL,
			GTK_STOCK_OK,
			GTK_RESPONSE_ACCEPT,
			GTK_STOCK_CANCEL,
			GTK_RESPONSE_REJECT,
			NULL);
		template->dialog = dialog;
		table = embrace_dialog_table_new(template);
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table, FALSE, FALSE, 2);
		// g_object_set_data(G_OBJECT(dialog), "table", table);
	} else {
		dialog = template->dialog;
		// table = g_object_get_data(G_OBJECT(dialog), "table");
	}
	
	gtk_container_foreach(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), 
			embrace_dialog_widgets_init, 
			NULL);
			
	ret = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_hide(dialog);
	if (ret == GTK_RESPONSE_ACCEPT) {
		gtk_container_foreach(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), 
			embrace_dialog_widgets_fill_vars, 
			NULL);
		return TRUE;
	}

	return FALSE;
}

// returns the text representation of the variable in a newly allocated buffer
gchar* embrace_get_variable_text(EmbraceVariable *variable)
{
	gchar *buffer = NULL;
	GtkTreeIter *iter;
	GtkListStore *store;
	
	switch (variable->type) {
		case EMBRACE_VARIABLE_TYPE_COUNT:
			return NULL; 
		case EMBRACE_VARIABLE_TYPE_ENTRY:
		case EMBRACE_VARIABLE_TYPE_FILE:
			return g_strdup( ((EmbraceVariableSimple*)variable)->last_text );
		case EMBRACE_VARIABLE_TYPE_C_ENTRY:
		case EMBRACE_VARIABLE_TYPE_C_STATIC:
			if (((EmbraceVariableCombo*)variable)->iter_valid) {
				iter = & (((EmbraceVariableCombo*)variable)->last_iter);
				store = ((EmbraceVariableCombo*)variable)->store;
				gtk_tree_model_get(GTK_TREE_MODEL(store), iter, 0, &buffer, -1);
			}
			return buffer;
	}
}

typedef struct _EmbraceTemplateSubst {
	gint cursor_pos; ///< in bytes
	gint selection_start; ///< in bytes
	gint selection_end; ///< in bytes
	/** string to be used in new lines (%newline% and \n) 
	 * it contains an EOL character and indentation characters */
	gchar *newline; 
	gchar *ws; ///< to be used at %ws% and \t
}EmbraceTemplateSubst;


void embrace_template_node_subst(EmbraceTemplateNode *node_in, 
	const gchar *selection_contents,
	GString *gstr,
	EmbraceTemplateSubst *subst)
{
	GSList *nodes;
	EmbraceTemplateNode *node;
	gint nvars = 0, nfilledvars = 0, gstr_length_before = gstr->len, len;
	gchar *text;
	gboolean has_visible_sub_block = FALSE;
	
	if ( (EMBRACE_TN_TYPE_BLOCK != node_in->type) && (EMBRACE_TN_TYPE_ROOTBLOCK != node_in->type)) {
		return;
	}
	nodes = ((EmbraceTemplateNodeBlock*)node_in)->nodes;

	while (NULL != nodes) {
		node = nodes->data;
		
		switch(node->type) {
			case EMBRACE_TN_TYPE_TEXT:
				text = ((EmbraceTemplateNodeText*)node)->text;
				g_string_append(gstr, text);
				break;
			case EMBRACE_TN_TYPE_NEWLINE:
				g_string_append(gstr, subst->newline);
				break;
			case EMBRACE_TN_TYPE_WS:
				g_string_append(gstr, subst->ws);
				break;
			case EMBRACE_TN_TYPE_SELECTION:
				nvars++;
				if (0 != selection_contents[0]) {
					nfilledvars++;
				}
				subst->selection_start = gstr->len;
				g_string_append(gstr, selection_contents);
				subst->selection_end = gstr->len;
				break;
			case EMBRACE_TN_TYPE_CURSOR:
				subst->cursor_pos = gstr->len;
				break;
			case EMBRACE_TN_TYPE_BLOCK:
				len = gstr->len;
				embrace_template_node_subst(node, selection_contents, gstr, subst);
				if (len != gstr->len) {
					has_visible_sub_block = TRUE;
				}
				break;
			case EMBRACE_TN_TYPE_VARIABLE:
				nvars++;
				text = embrace_get_variable_text(((EmbraceTemplateNodeVar*)node)->variable);
				if ((NULL != text) && (0 != text[0])) {
					g_string_append(gstr, text);
					nfilledvars ++;
				}
				g_free(text);
				break;
		}
		nodes = nodes->next;
	}
	
	// root block is always shown exactly once
	// other blocks are shown only if: 
	//  1. there is a count variable and it is set greater than 0 (by the user)
	//  2. there is a sub-block and it is visible
	//  3. there are no variables/selection in the block
	//  4. there are variables/selection and at least one of them is set (by the user)
	if (EMBRACE_TN_TYPE_BLOCK == node_in->type) {
		EmbraceTemplateNodeBlock *blocknode = (EmbraceTemplateNodeBlock*) node_in;
		gint repeat;
		if (blocknode->count_variable != NULL) {
			repeat = blocknode->count_variable->last_count;
		} else if (has_visible_sub_block || (nvars == 0) || (nfilledvars > 0)) {
			repeat = 1;
		} else {
			repeat = 0;
		}
		
		if (repeat <= 0) {
			g_string_truncate(gstr, gstr_length_before);
		} else if (repeat > 1) {
			gint i;
			gchar *repeat_text = g_strdup(gstr->str + gstr_length_before);
			for (i = 1; i < repeat; i++) {
				g_string_append(gstr, repeat_text);
			}
			g_free(repeat_text);
		}
	}
}

static void on_template_activate(EmbraceTemplate* template)
{
	gint pos, eolmode;
	GeanyDocument *doc = NULL;
	ScintillaObject *psciobj;
	EmbraceTemplateSubst subst;
	gchar *selection_contents, *line;
	GString *text;
	const GeanyIndentPrefs *indent_pref;
	
	if ((template->need_dialog) && !embrace_show_dialog(template)) {
		return;
	}

	doc = p_document->get_current();

	if (NULL == doc) return;
	psciobj = doc->editor->sci;

	p_sci->start_undo_action(psciobj);
	selection_contents = p_sci->get_selection_contents(psciobj);
	
	text = g_string_sized_new(100);
	subst.cursor_pos = -1;
	subst.selection_start = -1;
	subst.selection_end = -1;
	
	line = p_sci->get_line(psciobj, p_sci->get_current_line(psciobj));
	// @@ what about other whitespace, like UNICODE nbsp?
	for (pos = 0; (' ' == line[pos]) || ('\t' == line[pos]); pos++);
	eolmode = geany_functions->p_scintilla->send_message(psciobj, SCI_GETEOLMODE, 0, 0);
	if (eolmode == SC_EOL_CRLF) {
		subst.newline = g_strdup_printf("\r\n%.*s", pos, line);
	} else if (eolmode == SC_EOL_CR) {
		subst.newline = g_strdup_printf("\r%.*s", pos, line);
	} else {
		subst.newline = g_strdup_printf("\n%.*s", pos, line);
	}
	indent_pref = p_editor->get_indent_prefs(doc->editor);
	if (indent_pref->type != GEANY_INDENT_TYPE_SPACES) {
		subst.ws = g_strdup("\t");
	} else {
		subst.ws = g_strnfill(p_sci->get_tab_width(psciobj), ' ');
	}
	
	embrace_template_node_subst(template->root_node, p_sci->get_selection_contents(psciobj), text, &subst);
	pos = p_sci->get_selection_start(psciobj);
	p_sci->replace_sel(psciobj, text->str);
	g_string_free(text, TRUE);
	g_free(subst.newline);
	g_free(subst.ws);

	if (subst.cursor_pos >= 0)
		p_sci->set_current_position(psciobj, pos + subst.cursor_pos, TRUE);
	if (subst.selection_start >= 0)
		p_sci->set_selection_start(psciobj, pos + subst.selection_start);
	if (subst.selection_end >= 0)
		p_sci->set_selection_end(psciobj, pos + subst.selection_end);
	
	p_sci->end_undo_action(psciobj);
}

/** returns an absolute path in locale (usable by GtkFileChooser) in a new buffer
 * if filename_utf8 is relative, it is considered to be relative to dir_utf8
 * if failed, return NULL
*/
gchar *embrace_get_absolute_dir(const gchar* dir_utf8, const gchar *filename_utf8)
{
	gchar *path_utf8, *path = NULL, *dir2_utf8;
	gsize bread, bwritten;
	GeanyDocument *current_document = p_document->get_current();
	const gchar *docfilename_utf8 = current_document->file_name;
	
	if (g_path_is_absolute(filename_utf8)) {
		path = g_filename_from_utf8(filename_utf8, -1, &bread, &bwritten, NULL);
	} else if (NULL != dir_utf8){
		dir2_utf8 = g_path_get_dirname(filename_utf8);
		path_utf8 = g_build_filename(dir_utf8, dir2_utf8, NULL);
		g_free(dir2_utf8);
		path = g_filename_from_utf8(path_utf8, -1, &bread, &bwritten, NULL);
	}
	return path;
}

void on_file_open_clicked (GtkButton *button, GtkEntry *entry) 
{
	GtkWidget *dialog;
	gchar *path;
	const gchar *const_path;
	gboolean path_set = FALSE;
	EmbraceVariableSimple *var;
	GeanyDocument *current_document = p_document->get_current();
	const gchar *docfilename_utf8 = current_document->file_name;
	gchar *dir_utf8 = NULL;
	gsize bread, bwritten;
	if (NULL != docfilename_utf8) {
		dir_utf8 = g_path_get_dirname(docfilename_utf8);
	}

	dialog = gtk_file_chooser_dialog_new (_("Open File"),
						GTK_WINDOW(geany->main_widgets->window),
						GTK_FILE_CHOOSER_ACTION_OPEN,
						GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
						NULL);
	// 1. try to set path according to entry
	const_path = gtk_entry_get_text(entry);
	if ((NULL != const_path) && (0 != const_path[0])) {
		path = embrace_get_absolute_dir(dir_utf8, const_path);
		if (NULL != path) {
			path_set = gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), path);
			g_free(path);
		}
	}
	// 2. try to set last path used by this file variable
	if (! path_set) {
		var = (EmbraceVariableSimple*) g_object_get_data(G_OBJECT(entry), "var");
		if ((NULL != var) && (NULL != var->last_text) && (0 != var->last_text[0])) {
			path = embrace_get_absolute_dir(dir_utf8, var->last_text);
			if (NULL != path) {
				path_set = gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), path);
				g_free(path);
			}
		}
	}
	// 3. try to set current document's folder
	if (! path_set && (NULL != dir_utf8)) {
		path = g_filename_from_utf8(dir_utf8, -1, &bread, &bwritten, NULL);
		path_set = gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), path);
		g_free(path);
	}
	g_free(dir_utf8);
	

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		char *filename, *filename_utf8;

		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		filename_utf8 = g_filename_to_utf8(filename, -1, &bread, &bwritten, NULL);
		g_free (filename);

		if ((NULL != docfilename_utf8) &&
			( ((EmbraceVariable*)var)->flags & EMBRACE_VARIABLE_FLAG_FILE_RELATIVE) ) { 
			// make it relative
			gint docfile_len = strlen(docfilename_utf8), file_len = strlen(filename_utf8);
			gint minlen = docfile_len < file_len ?  docfile_len : file_len;
			gint i, idirsep; 
			GString *gstr = g_string_sized_new(file_len);
			gchar dirsep;
			if (((EmbraceVariable*)var)->flags & EMBRACE_VARIABLE_FLAG_FILE_SLASHSEP) {
				dirsep = '/';
			} else {
				dirsep = G_DIR_SEPARATOR;
			}
			
			// find common base
			for (i = 0, idirsep = -1; 
				(i < minlen) && (docfilename_utf8[i] == filename_utf8[i]); 
				i++)
			{
				if (G_DIR_SEPARATOR == docfilename_utf8[i]) {
					idirsep = i;
				}
			}
			
			if (idirsep < 0) { // no common base -> absolute path
				gtk_entry_set_text(entry, filename_utf8);
			} else {
				// the ".." entries ...
				for ( /* i = i */; i < docfile_len; i++) {
					if (G_DIR_SEPARATOR == docfilename_utf8[i]) {
						g_string_append(gstr, "..");
						g_string_append_c(gstr, dirsep);
					}
				}
				// ... and the dirs that are not common
				for (i = idirsep + 1; i < file_len; i++) {
					if (G_DIR_SEPARATOR == filename_utf8[i]) {
						g_string_append_c(gstr, dirsep);
					} else {
						g_string_append_c(gstr, filename_utf8[i]);
					}
				}
				
				gtk_entry_set_text(entry, gstr->str);
				g_string_free(gstr, TRUE);
			}
		} else {
			/* set absolute path if either of these are true: 
			 *  a) current document does not have filename yet
			 *  b) "relative" flag is npt set in config file */
			gtk_entry_set_text(entry, filename_utf8);
		}

		g_free (filename_utf8);
	}

	gtk_widget_destroy (dialog);
}

/* Callback when a menu item is clicked */
static void
on_item_activate(GtkMenuItem *menuitem, EmbraceTemplate *template)
{
	on_template_activate(template);
}

/* Callback when a toolbar button is clicked */
static void
on_button_clicked(GtkToolButton *toolbutton, gpointer gdata)
{
	on_template_activate((EmbraceTemplate*) gdata);
}

/* Callback when a toolbar menu button is clicked 
 * (not when the menu is shown) */
static void on_menu_button_clicked(GtkMenuToolButton *toolbutton, EmbraceMenu *menu)
{
	on_template_activate(menu->items[menu->current_item].template);
}

/* Callback when a toolbar menu button's menu item is activated */
static void on_sub_item_activate(GtkMenuItem *menuitem, EmbraceMenuItem *menu_item)
{
	EmbraceMenu *menu = menu_item->menu;
	if (menu_item->index != menu->current_item) {
		GtkTooltips *tooltips = GTK_TOOLTIPS(p_ui->lookup_widget(geany->main_widgets->window, "tooltips"));
		gtk_tooltips_set_tip(tooltips, menu->button, menu_item->label, NULL);
		gtk_tool_button_set_label(GTK_TOOL_BUTTON(menu->button), 
			menu_item->label);
		gtk_tool_button_set_icon_widget (GTK_TOOL_BUTTON(menu->button), 
			menu_item->icon);
		menu->current_item = menu_item->index;
	}
	on_template_activate(menu_item->template);
}

static void on_kb_activate(G_GNUC_UNUSED guint key_id)
{
	if (key_id > plugin_key_group->count) {
		return;
	}
	
	on_template_activate(embrace_kb_data[key_id]);
}

/* Called by Geany to initialize the plugin.
 * Note: data is the same as geany_data. */
void plugin_init(GeanyData *data)
{
	gint i=0, nkbs = 0, ikb;
	gboolean ret;
	gchar ***key_list_list;
	gchar **group_list, **value_list;
	gchar *group_name, *key, *value;
	gsize nkey, igroup, ivalue, nvalue;
	GtkWidget *submenu, *submenu_item;
	GtkWidget *vbox_main, *button, *icon_widget;
	EmbraceGroup *group;
	GeanyKeyBinding *kb;
	GSList *gslist, *gslist2;
	
	embrace_conf_dir = g_strconcat(geany->app->configdir, G_DIR_SEPARATOR_S "plugins"
		G_DIR_SEPARATOR_S PLUGIN_NAME G_DIR_SEPARATOR_S, NULL);
	gchar *conf_filename = g_strconcat(embrace_conf_dir,  PLUGIN_NAME ".conf", NULL);

	embrace_config = g_key_file_new();
	ret = g_key_file_load_from_file (embrace_config, conf_filename, G_KEY_FILE_NONE , NULL);
	
	if (FALSE == ret)  {
		// no user config file: try system-wide
		g_message("user config file %s not found", conf_filename);
		g_free(conf_filename);
		g_free(embrace_conf_dir);
		
		embrace_conf_dir = g_strconcat(geany->app->datadir, "plugins"
			G_DIR_SEPARATOR_S PLUGIN_NAME G_DIR_SEPARATOR_S, NULL);
		conf_filename = g_strconcat(embrace_conf_dir,  PLUGIN_NAME ".conf", NULL);
		ret = g_key_file_load_from_file (embrace_config, conf_filename, G_KEY_FILE_NONE , NULL);
		
		if (FALSE == ret) {
			g_warning("system-wide config file %s not found", conf_filename);
			g_free(conf_filename);
			g_free(embrace_conf_dir);
			embrace_conf_dir = NULL;
			// g_key_file_free(embrace_config);
			return;
		}
	}
	g_free(conf_filename);
	
	/* toolbar preparation */
	if ((embrace_settings.create_toolbar) && (NULL == embrace_toolbar)) {
		embrace_toolbar = gtk_hbox_new(FALSE, 0);
		vbox_main = p_ui->lookup_widget(geany->main_widgets->window, "vbox1");
		if (!GTK_IS_VBOX(vbox_main)) {
			g_warning("cannot add toolbar: \"vbox1\" not found");
		} else {
			gtk_box_pack_start(GTK_BOX(vbox_main), embrace_toolbar, FALSE, FALSE, 0);
			gtk_box_reorder_child(GTK_BOX(vbox_main), embrace_toolbar, 2);
			gtk_widget_show(embrace_toolbar);
		}
	}
	/* menu item */
	embrace_menu_item = gtk_menu_item_new_with_mnemonic(_("_Embrace"));
	gtk_widget_show(embrace_menu_item);
	gtk_container_add(GTK_CONTAINER(geany->main_widgets->tools_menu), embrace_menu_item); 
	embrace_menu = gtk_menu_new();
	gtk_widget_show(embrace_menu);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(embrace_menu_item), embrace_menu);


	// create embrace_groups 
	// fill name, filetypes fields
	group_list = g_key_file_get_groups(embrace_config, &embrace_group_num);
	if (embrace_group_num > 0) {
		embrace_groups = g_new0(EmbraceGroup, embrace_group_num);
		for (igroup = 0; igroup < embrace_group_num; igroup++) {
			group_name = group_list[igroup];
			group = &(embrace_groups[igroup]);
			group->name = g_strdup(group_name);
			value_list = g_key_file_get_string_list(embrace_config, 
				group_name, "filetype", &nvalue, NULL);
			if (NULL != value_list) {
				// allocate +1 for the terminating NULL
				group->filetypes = g_new0(GeanyFiletype*, nvalue + 1);
				for (ivalue = 0; ivalue < nvalue; ivalue++) {
					group->filetypes[ivalue] = 
						p_filetypes->lookup_by_name(value_list[ivalue]);
				}
				g_free(value_list);
			}
			embrace_init_group(group);
			nkbs += g_slist_length(group->keybindings);
		}
	}
	g_strfreev(group_list);
	
	// set keybindings
	if (nkbs > 0) {
		plugin_keys = g_new(GeanyKeyBinding, nkbs);
		embrace_kb_data = g_new(EmbraceTemplate*, nkbs);
	} else {
		plugin_keys = NULL;
		embrace_kb_data = NULL;
	}
	plugin_key_group->count = nkbs;
	plugin_key_group->keys = plugin_keys;
	
	for (igroup = 0, ikb = 0; igroup < embrace_group_num; igroup++) {
		group = &(embrace_groups[igroup]);
			
		gslist = group->keybindings;
		gslist2 = group->templates;
		while (NULL != gslist) {
			kb = (GeanyKeyBinding*) gslist->data;
			if (gslist2 == NULL) {
				g_warning("internal error: less template than keybinding in [%s]", 
					group->name);
				embrace_kb_data[ikb] = NULL;
			} else {
				embrace_kb_data[ikb] = (EmbraceTemplate*)gslist2->data;
			}
			if (ikb < plugin_key_group->count) {
				p_keybindings->set_item(plugin_key_group, ikb, on_kb_activate,
					kb->key, kb->mods, kb->name, kb->label, kb->menu_item);
			}
			ikb++;
			gslist = gslist->next;
			gslist2 = gslist2->next;
		}
	}
		
	// this initializes current document's filetype
	on_document_switch(NULL, NULL, NULL);
}

void embrace_node_free_rec(EmbraceTemplateNode *node)
{
	GSList *gslist;
	switch (node->type) {
		case EMBRACE_TN_TYPE_ROOTBLOCK:
		case EMBRACE_TN_TYPE_BLOCK:
			gslist = ((EmbraceTemplateNodeBlock*)node)->nodes;
			while (NULL != gslist) {
				embrace_node_free_rec((EmbraceTemplateNode*)gslist->data);
				gslist = gslist->next;
			}
			gslist = ((EmbraceTemplateNodeBlock*)node)->nodes;
			g_slist_free(gslist);
			break;
		case EMBRACE_TN_TYPE_TEXT:
			g_free( ((EmbraceTemplateNodeText*)node)->text);
			break;
		case EMBRACE_TN_TYPE_VARIABLE:
			// no need to free variable: it is done other way
		case EMBRACE_TN_TYPE_NEWLINE:
		case EMBRACE_TN_TYPE_WS:
		case EMBRACE_TN_TYPE_CURSOR:
		case EMBRACE_TN_TYPE_SELECTION:
			break;
		default:
			g_warning("embrace_node_free_rec(): unknown node type %d ", node->type);
			break;
	}
	g_free(node);
}

/* Called by Geany before unloading the plugin.
 * Here any UI changes should be removed, memory freed and any other finalization done.
 * Be sure to leave Geany as it was before plugin_init(). */

void plugin_cleanup(void)
{
	gint i, j;
	GtkWidget *widget;
	EmbraceGroup *group;
	GeanyKeyBinding *keybinding;
	EmbraceTemplate *template;
	EmbraceVariable *variable;
	
	GSList *gslist;
	plugin_key_group->count = 0;
	plugin_key_group->keys = 0;

	g_free(plugin_keys);
	plugin_keys = NULL;

	g_key_file_free(embrace_config);
	
	for (i = 0; i < embrace_group_num; i++) {
		group = &embrace_groups[i];

		g_free(group->name);
		g_free(group->filetypes);
		
		gslist = group->tool_items;
		while (NULL != gslist) {
			EmbraceMenu *menu;
			widget = GTK_WIDGET(gslist->data);
			// buttons are destroyed with embrace_toolbar
			// but menutoolbuttons have additional data associated
			if ( GTK_IS_MENU_TOOL_BUTTON(widget) && 
				(menu = g_object_get_data(G_OBJECT(widget), "menu"))) {

				for(j = 0; j < menu->item_num; j++) {
					g_free(menu->items[j].label);
					gtk_widget_destroy(menu->items[j].icon);
				}
				gtk_widget_destroy(widget);
				g_free(menu->items);
				g_free(menu);
			}
			gslist = gslist->next;
		}
		g_slist_free(group->tool_items);


		gslist = group->templates;
		while (NULL != gslist) {
			template = (EmbraceTemplate*) gslist->data;
			g_free(template->name);
			g_free(template->label);
			if (NULL != template->dialog) {
				gtk_widget_destroy(template->dialog);
			}
			embrace_node_free_rec(template->root_node);
			g_free(template);
			gslist = gslist->next;
		}
		g_slist_free(group->templates);
		
		gslist = group->keybindings;
		while (NULL != gslist) {
			keybinding = (GeanyKeyBinding*) gslist->data;
			g_free(keybinding->name);
			g_free(keybinding->label);
			g_free(keybinding);
			gslist = gslist->next;
		}
		g_slist_free(group->keybindings);

		g_datalist_clear(&(group->variables));

	}

	if (NULL != embrace_toolbar) {
		gtk_widget_destroy(embrace_toolbar);
	}
	if (NULL != embrace_menu_item) {
		gtk_widget_destroy(embrace_menu_item);
	}

	g_free(embrace_groups);
	g_free(embrace_conf_dir);
	g_free(embrace_kb_data);
	return;
}
