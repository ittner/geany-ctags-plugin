/*
 * main-notebook.c - Part of the Geany Devhelp Plugin
 * 
 * Copyright 2010 Matthew Brush <mbrush@leftclick.ca>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */
 
#include <gtk/gtk.h>
#include "geanyplugin.h"
#include "plugin.h"
#include "main-notebook.h"

static gboolean holds_main_notebook(GtkWidget *widget);
static GtkWidget *create_main_notebook(void);

/**
 * Checks to see if the main_notebook exists in Geany's UI.
 * 
 * @return	TRUE if the main_notebook exists, FALSE if not.
 */
gboolean main_notebook_exists(void)
{
	return holds_main_notebook(geany->main_widgets->window);
}

/**
 * Checks to see if the notebook needs to be destroyed, as in whether it
 * even exists or if it only contains the code tab (ie. last tab).  This
 * function gets called from main_notebook_destroy() to ensure that you
 * can't pull the rug out from under another plugin using the notebook.
 * 
 * @return	TRUE if there are no other plugins using the main notebook
 * 				and it's safe to destroy or FALSE if it should be left
 * 				alone.
 */
gboolean main_notebook_needs_destroying(void)
{
	if (main_notebook_exists())
	{
		GtkWidget *nb = main_notebook_get();
		return (gtk_notebook_get_n_pages(GTK_NOTEBOOK(nb)) > 1) ? FALSE : TRUE;
	}
	return FALSE;
}

/**
 * Puts Geany's UI back to the way it was before the main_notebook was
 * added in.  This function won't actually do anything if another plugin
 * is still using the main_notebook.
 */
void main_notebook_destroy(void)
{	
	GtkWidget *main_notebook, *doc_nb_parent, *vbox, *doc_nb;
	
	if (!main_notebook_needs_destroying())
		return;
		
	main_notebook = ui_lookup_widget(geany->main_widgets->window, 
									"main_notebook");

	doc_nb_parent = gtk_widget_get_parent(main_notebook);
	doc_nb = g_object_get_data(G_OBJECT(main_notebook), "doc_notebook");
	
	/* temporarily disable the notebook to prevent a segfault when
	 * on_editor_focus_in() calls editor_check_colorize(). */
	gtk_widget_set_sensitive(geany->main_widgets->notebook, FALSE);

	vbox = ui_lookup_widget(geany->main_widgets->window, "vbox1");
	if (GTK_IS_WIDGET(doc_nb) && doc_nb->parent != NULL)
	{
		gtk_widget_ref(doc_nb);
		gtk_container_remove(GTK_CONTAINER(main_notebook), doc_nb);
		gtk_widget_destroy(main_notebook);
		gtk_container_add(GTK_CONTAINER(doc_nb_parent), doc_nb);
		gtk_widget_unref(doc_nb);
	}
	
	gtk_widget_set_sensitive(geany->main_widgets->notebook, TRUE);
}

/**
 * Checks to see if the main_notebook already exists and if it does, it
 * returns a pointer to it, if not, it creates the main_notebook.  This 
 * function should be called by plugins wishing to use the main_notebook
 * to ensure that there is no conflict with other plugins.
 * 
 * @return The main_notebook that already existed or was created.  If
 * 			 there was a problem creating/getting the main_notebook,
 * 			 NULL is returned.
 */
GtkWidget *main_notebook_get(void)
{
	if (main_notebook_exists())
		return ui_lookup_widget(geany->main_widgets->window, "main_notebook");
	else
		return create_main_notebook();
}


/* we need this since ui_lookup_widget() doesn't seem to realize when
 * the main_notebook widget is destroyed and so keeps returning something
 * even when main_notebook is gone. */
static gboolean holds_main_notebook(GtkWidget *widget)
{
    gboolean found = FALSE;
    const gchar *widget_name = gtk_widget_get_name(widget);
    
    if (widget_name != NULL && g_strcmp0(widget_name, "main_notebook") == 0)
        found = TRUE;
    if (GTK_IS_CONTAINER(widget))
    {
        GList *children, *iter;
        
        children = gtk_container_get_children(GTK_CONTAINER(widget));
        
        for (iter=children; !found && iter; iter=g_list_next(iter))
            found = holds_main_notebook(iter->data);
            
        g_list_free(children);
    }
    
    return found;
}

/* 
 * Creates the main_notebook if it doesn't already exist and returns a
 * pointer to it.  If it does already exist, the function returns NULL.
 */
static GtkWidget *create_main_notebook(void)
{
	GtkWidget *main_notebook, *doc_nb_box, *doc_nb_parent;
	GtkWidget *vbox, *doc_nb;

	if (main_notebook_exists())
		return NULL;
	
	doc_nb_box = gtk_vbox_new(FALSE, 0);
	vbox = ui_lookup_widget(geany->main_widgets->window, "vbox1");
	
	main_notebook = gtk_notebook_new();
	gtk_widget_set_name(main_notebook, "main_notebook");
	
	doc_nb_parent = gtk_widget_get_parent(geany->main_widgets->sidebar_notebook);
	
	/* sidebar on left */
	if (gtk_paned_get_child1(GTK_PANED(doc_nb_parent)) == geany->main_widgets->sidebar_notebook)
		doc_nb = gtk_paned_get_child2(GTK_PANED(doc_nb_parent));
	else /* sidebar on right */
		doc_nb = gtk_paned_get_child1(GTK_PANED(doc_nb_parent));

	gtk_notebook_append_page(GTK_NOTEBOOK(main_notebook), 
							 doc_nb_box,
							 gtk_label_new(_("Code")));
	
	gtk_widget_ref(doc_nb);
	gtk_container_remove(GTK_CONTAINER(doc_nb_parent), doc_nb);

	gtk_container_add(GTK_CONTAINER(doc_nb_parent), main_notebook);
	gtk_widget_show_all(main_notebook);
	
	gtk_container_add(GTK_CONTAINER(doc_nb_box), doc_nb);
	gtk_widget_unref(doc_nb);
	gtk_widget_show_all(doc_nb);

	gtk_notebook_set_current_page(GTK_NOTEBOOK(main_notebook), 0);
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(main_notebook), GTK_POS_BOTTOM);

	/* from ui_utils: ui_hookup_widget() */
	g_object_set_data_full(
		G_OBJECT(geany->main_widgets->window), 
		"main_notebook",
		g_object_ref(main_notebook),  
		(GDestroyNotify)g_object_unref);

	return main_notebook;
}
