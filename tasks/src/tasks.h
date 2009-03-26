/*
 *      tasks - tasks.h
 *
 *      Copyright 2009 Bert Vermeulen <bert@biot.com>
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
 */

typedef struct {
	unsigned int line;
	GString *description;
} GeanyTask;


static gboolean on_document_close(GObject *object, GeanyDocument *doc, gpointer data);
static gboolean on_document_open(GObject *object, GeanyDocument *doc, gpointer data);
static gboolean on_document_activate(GObject *object, GeanyDocument *doc, gpointer data);
static gboolean on_editor_notify(GObject *object, GeanyEditor *editor, SCNotification *nt, gpointer data);
static gboolean tasks_button_cb(GtkWidget *widget, GdkEventButton *event, gpointer data);
static gboolean tasks_key_cb(GtkWidget *widget, GdkEventKey *event, gpointer data);
static void free_editor_tasks(void *editor);
static void scan_all_documents(void);
static void scan_document_for_tasks(GeanyDocument *doc);
static void create_tasks_tab(void);
static int scan_line_for_tokens(ScintillaObject *sci, unsigned int line);
static int scan_buf_for_tokens(char *buf);
static GeanyTask *create_task(unsigned int line, char *description);
static int find_line(GeanyTask *task, unsigned int *line);
static void found_token(GeanyEditor *editor, unsigned int line, char *d);
static void no_token(GeanyEditor *editor, unsigned int line);
static void lines_moved(GeanyEditor *editor, unsigned int line, int change);
static int keysort(GeanyTask *a, GeanyTask *b);
static void render_taskstore(GeanyEditor *editor);
