
/*
 * gdb-ui-main.c - A GTK-based user interface for the GNU debugger.
 *
 * See the file "gdb-ui.h" for license information.
 *
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <gtk/gtk.h>

#include "gdb-io.h"
#include "gdb-ui.h"

#include "support.h"



GdbUiSetup gdbui_setup;

static GtkWidget *vbox;
static GtkWidget *action_area;
static GtkWidget *stat_lab;

static GtkWidget *load_btn;
static GtkWidget *run_btn;
static GtkWidget *pause_btn;
static GtkWidget *cont_btn;
static GtkWidget *step_btn;
static GtkWidget *stepi_btn;
static GtkWidget *next_btn;
static GtkWidget *nexti_btn;
static GtkWidget *until_btn;
static GtkWidget *stack_btn;
static GtkWidget *break_btn;
static GtkWidget *watch_btn;
static GtkWidget *finish_btn;
static GtkWidget *return_btn;
static GtkWidget *kill_btn;
static GtkWidget *env_btn;
static GtkWidget *unload_btn;
static GtkWidget *prefs_btn;

static GtkWidget *term_chk;
static GtkWidget *pipe_chk;


static GtkWidget *con_lab;
static GtkWidget *con_cmd;

static GtkWidget *last_used = NULL;

static gboolean pause_clicked = FALSE;


#define we(w) gtk_widget_set_sensitive(w,TRUE);
#define wd(w) gtk_widget_set_sensitive(w,FALSE);

#define pipe_chk_active() gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pipe_chk))

#define black "#000000"
#define white "#FFFFFF"
#define red   "#EE0000"
#define green  "#00DD00"
#define blue   "#0000FF"
#define yellow "#FFFF00"


void
gdbui_enable(gboolean enabled)
{
	gtk_widget_set_sensitive(vbox, enabled);
}


static void
status(const gchar * msg, const gchar * bg, const gchar * fg)
{
	gchar *esc = g_markup_escape_text(msg, -1);
	gchar *mu =
		g_strdup_printf
		("<span weight=\"bold\" foreground=\"%s\"  background=\"%s\">  %s  </span>", fg, bg,
		 esc);
	gtk_label_set_markup(GTK_LABEL(stat_lab), mu);
	g_free(mu);
	g_free(esc);
}

static GdbStatus curr_status = GdbDead;

static void
status_func(GdbStatus st)
{
	wd(load_btn);
	wd(run_btn);
	wd(pause_btn);
	wd(cont_btn);
	wd(step_btn);
	wd(stepi_btn);
	wd(next_btn);
	wd(nexti_btn);
	wd(until_btn);
	wd(finish_btn) wd(return_btn) wd(stack_btn);
	wd(kill_btn);
	wd(pipe_chk);
	wd(break_btn);
	wd(watch_btn);
	wd(con_lab);
	wd(con_cmd);
	wd(term_chk);
	wd(env_btn);
	we(unload_btn);
	switch (st)
	{
		case GdbDead:
			{
				we(load_btn);
				we(pipe_chk);
				we(term_chk);
				wd(unload_btn);
				status(_("(no program)"), black, white);
				break;
			}
		case GdbLoaded:
			{
				we(load_btn);
				we(run_btn);
				we(pipe_chk);
				we(break_btn);
				we(watch_btn);
				we(term_chk);
				we(con_lab);
				we(con_cmd);
				we(env_btn);
				status(_("loaded"), white, black);
				break;
			}
		case GdbStartup:
			{
				status(_("starting"), yellow, red);
				break;
			}
		case GdbRunning:
			{
				we(pause_btn);
				we(kill_btn);
				status(_("running"), green, white);
				break;
			}
		case GdbStopped:
			{
				we(cont_btn);
				we(step_btn);
				we(stepi_btn);
				we(next_btn);
				we(nexti_btn);
				we(until_btn);
				we(finish_btn) we(return_btn) we(stack_btn);
				we(kill_btn);
				we(break_btn);
				we(watch_btn);
				we(con_lab);
				we(con_cmd);
				we(pipe_chk);
				we(env_btn);
				status(_("stopped"), red, yellow);
				break;
			}
		case GdbFinished:
			{
				we(load_btn);
				we(run_btn);
				we(con_lab);
				we(con_cmd);
				we(pipe_chk);
				we(term_chk);
				we(break_btn);
				we(watch_btn);
				we(env_btn);
				status(_("terminated"), white, black);
				break;
			}
	}
	if (!(last_used->state & GTK_STATE_INSENSITIVE))
	{
		gtk_widget_grab_focus(last_used);
	}
	curr_status = st;
}



static void
show_line(const gchar * filename, const gchar * line, const gchar * reason)
{
	if (gdbui_setup.line_func)
	{
		gdbui_setup.line_func(filename, line, reason);
	}
	else
	{
		g_printerr("%s:%s (%s)\n", filename, line, reason);
	}
}



static void
signal_func(const GdbSignalInfo * si)
{
	gchar *msg = g_strdup_printf("%s (%s)\nat %s in function %s()\nat %s:%s%s%s",
				     si->signal_name, si->signal_meaning, si->addr,
				     si->func, si->file, si->line, si->from ? "\nfrom " : "",
				     si->from ? si->from : "");
	if (pause_clicked)
	{
		status(_("paused"), yellow, red);
		pause_clicked = FALSE;
	}
	else
	{
		GtkWidget *dlg = gtk_message_dialog_new(GTK_WINDOW(gdbui_setup.main_window),
							GTK_DIALOG_MODAL |
							GTK_DIALOG_DESTROY_WITH_PARENT,
							GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
							_("Program received signal:"));

		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dlg), "%s", msg);
		gtk_window_set_title(GTK_WINDOW(dlg), "debugger");
		gtk_window_set_keep_above(GTK_WINDOW(dlg), TRUE);
		gtk_dialog_run(GTK_DIALOG(dlg));
		gtk_widget_destroy(dlg);
		status(si->signal_name, yellow, red);
	}
	if (gdbui_setup.warn_func)
	{
		gchar *p;
		for (p = msg; *p; p++)
		{
			if (*p == '\n')
			{
				*p = ' ';
			}
		}
		gdbui_setup.warn_func(msg);
	}
	g_free(msg);
	show_line(si->fullname, si->line, NULL);
}



static void
step_func(const gchar * filename, const gchar * line, const gchar * reason)
{
	status(reason, yellow, red);
	show_line(filename, line, reason);
}



static void
info_func(const gchar * msg)
{
	if (gdbui_setup.info_func)
	{
		gdbui_setup.info_func(msg);
	}
	else
	{
		g_printerr("%s", msg);
	}
}

static void
stack_dlg(const GSList * frame_list)
{
	while (gtk_events_pending())
	{
		gtk_main_iteration();
	}
	gdbui_stack_dlg(frame_list);
	we(stack_btn);
	gtk_widget_grab_focus(stack_btn);
}



static void
err_func(const gchar * msg)
{
	GtkWidget *dlg = NULL;
	dlg = gtk_message_dialog_new(GTK_WINDOW(gdbui_setup.main_window),
				     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				     GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Error:"));
	gtk_window_set_keep_above(GTK_WINDOW(dlg), TRUE);
	gtk_window_set_title(GTK_WINDOW(dlg), "debugger");
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dlg), "%s", msg);
	gtk_window_set_keep_above(GTK_WINDOW(dlg), TRUE);
	gtk_dialog_run(GTK_DIALOG(dlg));
	gtk_widget_destroy(dlg);
	if (gdbui_setup.warn_func)
	{
		gdbui_setup.warn_func(msg);
	}
}



static void
pipe_click(GtkWidget * w, gpointer user_data)
{
	gdbio_send_cmd("-interpreter-exec console \"handle SIGPIPE %s\"\n",
		       pipe_chk_active()? "nostop" : "stop");
}




static GtkTooltips *tooltips = NULL;

void
gdbui_set_tips(GtkTooltips * tips)
{
	if (tooltips)
	{
		gtk_object_destroy(GTK_OBJECT(tooltips));
	}
	tooltips = tips;
}


void
gdbui_set_tip(GtkWidget * w, gchar * tip)
{
	if (gdbui_setup.options.show_tooltips)
	{
		if (w && tip)
		{
			if (!tooltips)
			{
				tooltips = gtk_tooltips_new();
			}
			gtk_tooltips_set_tip(tooltips, w, tip, NULL);

		}
	}
}


static const gboolean disable_mnemonics = TRUE;

static GtkWidget *
make_btn(gchar * text, GtkCallback cb, gchar * img, gchar * tip)
{
	GtkWidget *btn;
	if (text && disable_mnemonics)
	{
		gchar *p;
		gchar buf[32];
		strncpy(buf, text, sizeof(buf));
		for (p = buf; *p; p++)
		{
			if (*p == '_')
			{
				memmove(p, p + 1, strlen(p));
			}
		}
		text = buf;
	}
	btn = text ? gtk_button_new_with_mnemonic(text) : gtk_button_new();
	if (cb)
	{
		g_signal_connect(G_OBJECT(btn), "clicked", G_CALLBACK(cb), NULL);
	}
	if (tip)
	{
		gdbui_set_tip(btn, tip);
	}
	if (img && gdbui_setup.options.show_icons)
	{
		gtk_button_set_image(GTK_BUTTON(btn),
				     gtk_image_new_from_stock(img, GTK_ICON_SIZE_BUTTON));
	}
	return btn;
}


#define used(w) last_used=w;



// Crude check for "ELF 32-bit LSB [reloc|exec|shared|core] Intel 80386, version 1"
typedef enum
{ ELF_RELOC, ELF_EXEC, ELF_SHARED, ELF_CORE, ELF_NOT_ELF } ElfType;

static ElfType
get_elf_type(const char *filename)
{
	char buf[32];
	ElfType rv = ELF_NOT_ELF;
	int n;
	FILE *fh = fopen(filename, "r");
	if (fh)
	{
		memset(buf, 0, sizeof(buf));
		n = fread(buf, 1, sizeof(buf), fh);
		fclose(fh);
		if ((n == sizeof(buf)) &&
		    (memcmp(buf, "\177ELF", 4) == 0) &&
		    (memcmp(buf + 5, "\1\1\0\0\0\0\0\0\0\0\0", 11) == 0)
		    /* && (memcmp(buf+17,"\0\3\0\1",4)==0) *//* <= Intel 80386, version 1 */
			)
		{
			switch (buf[4])
			{
				case '\1':
					break;	/* 32-bit */
				case '\2':
					break;	/* 64-bit */
				default:
					return ELF_NOT_ELF;
			}
			switch (buf[16])
			{
				case '\1':
					return ELF_RELOC;
				case '\2':
					return ELF_EXEC;
				case '\3':
					return ELF_SHARED;
				case '\4':
					return ELF_CORE;
			}
		}
	}
	return rv;
}




static void
load_click(GtkWidget * btn, gpointer user_data)
{
	gchar *errmsg = NULL;
	GtkWidget *dlg = gtk_file_chooser_dialog_new(_("Select executable to debug"),
						     GTK_WINDOW(gdbui_setup.main_window),
						     GTK_FILE_CHOOSER_ACTION_OPEN,
						     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						     GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	do
	{
		if (errmsg)
		{
			err_func(errmsg);
			errmsg = NULL;
		}
		if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT)
		{
			gchar *fn = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
			if (fn)
			{
				if (access(fn, R_OK) == 0)
				{
					switch (get_elf_type(fn))
					{
						case ELF_RELOC:
							{
								errmsg = _("Debugging of object files is not supported.");
								break;
							}
						case ELF_EXEC:
							{
								if (access(fn, X_OK) == 0)
								{
									gchar *ldd =
										g_strconcat("ldd ",
											    fn,
											    NULL);
									FILE *fh = popen(ldd, "r");
									if (fh)
									{
										ssize_t r = 0;
										char *buf = NULL;
										size_t len = 0;
										gboolean have_x =
											FALSE;
										while (r >= 0)
										{
											r = getline
												(&buf,
												 &len,
												 fh);
											if (len
											    && buf
											    &&
											    strstr
											    (buf,
											     "libX11.so"))
											{
												have_x = TRUE;
											}
										}
										fclose(fh);
										gtk_toggle_button_set_active
											(GTK_TOGGLE_BUTTON
											 (term_chk),
											 !have_x);
									}
									gdbio_load(fn);
									if (pipe_chk_active())
									{
										pipe_click(pipe_chk,
											   NULL);
									}
								}
								else
								{
									errmsg = _("You don't have permission to execute this file.");
								}
								break;
							}
						case ELF_SHARED:
							{
								errmsg = _("Debugging of shared libraries is not supported.");
								break;
							}
						case ELF_CORE:
							{
								errmsg = _("Debugging of core files id not supported.");
								break;
							}
						default:
							{
								errmsg = _("Target file must be ELF 32-bit x86 executable.");
							}
					}
				}
				else
				{
					errmsg = _("You don't have permission to read this file.");
				}
				g_free(fn);
			}
		}
		else
		{
			break;
		}
	}
	while (errmsg);
	gtk_widget_destroy(dlg);
	used(btn);
}



static void
pause_click(GtkWidget * btn, gpointer user_data)
{
	pause_clicked = TRUE;
	gdbio_pause_target();
	used(btn);
}



static void
kill_click(GtkWidget * btn, gpointer user_data)
{
	gboolean have_cmd_line = !(con_cmd->state & GTK_STATE_INSENSITIVE);
	gdbio_kill_target(!have_cmd_line);
	used(btn);
}



static void
run_click(GtkWidget * btn, gpointer user_data)
{
	gdbio_exec_target(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(term_chk))
			  && gdbui_setup.options.term_cmd
			  && *gdbui_setup.options.term_cmd ? gdbui_setup.options.term_cmd : NULL);
	used(btn);
}


static void
cont_click(GtkWidget * btn, gpointer user_data)
{
	gdbio_continue();
	used(btn);
}



static void
stack_click(GtkWidget * btn, gpointer user_data)
{
	wd(stack_btn);
	gdbio_show_stack(stack_dlg);
	used(btn);
}



static void
break_click(GtkWidget * btn, gpointer user_data)
{
	gdbui_break_dlg(FALSE);
	used(btn);
}


static void
watch_click(GtkWidget * btn, gpointer user_data)
{
	gdbui_break_dlg(TRUE);
	used(btn);
}



static void
step_click(GtkWidget * btn, gpointer user_data)
{
	gdbio_send_cmd("-exec-step\n");
	used(btn);
}

static void
stepi_click(GtkWidget * btn, gpointer user_data)
{
	gdbio_send_cmd("-exec-step-instruction\n");
	used(btn);
}


static void
next_click(GtkWidget * btn, gpointer user_data)
{
	gdbio_send_cmd("-exec-next\n");
	used(btn);
}

static void
nexti_click(GtkWidget * btn, gpointer user_data)
{
	gdbio_send_cmd("-exec-next-instruction\n");
	used(btn);
}



static void
until_click(GtkWidget * btn, gpointer user_data)
{
	LocationInfo *li = gdbui_location_dlg(_("Run to location"), FALSE);
	if (li)
	{
		if (li->filename && *(li->filename))
		{
			gdbio_send_cmd("-exec-until %s:%s\n", li->filename, li->line_num);
		}
		else
		{
			gdbio_send_cmd("-exec-until %s:%s\n", li->line_num);
		}
		gdbui_free_location_info(li);
	}
	used(btn);
}


static void
finish_click(GtkWidget * btn, gpointer user_data)
{
	gdbio_finish();
	used(btn);
}



static void
return_click(GtkWidget * btn, gpointer user_data)
{
	gdbio_return();
	used(btn);
}



static void
env_click(GtkWidget * btn, gpointer user_data)
{
	gdbio_get_env(gdbui_env_dlg);
	used(btn);
}



static void
unload_click(GtkWidget * btn, gpointer user_data)
{
	gdbio_exit();
	used(btn);
}



static void
prefs_click(GtkWidget * btn, gpointer user_data)
{
	gdbui_opts_dlg();
#ifdef STANDALONE
	gtk_window_set_keep_above(GTK_WINDOW(gdbui_setup.main_window), gdbui_opts.stay_on_top);
#endif
}



static void
entry_activate(GtkWidget * w, gpointer user_data)
{
	const gchar *txt = gtk_entry_get_text(GTK_ENTRY(w));
	if (txt && *txt)
	{
		gdbio_send_cmd("%s\n", txt);
		gtk_entry_set_text(GTK_ENTRY(w), "");
	}
	used(w);
}

#define new_row \
  w=gtk_hbox_new(TRUE,0); \
  gtk_box_pack_start(GTK_BOX(action_area),w, TRUE, TRUE, 0); \
  gtk_widget_show(w);



#define split1 split(action_area)
#define split2 split(vbox)


//#define splitw gtk_widget_new(GTK_TYPE_BIN,NULL)
//#define splitw gtk_hseparator_new()
#define splitw gtk_hbox_new(FALSE,0)


#define split(vb) \
gtk_box_pack_start(GTK_BOX(vb),splitw,FALSE,FALSE,3);


#define BtnGrow TRUE
#define BtnFill TRUE
#define BtnPad 1

GtkWidget *
gdbui_create_widgets(GtkWidget * parent)
{
	GtkWidget *w = NULL;
	vbox = gtk_vbox_new(FALSE, 0);
	if (parent)
	{
		gtk_container_add(GTK_CONTAINER(parent), vbox);
	}

	split2 stat_lab = gtk_label_new(_("no program"));
	gtk_box_pack_start(GTK_BOX(vbox), stat_lab, FALSE, FALSE, 4);
	gtk_widget_show(vbox);

	action_area = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), action_area, FALSE, FALSE, 0);
	gtk_widget_show(action_area);
	load_btn =
		make_btn(_("_Load"), load_click, GTK_STOCK_OPEN, _("Load target program into debugger."));
	unload_btn =
		make_btn(_("_Unload"), unload_click, GTK_STOCK_QUIT,
			 _("Kill the target program AND the debugger."));
	run_btn =
		make_btn(_("_Run"), run_click, GTK_STOCK_EXECUTE,
			 _("Execute target program in debugger."));
	kill_btn =
		make_btn(_("_Kill"), kill_click, GTK_STOCK_STOP,
			 _("Kill the target program with SIGKILL."));
	pause_btn =
		make_btn(_("_Pause"), pause_click, GTK_STOCK_MEDIA_PAUSE,
			 _("Pause the target program with SIGINT."));
	cont_btn =
		make_btn(_("_Continue"), cont_click, GTK_STOCK_MEDIA_PLAY,
			 _("Continue executing target program."));
	step_btn =
		make_btn(_("_Step"), step_click, GTK_STOCK_GO_FORWARD,
			 _("Step to the next line or function call."));
	stepi_btn =
		make_btn(_("Step _in"), stepi_click, GTK_STOCK_GOTO_LAST,
			 _("Execute the next machine instruction or function call."));
	next_btn = make_btn("_Next", next_click, GTK_STOCK_MEDIA_FORWARD, _("Step to the next line."));
	nexti_btn =
		make_btn(_("Ne_xt in"), nexti_click, GTK_STOCK_MEDIA_NEXT,
			 _("Execute the next machine instruction."));
	until_btn =
		make_btn(_("Run _to"), until_click, GTK_STOCK_JUMP_TO,
			 _("Run to specified source line."));
	stack_btn =
		make_btn(_("Stac_k"), stack_click, GTK_STOCK_DND_MULTIPLE,
			 _("Display a backtrace of the current call stack."));
	break_btn = make_btn("_Breaks", break_click, GTK_STOCK_INDEX, _("Add or remove breakpoints."));
	watch_btn = make_btn("_Watches", watch_click, GTK_STOCK_FIND, _("Add or remove watchpoints."));
	finish_btn =
		make_btn(_("_Finish"), finish_click, GTK_STOCK_GOTO_BOTTOM,
			 _("Complete the currently executing function."));
	return_btn =
		make_btn(_("_Return"), return_click, GTK_STOCK_UNDO,
			 _("Return immediately from the current function."));
	env_btn =
		make_btn(_("En_viron"), env_click, GTK_STOCK_PROPERTIES,
			 _("Set target environment and command line options."));
	prefs_btn =
		make_btn(_("_Options"), prefs_click, GTK_STOCK_PREFERENCES,
			 _("Set user interface options."));

	split1;
	new_row;
	gtk_box_pack_start(GTK_BOX(w), load_btn, BtnGrow, BtnFill, BtnPad);
	gtk_box_pack_start(GTK_BOX(w), unload_btn, BtnGrow, BtnFill, BtnPad);


	new_row;
	gtk_box_pack_start(GTK_BOX(w), run_btn, BtnGrow, BtnFill, BtnPad);
	gtk_box_pack_start(GTK_BOX(w), kill_btn, BtnGrow, BtnFill, BtnPad);


	split1;

	new_row;
	gtk_box_pack_start(GTK_BOX(w), pause_btn, BtnGrow, BtnFill, BtnPad);
	gtk_box_pack_start(GTK_BOX(w), cont_btn, BtnGrow, BtnFill, BtnPad);

	new_row;
	gtk_box_pack_start(GTK_BOX(w), next_btn, BtnGrow, BtnFill, BtnPad);
	gtk_box_pack_start(GTK_BOX(w), nexti_btn, BtnGrow, BtnFill, BtnPad);

	new_row;
	gtk_box_pack_start(GTK_BOX(w), step_btn, BtnGrow, BtnFill, BtnPad);
	gtk_box_pack_start(GTK_BOX(w), stepi_btn, BtnGrow, BtnFill, BtnPad);

	new_row;
	gtk_box_pack_start(GTK_BOX(w), finish_btn, BtnGrow, BtnFill, BtnPad);
	gtk_box_pack_start(GTK_BOX(w), return_btn, BtnGrow, BtnFill, BtnPad);

	new_row;
	gtk_box_pack_start(GTK_BOX(w), until_btn, BtnGrow, BtnFill, BtnPad);
	gtk_box_pack_start(GTK_BOX(w), stack_btn, BtnGrow, BtnFill, BtnPad);


	split1;

	new_row;
	gtk_box_pack_start(GTK_BOX(w), watch_btn, BtnGrow, BtnFill, BtnPad);
	gtk_box_pack_start(GTK_BOX(w), break_btn, BtnGrow, BtnFill, BtnPad);

	new_row;
	gtk_box_pack_start(GTK_BOX(w), env_btn, BtnGrow, BtnFill, BtnPad);
	gtk_box_pack_start(GTK_BOX(w), prefs_btn, BtnGrow, BtnFill, BtnPad);

	split1;

	w = vbox;
	term_chk = gtk_check_button_new_with_label(_("Run in terminal"));
	gtk_box_pack_start(GTK_BOX(w), term_chk, FALSE, FALSE, 0);
	gdbui_set_tip(term_chk, _("Execute target program inside a terminal window."));


	pipe_chk = gtk_check_button_new_with_label(_("Ignore SIGPIPE"));
	gdbui_set_tip(pipe_chk,
		      _("Don't pause execution when target gets a SIGPIPE signal.\n"
		      "(Useful for certain networking applications.)"));
	gtk_box_pack_start(GTK_BOX(w), pipe_chk, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(pipe_chk), "clicked", G_CALLBACK(pipe_click), NULL);

	split2;

	con_lab = gtk_label_new(_("Console:"));
	gdbui_set_tip(con_lab, _("Send a GDB command directly to the debugger."));
	gtk_misc_set_alignment(GTK_MISC(con_lab), 0.0f, 0.0f);
	gtk_box_pack_start(GTK_BOX(vbox), con_lab, FALSE, FALSE, 0);

	con_cmd = gtk_entry_new();
	g_signal_connect(G_OBJECT(con_cmd), "activate", G_CALLBACK(entry_activate), NULL);
	gtk_box_pack_start(GTK_BOX(vbox), con_cmd, FALSE, FALSE, 0);

	gtk_widget_show_all(vbox);

	gdbio_setup.error_func = err_func;
	gdbio_setup.signal_func = signal_func;
	gdbio_setup.status_func = status_func;
	gdbio_setup.step_func = step_func;
	gdbio_setup.info_func = info_func;
	last_used = load_btn;
	status_func(curr_status);

	return vbox;
}





#ifdef STANDALONE

static void
quit()
{
	gdbio_exit();
	g_printerr("\n");
	gtk_main_quit();
}



static void
quit_click(GtkWidget * btn, gpointer user_data)
{
	quit();
}



gint
main(gint argc, gchar * argv[])
{
	GtkWidget *quit_btn;
	GtkWidget *vbox;
	gtk_init(&argc, &argv);
	gdbui_setup.main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(gdbui_setup.main_window), "debug");
	g_signal_connect(G_OBJECT(gdbui_setup.main_window), "destroy", quit, NULL);
	vbox = gdbui_create_widgets();
	gtk_box_pack_start(GTK_BOX(vbox), gtk_hseparator_new(), TRUE, TRUE, 4);
	quit_btn = make_btn(_("_Quit"), quit_click, _("Exit everything"));
	gtk_box_pack_start(GTK_BOX(vbox), quit_btn, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(gdbui_setup.main_window), vbox);
	gtk_widget_show_all(gdbui_setup.main_window);
	gtk_main();
	gdbui_opts_done();
	return 0;
}

#endif /* STANDALONE */
