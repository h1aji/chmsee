/*
 *  Copyright (C) 2010 Ji YongGang <jungleji@gmail.com>
 *  Copyright (C) 2009 LI Daobing <lidaobing@gmail.com>
 *
 *  ChmSee is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.

 *  ChmSee is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with ChmSee; see the file COPYING.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

/***************************************************************************
 *   Copyright (C) 2003 by zhong                                           *
 *   zhongz@163.com                                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <gdk/gdkkeysyms.h>

#include "chmsee.h"
#include "setup.h"
#include "link.h"
#include "utils.h"
#include "components/book.h"
#include "components/html-webkit.h"
#include "models/chmfile.h"

typedef struct _ChmseePrivate ChmseePrivate;

struct _ChmseePrivate {
        GtkWidget       *menubar;
        GtkWidget       *toolbar;
        GtkWidget       *book;
        GtkWidget       *statusbar;

        CsChmfile       *chmfile;
        CsConfig        *config;

        GtkActionGroup  *action_group;
        GtkUIManager    *ui_manager;
        guint            scid_default;

        gint             state; /* see enum CHMSEE_STATE_* */
};

enum {
        CHMSEE_STATE_INIT,    /* init state, no book is loaded */
        CHMSEE_STATE_LOADING, /* loading state, don't pop up an error window when open homepage failed */
        CHMSEE_STATE_NORMAL   /* normal state, one book is loaded */
};

static const GtkTargetEntry view_drop_targets[] = {
        { "text/uri-list", 0, 0 }
};

static const char *ui_description =
        "<ui>"
        "  <menubar name='MainMenu'>"
        "    <menu action='FileMenu'>"
        "      <menuitem action='Open'/>"
        "      <menuitem action='RecentFiles'/>"
        "      <separator/>"
        "      <menuitem action='NewTab'/>"
        "      <menuitem action='CloseTab'/>"
        "      <separator/>"
        "      <menuitem action='Exit'/>"
        "    </menu>"
        "    <menu action='EditMenu'>"
        "      <menuitem action='Copy'/>"
        "      <menuitem action='SelectAll'/>"
        "      <separator/>"
        "      <menuitem action='Find'/>"
        "      <menuitem action='Preferences'/>"
        "    </menu>"
        "    <menu action='ViewMenu'>"
        "      <menuitem action='FullScreen'/>"
        "      <menuitem action='SidePane'/>"
        "      <separator/>"
        "      <menuitem action='Home'/>"
        "      <menuitem action='Back'/>"
        "      <menuitem action='Forward'/>"
        "      <menuitem action='Prev'/>"
        "      <menuitem action='Next'/>"
        "      <separator/>"
        "      <menuitem action='ZoomIn'/>"
        "      <menuitem action='ZoomReset'/>"
        "      <menuitem action='ZoomOut'/>"
        "    </menu>"
        "    <menu action='HelpMenu'>"
        "      <separator/>"
        "      <menuitem action='About'/>"
        "    </menu>"
        "  </menubar>"
        "  <toolbar name='toolbar'>"
        "    <toolitem action='Open'/>"
        "    <separator/>"
        "    <toolitem action='SidePane' name='sidepane'/>"
        "    <toolitem action='Back'/>"
        "    <toolitem action='Forward'/>"
        "    <toolitem action='Home'/>"
        "    <toolitem action='Prev'/>"
        "    <toolitem action='Next'/>"
        "    <toolitem action='ZoomIn'/>"
        "    <toolitem action='ZoomReset'/>"
        "    <toolitem action='ZoomOut'/>"
        "    <toolitem action='Preferences'/>"
        "    <toolitem action='About'/>"
        "  </toolbar>"
        "  <accelerator action='OnKeyboardEscape'/>"
        "  <accelerator action='OnKeyboardControlEqual'/>"
        "</ui>";

#define CHMSEE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), CHMSEE_TYPE, ChmseePrivate))

static void chmsee_class_init(ChmseeClass *);
static void chmsee_init(Chmsee *);
static void chmsee_finalize(GObject *);
static void chmsee_dispose(GObject *);

static gboolean delete_cb(GtkWidget *, GdkEvent *, Chmsee *);
static gboolean window_state_event_cb(Chmsee *, GdkEventWindowState *);
static gboolean configure_event_cb(GtkWidget *, GdkEventConfigure *, Chmsee *);
static void book_model_changed_cb(Chmsee *, CsChmfile *, const gchar *);
static void book_html_changed_cb(Chmsee *, CsBook *);
static void book_message_notify_cb(Chmsee *, GParamSpec *, CsBook *);

static void open_file_response_cb(GtkWidget *, gint, Chmsee *);
static void about_response_cb(GtkDialog *, gint, gpointer);

static void on_open_file(GtkAction *, Chmsee *);
static void on_recent_files(GtkRecentChooser *, Chmsee *);
static void on_open_new_tab(GtkAction *, Chmsee *);
static void on_close_current_tab(GtkAction *, Chmsee *);

static void on_quit(GtkAction *, Chmsee *);
static void on_menu_file(GtkAction *, Chmsee *);
static void on_menu_edit(GtkAction *, Chmsee *);
static void on_home(GtkAction *, Chmsee *);
static void on_back(GtkAction *, Chmsee *);
static void on_forward(GtkAction *, Chmsee *);
static void on_prev(GtkAction *, Chmsee *);
static void on_next(GtkAction *, Chmsee *);
static void on_zoom_in(GtkAction *, Chmsee *);
static void on_zoom_reset(GtkAction *, Chmsee *);
static void on_zoom_out(GtkAction *, Chmsee *);
static void on_setup(GtkAction *, Chmsee *);
static void on_about(GtkAction *);
static void on_copy(GtkAction *, Chmsee *);
static void on_select_all(GtkAction *, Chmsee *);
static void on_find(GtkAction *, Chmsee *);
static void on_keyboard_escape(GtkAction *, Chmsee *);
static void on_fullscreen_toggled(GtkToggleAction *, Chmsee *);
static void on_sidepane_toggled(GtkToggleAction *, Chmsee *);

static void populate_windows(Chmsee *);
static void set_fullscreen(Chmsee *, gboolean);
static void set_sidepane_state(Chmsee *, gboolean);
static void open_draged_file(Chmsee *, const gchar *);
static void drag_data_received(GtkWidget *, GdkDragContext *, gint, gint,
                               GtkSelectionData *, guint, guint);
static void update_status_bar(Chmsee *, const gchar *);

/* Normal items */
static const GtkActionEntry entries[] = {
        { "FileMenu", NULL, N_("_File"), NULL, NULL, G_CALLBACK(on_menu_file) },
        { "EditMenu", NULL, N_("_Edit"), NULL, NULL, G_CALLBACK(on_menu_edit) },
        { "ViewMenu", NULL, N_("_View") },
        { "HelpMenu", NULL, N_("_Help") },

        { "Open", GTK_STOCK_OPEN, N_("_Open"), "<control>O", N_("Open a file"), G_CALLBACK(on_open_file)},
        { "RecentFiles", NULL, N_("_Recent Files"), NULL, NULL, NULL},

        { "NewTab", NULL, N_("New _Tab"), "<control>T", NULL, G_CALLBACK(on_open_new_tab)},
        { "CloseTab", NULL, N_("_Close Tab"), "<control>W", NULL, G_CALLBACK(on_close_current_tab)},
        { "Exit", GTK_STOCK_QUIT, N_("E_xit"), "<control>Q", N_("Exit ChmSee"), G_CALLBACK(on_quit)},

        { "Copy", NULL, N_("_Copy"), NULL, NULL, G_CALLBACK(on_copy)},
        { "SelectAll", NULL, N_("Select _All"), NULL, NULL, G_CALLBACK(on_select_all)},

        { "Find", GTK_STOCK_FIND, N_("_Find"), "<control>F", NULL, G_CALLBACK(on_find)},

        { "Preferences", GTK_STOCK_PREFERENCES, N_("_Preferences"), NULL, N_("Preferences"), G_CALLBACK(on_setup)},

        { "Home", GTK_STOCK_HOME, N_("_Home"), NULL, NULL, G_CALLBACK(on_home)},
        { "Back", GTK_STOCK_GO_BACK, N_("_Back"), "<alt>Left", NULL, G_CALLBACK(on_back)},
        { "Forward", GTK_STOCK_GO_FORWARD, N_("_Forward"), "<alt>Right", NULL, G_CALLBACK(on_forward)},
        { "Prev", GTK_STOCK_GO_UP, N_("_Prev"), "<control>Up", NULL, G_CALLBACK(on_prev)},
        { "Next", GTK_STOCK_GO_DOWN, N_("_Next"), "<control>Down", NULL, G_CALLBACK(on_next)},

        { "About", GTK_STOCK_ABOUT, N_("_About"), NULL, N_("About ChmSee"), G_CALLBACK(on_about)},

        { "ZoomIn", GTK_STOCK_ZOOM_IN, N_("Zoom _In"), "<control>plus", NULL, G_CALLBACK(on_zoom_in)},
        { "ZoomReset", GTK_STOCK_ZOOM_100, N_("_Normal Size"), "<control>0", NULL, G_CALLBACK(on_zoom_reset)},
        { "ZoomOut", GTK_STOCK_ZOOM_OUT, N_("Zoom _Out"), "<control>minus", NULL, G_CALLBACK(on_zoom_out)},

        { "OnKeyboardEscape", NULL, NULL, "Escape", NULL, G_CALLBACK(on_keyboard_escape)},
        { "OnKeyboardControlEqual", NULL, NULL, "<control>equal", NULL, G_CALLBACK(on_zoom_in)}
};

/* Toggle items */
static const GtkToggleActionEntry toggle_entries[] = {
        { "FullScreen", NULL, N_("Full _Screen"), "F11", "Switch between fullscreen and window mode", G_CALLBACK(on_fullscreen_toggled), FALSE },
        { "SidePane", NULL, N_("Side _Pane"), "F9", NULL, G_CALLBACK(on_sidepane_toggled), FALSE }
};

/* Radio items */
static const GtkRadioActionEntry radio_entries[] = {
};

static const gchar *active_actions[] = {
        "NewTab", "CloseTab", "SelectAll", "Home", "Find", "SidePane",
        "ZoomIn", "ZoomOut", "ZoomReset", "Back", "Forward", "Prev", "Next",
        ""
};

/* GObject functions */

G_DEFINE_TYPE (Chmsee, chmsee, GTK_TYPE_WINDOW);

static void
chmsee_class_init(ChmseeClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        g_type_class_add_private(klass, sizeof(ChmseePrivate));

        object_class->finalize = chmsee_finalize;
        object_class->dispose  = chmsee_dispose;

        GTK_WIDGET_CLASS(klass)->drag_data_received = drag_data_received;
}

static void
chmsee_init(Chmsee *self)
{
        ChmseePrivate *priv = CHMSEE_GET_PRIVATE (self);

        priv->chmfile = NULL;
        priv->config  = NULL;
        priv->state   = CHMSEE_STATE_INIT;

        gtk_widget_add_events(GTK_WIDGET(self),
                              GDK_STRUCTURE_MASK | GDK_BUTTON_PRESS_MASK);

        g_signal_connect(G_OBJECT (self),
                         "window-state-event",
                         G_CALLBACK (window_state_event_cb),
                         NULL);

        gtk_drag_dest_set(GTK_WIDGET (self),
                          GTK_DEST_DEFAULT_ALL,
                          view_drop_targets,
                          G_N_ELEMENTS (view_drop_targets),
                          GDK_ACTION_COPY);

        /* quit event handle */
        g_signal_connect(G_OBJECT (self),
                         "delete-event",
                         G_CALLBACK (delete_cb),
                         self);

        /* start up html render engine */
        if(!cs_html_webkit_init_system()) {
                g_error("Initialize html render engine failed!");
                exit(1);
        }

        populate_windows(self);
}

static void
chmsee_dispose(GObject *gobject)
{
        g_debug("Chmsee >>> dispose");
        ChmseePrivate *priv = CHMSEE_GET_PRIVATE (CHMSEE (gobject));

        if (priv->chmfile != NULL) {
                /* save last opened page */
                const gchar *bookfolder = cs_chmfile_get_bookfolder(priv->chmfile);
                gchar *location = cs_book_get_location(CS_BOOK (priv->book));
                gchar *page = g_strrstr(location, bookfolder);

                if (page != NULL) {
                        page = page + strlen(bookfolder);
                        gchar *last_file = g_strdup_printf("%s::%s", priv->config->last_file, page);
                        g_free(priv->config->last_file);
                        priv->config->last_file = last_file;
                }

                g_free(location);

                g_object_unref(priv->chmfile);
                priv->chmfile = NULL;
        }

        if (priv->action_group != NULL) {
                g_object_unref(priv->action_group);
                g_object_unref(priv->ui_manager);
                priv->action_group = NULL;
                priv->ui_manager = NULL;
        }

        G_OBJECT_CLASS (chmsee_parent_class)->dispose(gobject);
}

static void
chmsee_finalize(GObject *object)
{
        g_debug("Chmsee >>> finalize");

        G_OBJECT_CLASS (chmsee_parent_class)->finalize (object);
}

/* Callbacks */

static gboolean
delete_cb(GtkWidget *widget, GdkEvent *event, Chmsee *self)
{
        g_debug("Chmsee >>> window delete");
        on_quit(NULL, self);
        return TRUE;
}

static gboolean
window_state_event_cb(Chmsee *self, GdkEventWindowState *event)
{
        g_return_val_if_fail(IS_CHMSEE (self), FALSE);
        g_return_val_if_fail(event->type == GDK_WINDOW_STATE, FALSE);

        ChmseePrivate *priv = CHMSEE_GET_PRIVATE (self);

        g_debug("Chmsee >>> on_window_state_event with event->changed_mask = %d and event->new_window_state = %d",
                event->changed_mask,
                event->new_window_state
                );

        if (event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN) {
                if (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN) {
                        if (priv->config->fullscreen) {
                                priv->config->fullscreen = TRUE;
                                gtk_widget_hide(priv->menubar);
                                gtk_widget_hide(priv->toolbar);
                                gtk_widget_hide(priv->statusbar);
                        } else {
                                g_warning("expect not fullscreen but got a fullscreen event, restored");
                                set_fullscreen(self, FALSE);
                                return TRUE;
                        }
                } else {
                        if (!priv->config->fullscreen) {
                                priv->config->fullscreen = FALSE;
                                gtk_widget_show(priv->menubar);
                                gtk_widget_show(priv->toolbar);
                                gtk_widget_show(priv->statusbar);
                        } else {
                                g_warning("expect fullscreen but got an unfullscreen event, restored");
                                set_fullscreen(self, TRUE);
                                return TRUE;
                        }
                }
        }

        return FALSE;
}

static gboolean
configure_event_cb(GtkWidget *widget, GdkEventConfigure *event, Chmsee *self)
{
        ChmseePrivate *priv = CHMSEE_GET_PRIVATE (self);

        if (!priv->config->fullscreen) {
                priv->config->width  = event->width;
                priv->config->height = event->height;
                priv->config->pos_x  = event->x;
                priv->config->pos_y  = event->y;
        }

        return FALSE;
}

static void
book_model_changed_cb(Chmsee *self, CsChmfile *chmfile, const gchar *filename)
{
        g_debug("Chmsee >>> receive book model changed callback %s", filename);
        ChmseePrivate *priv = CHMSEE_GET_PRIVATE (self);
        gboolean has_model = (chmfile != NULL);

        gint n = 0;
        while (strlen(active_actions[n])) {
                gtk_action_set_sensitive(gtk_action_group_get_action(priv->action_group, active_actions[n++]), has_model);
        }

        gtk_widget_set_sensitive(priv->book, has_model);

        if (filename
            && g_str_has_prefix(filename, "file://")
            && (g_str_has_suffix(filename, ".chm") || g_str_has_suffix(filename, ".CHM"))) {
                open_draged_file(self, filename);
        }
}

static void
book_html_changed_cb(Chmsee *self, CsBook *book)
{
        g_debug("Chmsee >>> recieve html_changed signal");

        ChmseePrivate *priv = CHMSEE_GET_PRIVATE (self);

        gtk_action_set_sensitive(gtk_action_group_get_action(priv->action_group, "Home"),
                                 cs_book_has_homepage(book));
        gtk_action_set_sensitive(gtk_action_group_get_action(priv->action_group, "Back"),
                                 cs_book_can_go_back(book));
        gtk_action_set_sensitive(gtk_action_group_get_action(priv->action_group, "Forward"),
                                 cs_book_can_go_forward(book));
}

static void
book_message_notify_cb(Chmsee *self, GParamSpec *pspec, CsBook *book)
{
        gchar *message;
        g_object_get(book, "book-message", &message, NULL);

        update_status_bar(self, message);
        g_free(message);
}

static void
open_file_response_cb(GtkWidget *widget, gint response_id, Chmsee *self)
{
        gchar *filename = NULL;

        if (response_id == GTK_RESPONSE_OK)
                filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (widget));

        gtk_widget_destroy(widget);

        if (filename != NULL) {
                chmsee_open_file(self, filename);
                g_free(filename);
        }
}

/* Toolbar button events */

static void
on_open_file(GtkAction *action, Chmsee *self)
{
        ChmseePrivate *priv = CHMSEE_GET_PRIVATE (self);

        /* create open file dialog */
        GtkBuilder *builder = gtk_builder_new();
        gtk_builder_add_from_file(builder, RESOURCE_FILE ("openfile-dialog.ui"), NULL);

        GtkWidget *dialog = BUILDER_WIDGET (builder, "openfile_dialog");

        g_signal_connect(G_OBJECT (dialog),
                         "response",
                         G_CALLBACK (open_file_response_cb),
                         self);

        /* file list fiter */
        GtkFileFilter *filter;
        filter = gtk_file_filter_new();
        gtk_file_filter_set_name(filter, _("CHM Files"));
        gtk_file_filter_add_pattern(filter, "*.[cC][hH][mM]");
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER (dialog), filter);

        filter = gtk_file_filter_new();
        gtk_file_filter_set_name(filter, _("All Files"));
        gtk_file_filter_add_pattern(filter, "*");
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER (dialog), filter);

        /* previous opened file folder */
        gchar *last_dir = NULL;
        if (priv->config->last_file) {
                last_dir = g_path_get_dirname(priv->config->last_file);
        } else {
                last_dir = g_strdup(g_get_home_dir());
        }
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER (dialog), last_dir);
        g_free(last_dir);

        g_object_unref(G_OBJECT (builder));
}

static void
on_recent_files(GtkRecentChooser *chooser, Chmsee *self)
{
        gchar *uri = gtk_recent_chooser_get_current_uri(chooser);

        if (uri != NULL) {
                gchar *filename = g_filename_from_uri(uri, NULL, NULL);

                chmsee_open_file(self, filename);
                g_free(filename);
                g_free(uri);
        }
}

static void
on_copy(GtkAction *action, Chmsee *self)
{
        cs_book_copy(CS_BOOK (CHMSEE_GET_PRIVATE (self)->book));
}

static void
on_select_all(GtkAction *action, Chmsee *self)
{
        cs_book_select_all(CS_BOOK (CHMSEE_GET_PRIVATE (self)->book));
}

static void
on_setup(GtkAction *action, Chmsee *self)
{
        setup_window_new(self);
}

static void
on_back(GtkAction *action, Chmsee *self)
{
        cs_book_go_back(CS_BOOK (CHMSEE_GET_PRIVATE (self)->book));
}

static void
on_forward(GtkAction *action, Chmsee *self)
{
        cs_book_go_forward(CS_BOOK (CHMSEE_GET_PRIVATE (self)->book));
}

static void
on_prev(GtkAction *action, Chmsee *self)
{
        cs_book_go_prev(CS_BOOK (CHMSEE_GET_PRIVATE (self)->book));
}

static void
on_next(GtkAction *action, Chmsee *self)
{
        cs_book_go_next(CS_BOOK (CHMSEE_GET_PRIVATE (self)->book));
}

static void
on_menu_file(GtkAction *action, Chmsee *self)
{
        ChmseePrivate *priv = CHMSEE_GET_PRIVATE (self);

        gtk_action_set_sensitive(gtk_action_group_get_action(priv->action_group, "CloseTab"),
                                 cs_book_can_close_tab(CS_BOOK (priv->book)));
}

static void
on_menu_edit(GtkAction *action, Chmsee *self)
{
        ChmseePrivate *priv = CHMSEE_GET_PRIVATE (self);

        gtk_action_set_sensitive(gtk_action_group_get_action(priv->action_group, "Copy"),
                                 cs_book_can_copy(CS_BOOK (priv->book)));
}

static void
on_home(GtkAction *action, Chmsee *self)
{
        cs_book_homepage(CS_BOOK (CHMSEE_GET_PRIVATE (self)->book));
}

static void
on_zoom_in(GtkAction *action, Chmsee *self)
{
        cs_book_zoom_in(CS_BOOK (CHMSEE_GET_PRIVATE (self)->book));
}

static void
on_zoom_out(GtkAction *action, Chmsee *self)
{
        cs_book_zoom_out(CS_BOOK (CHMSEE_GET_PRIVATE (self)->book));
}

static void
on_zoom_reset(GtkAction *action, Chmsee *self)
{
        cs_book_zoom_reset(CS_BOOK (CHMSEE_GET_PRIVATE (self)->book));
}

static void
about_response_cb(GtkDialog *dialog, gint response_id, gpointer user_data)
{
        if (response_id == GTK_RESPONSE_CANCEL)
                gtk_widget_destroy(GTK_WIDGET (dialog));
}

static void
on_about(GtkAction *action)
{
        GtkBuilder *builder = gtk_builder_new();
        gtk_builder_add_from_file(builder, RESOURCE_FILE ("about-dialog.ui"), NULL);

        GtkWidget *dialog = BUILDER_WIDGET (builder, "about_dialog");

        g_signal_connect(G_OBJECT (dialog),
                         "response",
                         G_CALLBACK (about_response_cb),
                         NULL);

        gtk_about_dialog_set_version(GTK_ABOUT_DIALOG (dialog), PACKAGE_VERSION);

        g_object_unref(builder);
}

static void
on_open_new_tab(GtkAction *action, Chmsee *self)
{
        ChmseePrivate *priv = CHMSEE_GET_PRIVATE (self);

        gchar *location = cs_book_get_location(CS_BOOK (priv->book));
        cs_book_new_tab_with_fulluri(CS_BOOK (priv->book), location);
        g_free(location);
}

static void
on_close_current_tab(GtkAction *action, Chmsee *self)
{
        cs_book_close_current_tab(CS_BOOK (CHMSEE_GET_PRIVATE (self)->book));
}

static void
on_keyboard_escape(GtkAction *action, Chmsee *self)
{
        cs_book_findbar_hide(CS_BOOK (CHMSEE_GET_PRIVATE (self)->book));
}

static void
on_fullscreen_toggled(GtkToggleAction *action, Chmsee *self)
{
        set_fullscreen(self, gtk_toggle_action_get_active(action));
}

static void
on_sidepane_toggled(GtkToggleAction *action, Chmsee *self)
{
        set_sidepane_state(self, gtk_toggle_action_get_active(action));
}

static void
on_find(GtkAction *action, Chmsee *self)
{
        cs_book_findbar_show(CS_BOOK (CHMSEE_GET_PRIVATE (self)->book));
}

/* internal functions */

static void
on_quit(GtkAction *action, Chmsee *self)
{
        g_message("Chmsee >>> quit");

        ChmseePrivate *priv = CHMSEE_GET_PRIVATE (self);

        cs_html_webkit_shutdown_system();
        priv->config->hpaned_pos = cs_book_get_hpaned_position(CS_BOOK (priv->book));
        gtk_widget_destroy(GTK_WIDGET (self));

        gtk_main_quit();
}

static void
drag_data_received(GtkWidget *widget, GdkDragContext *context, gint x, gint y,
                   GtkSelectionData *selection_data, guint info, guint time)
{
        gchar **uris;

        uris = gtk_selection_data_get_uris(selection_data);
        if (uris == NULL) {
                gtk_drag_finish(context, FALSE, FALSE, time);
                return;
        }

        gint i;
        for (i = 0; uris[i]; i++) {
                gchar *uri = uris[i];
                if (g_str_has_prefix(uri, "file://")
                    && (g_str_has_suffix(uri, ".chm") || g_str_has_suffix(uri, ".CHM"))) {
                        open_draged_file(CHMSEE (widget), uri);
                        break;
                }
        }

        gtk_drag_finish(context, TRUE, FALSE, time);

        g_strfreev(uris);
}

static void
open_draged_file(Chmsee *chmsee, const gchar *file)
{
        gchar *fname = g_uri_unescape_string(file+7, NULL); // +7 remove "file://" prefix
        chmsee_open_file(chmsee, fname);
        g_free(fname);
}

static void
populate_windows(Chmsee *self)
{
        ChmseePrivate *priv = CHMSEE_GET_PRIVATE (self);

        GtkWidget *vbox = GTK_WIDGET (gtk_vbox_new(FALSE, 2));

        priv->action_group = gtk_action_group_new("MenuActions");
        gtk_action_group_set_translation_domain(priv->action_group, NULL);
        gtk_action_group_add_actions(priv->action_group, entries, G_N_ELEMENTS (entries), self);
        gtk_action_group_add_toggle_actions(priv->action_group, toggle_entries, G_N_ELEMENTS (toggle_entries), self);

        gint n = 0;
        while (strlen(active_actions[n])) {
                gtk_action_set_sensitive(gtk_action_group_get_action(priv->action_group, active_actions[n++]), FALSE);
        }

        priv->ui_manager = gtk_ui_manager_new();
        gtk_ui_manager_insert_action_group(priv->ui_manager, priv->action_group, 0);

        gtk_window_add_accel_group(GTK_WINDOW (self),
                                   gtk_ui_manager_get_accel_group(priv->ui_manager));

        GError *error = NULL;
        if (!gtk_ui_manager_add_ui_from_string(priv->ui_manager, ui_description, -1, &error)) {
                g_warning("Chmsee >>> building menus failed: %s", error->message);
                g_error_free(error);
                exit(EXIT_FAILURE);
        }

        priv->menubar = gtk_handle_box_new();
        gtk_container_add(GTK_CONTAINER (priv->menubar), gtk_ui_manager_get_widget(priv->ui_manager, "/MainMenu"));
        gtk_box_pack_start(GTK_BOX (vbox), priv->menubar, FALSE, FALSE, 0);

        GtkWidget *recent_menu = gtk_recent_chooser_menu_new_for_manager(gtk_recent_manager_get_default());
        gtk_recent_chooser_set_show_not_found(GTK_RECENT_CHOOSER (recent_menu), FALSE);
        gtk_recent_chooser_set_local_only(GTK_RECENT_CHOOSER (recent_menu), TRUE);
        gtk_recent_chooser_set_limit(GTK_RECENT_CHOOSER (recent_menu), 10);
        gtk_recent_chooser_set_show_icons(GTK_RECENT_CHOOSER (recent_menu), FALSE);
        gtk_recent_chooser_set_sort_type(GTK_RECENT_CHOOSER (recent_menu), GTK_RECENT_SORT_MRU);
        gtk_recent_chooser_menu_set_show_numbers(GTK_RECENT_CHOOSER_MENU (recent_menu), TRUE);

        GtkRecentFilter *filter = gtk_recent_filter_new();
        gtk_recent_filter_add_application(filter, g_get_application_name());
        gtk_recent_chooser_set_filter(GTK_RECENT_CHOOSER (recent_menu), filter);

        g_signal_connect(recent_menu,
                         "item-activated",
                         G_CALLBACK (on_recent_files),
                         self);

        gtk_menu_item_set_submenu(GTK_MENU_ITEM (gtk_ui_manager_get_widget(priv->ui_manager, "/MainMenu/FileMenu/RecentFiles")),
                                  recent_menu);

        priv->toolbar = gtk_handle_box_new();
        gtk_container_add(GTK_CONTAINER (priv->toolbar), gtk_ui_manager_get_widget(priv->ui_manager, "/toolbar"));
        gtk_box_pack_start(GTK_BOX (vbox), priv->toolbar, FALSE, FALSE, 0);
        /* gtk_toolbar_set_style(GTK_TOOLBAR (gtk_ui_manager_get_widget(ui_manager, "/toolbar")), */
        /*                       GTK_TOOLBAR_ICONS);// FIXME: issue 43 */
        gtk_tool_button_set_icon_widget(
                GTK_TOOL_BUTTON(gtk_ui_manager_get_widget(priv->ui_manager, "/toolbar/sidepane")),
                gtk_image_new_from_file(RESOURCE_FILE ("show-pane.png")));

        priv->book = cs_book_new();
        gtk_box_pack_start(GTK_BOX(vbox), priv->book, TRUE, TRUE, 0);
        g_signal_connect_swapped(priv->book,
                                 "model-changed",
                                 G_CALLBACK (book_model_changed_cb),
                                 self);

        /* status bar */
        priv->statusbar = gtk_statusbar_new();
        gtk_box_pack_start(GTK_BOX(vbox), priv->statusbar, FALSE, FALSE, 0);

        priv->scid_default = gtk_statusbar_get_context_id(GTK_STATUSBAR (priv->statusbar), "default");

        gtk_container_add(GTK_CONTAINER (self), vbox);

        update_status_bar(self, _("Ready!"));
        gtk_widget_show_all(GTK_WIDGET (self));
        cs_book_findbar_hide(CS_BOOK (priv->book));
        g_debug("Chmsee >>> populate window finished.");
}

static void
set_fullscreen(Chmsee *self, gboolean fullscreen)
{
        g_debug("Chmsee >>> set_fullscreen with fullscreen = %d", fullscreen);

        ChmseePrivate *priv = CHMSEE_GET_PRIVATE (self);
        priv->config->fullscreen = fullscreen;

        if (fullscreen) {
                g_debug("Chmsee >>> call gtk_window_fullscreen");
                gtk_window_fullscreen(GTK_WINDOW (self));
        } else {
                g_debug("ChmSee >>> call gtk_window_unfullscreen");
                gtk_window_unfullscreen(GTK_WINDOW (self));
        }
}

static void
update_status_bar(Chmsee *self, const gchar *message)
{
        ChmseePrivate *priv = CHMSEE_GET_PRIVATE (self);
        gchar *status = g_strdup_printf(" %s", message);

        gtk_statusbar_pop(GTK_STATUSBAR(priv->statusbar), priv->scid_default);
        gtk_statusbar_push(GTK_STATUSBAR(priv->statusbar), priv->scid_default, status);

        g_free(status);
}

static void
set_sidepane_state(Chmsee *self, gboolean state)
{
        ChmseePrivate *priv = CHMSEE_GET_PRIVATE (self);

        g_object_set(priv->book,
                     "sidepane-visible", state,
                     NULL);
        const gchar *pane_icon = state ? RESOURCE_FILE ("hide-pane.png") : RESOURCE_FILE ("show-pane.png");

        GtkWidget *icon_widget = gtk_image_new_from_file(pane_icon);
        gtk_widget_show(icon_widget);

        gtk_tool_button_set_icon_widget(
                GTK_TOOL_BUTTON (gtk_ui_manager_get_widget(priv->ui_manager, "/toolbar/sidepane")),
                icon_widget);
};

/* External functions */

Chmsee *
chmsee_new(CsConfig *config)
{
        Chmsee        *self = g_object_new(CHMSEE_TYPE, NULL);
        ChmseePrivate *priv = CHMSEE_GET_PRIVATE (self);

        priv->config = config;

        if (config->pos_x >= 0 && config->pos_y >= 0)
                gtk_window_move(GTK_WINDOW (self), config->pos_x, config->pos_y);

        if (config->width <= 0 || config->height <= 0) {
                config->width = DEFAULT_WIDTH;
                config->height = DEFAULT_HEIGHT;
        }
        gtk_window_resize(GTK_WINDOW (self), config->width, config->height);

        gtk_window_set_title(GTK_WINDOW (self), "ChmSee");
        gtk_window_set_icon_from_file(GTK_WINDOW (self), RESOURCE_FILE ("chmsee-icon.png"), NULL);

        cs_book_set_hpaned_position(CS_BOOK (priv->book), config->hpaned_pos);
        set_sidepane_state(self, FALSE); //hide

        /* widget size changed event handle */
        g_signal_connect(G_OBJECT (self),
                         "configure-event",
                         G_CALLBACK (configure_event_cb),
                         self);

        g_message("Chmsee >>> created");
        return self;
}

void
chmsee_open_file(Chmsee *self, const gchar *filename)
{
        g_return_if_fail(IS_CHMSEE (self));

        g_message("Chmsee >>> open file = %s", filename);
        ChmseePrivate *priv = CHMSEE_GET_PRIVATE (self);

        /* create chmfile, get file infomation */
        CsChmfile *new_chmfile = cs_chmfile_new(filename, priv->config->bookshelf);

        if (new_chmfile != NULL) {
                /* close currently opened book */
                if (priv->chmfile != NULL)
                        g_object_unref(priv->chmfile);

                priv->chmfile = new_chmfile;

                /* set global charset and font to this file */
                if (!strlen(cs_chmfile_get_charset(priv->chmfile)) && strlen(priv->config->charset))
                        cs_chmfile_set_charset(priv->chmfile, priv->config->charset);
                if (!strlen(cs_chmfile_get_variable_font(priv->chmfile)))
                        cs_chmfile_set_variable_font(priv->chmfile, priv->config->variable_font);
                if (!strlen(cs_chmfile_get_fixed_font(priv->chmfile)))
                        cs_chmfile_set_fixed_font(priv->chmfile, priv->config->fixed_font);

                priv->state = CHMSEE_STATE_LOADING;

                cs_book_set_model(CS_BOOK (priv->book), priv->chmfile);

                gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON (gtk_ui_manager_get_widget(priv->ui_manager, "/toolbar/sidepane")), TRUE);
                gtk_container_set_focus_child(GTK_CONTAINER(self), priv->book);

                g_signal_connect_swapped(priv->book,
                                         "html-changed",
                                         G_CALLBACK (book_html_changed_cb),
                                         self);
                g_signal_connect_swapped(priv->book,
                                         "notify::book-message",
                                         G_CALLBACK (book_message_notify_cb),
                                         self);

                /* update window title */
                gchar *window_title = g_strdup_printf("%s - ChmSee", cs_chmfile_get_bookname(priv->chmfile));
                g_debug("Chmsee >>> update window title %s", window_title);
                gtk_window_set_title(GTK_WINDOW (self), window_title);
                g_free(window_title);

                /* record last opened file */
                if (priv->config->last_file != NULL)
                        g_free(priv->config->last_file);

                priv->config->last_file = g_strdup(cs_chmfile_get_filename(priv->chmfile));
                g_debug("Chmsee >>> record last file =  %s", priv->config->last_file);

                /* recent files */
                gchar *content;
                gsize length;

                if (g_file_get_contents(filename, &content, &length, NULL)) {
                        static gchar *groups[] = {
                                "CHM Viewer",
                                NULL
                        };

                        GtkRecentData *data = g_slice_new(GtkRecentData);
                        data->display_name = NULL;
                        data->description = NULL;
                        data->mime_type = "application/x-chm";
                        data->app_name = (gchar*)g_get_application_name();
                        data->app_exec = g_strjoin(" ", g_get_prgname(), "%u", NULL);
                        data->groups = groups;
                        data->is_private = FALSE;

                        gchar *uri = g_filename_to_uri(filename, NULL, NULL);

                        GtkRecentManager *manager = gtk_recent_manager_get_default();
                        gtk_recent_manager_add_full(manager, uri, data);

                        g_free(uri);
                        g_free(data->app_exec);
                        g_slice_free(GtkRecentData, data);
                }

                priv->state = CHMSEE_STATE_NORMAL;
        } else {
                g_warning("CHMSEE >>> Can not open spectified file %s", filename);

                /* Popup an error message dialog */
                GtkWidget *msg_dialog;

                msg_dialog = gtk_message_dialog_new(GTK_WINDOW (self),
                                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    GTK_MESSAGE_ERROR,
                                                    GTK_BUTTONS_CLOSE,
                                                    _("Error: Can not open spectified file '%s'"),
                                                    filename);
                gtk_window_set_position(GTK_WINDOW (msg_dialog), GTK_WIN_POS_CENTER);
                gtk_dialog_run(GTK_DIALOG (msg_dialog));
                gtk_widget_destroy(msg_dialog);
        }
}

gboolean
chmsee_get_startup_lastfile(Chmsee *self)
{
        g_return_val_if_fail(IS_CHMSEE (self), FALSE);

        return CHMSEE_GET_PRIVATE (self)->config->startup_lastfile;
}

void
chmsee_set_startup_lastfile(Chmsee *self, gboolean state)
{
        g_return_if_fail(IS_CHMSEE (self));

        g_debug("Chmsee >>> set startup lastfile = %d", state);
        CHMSEE_GET_PRIVATE (self)->config->startup_lastfile = state;
}

const gchar *
chmsee_get_variable_font(Chmsee *self)
{
        g_return_val_if_fail(IS_CHMSEE (self), NULL);

        ChmseePrivate *priv = CHMSEE_GET_PRIVATE (self);
        if (priv->chmfile != NULL)
                return cs_chmfile_get_variable_font(priv->chmfile);
        else
                return priv->config->variable_font;
}

void
chmsee_set_variable_font(Chmsee *self, const gchar *font_name)
{
        g_return_if_fail(IS_CHMSEE (self));

        g_debug("Chmsee >>> set variable font = %s", font_name);
        ChmseePrivate *priv = CHMSEE_GET_PRIVATE (self);

        if (priv->chmfile != NULL) {
//                cs_html_webkit_set_variable_font(font_name);
                cs_chmfile_set_variable_font(priv->chmfile, font_name);
        } else {
                g_free(priv->config->variable_font);
                priv->config->variable_font = g_strdup(font_name);
        }
}

const gchar *
chmsee_get_fixed_font(Chmsee *self)
{
        g_return_val_if_fail(IS_CHMSEE (self), NULL);

        ChmseePrivate *priv = CHMSEE_GET_PRIVATE (self);

        if (priv->chmfile != NULL)
                return cs_chmfile_get_fixed_font(priv->chmfile);
        else
                return priv->config->fixed_font;
}

void
chmsee_set_fixed_font(Chmsee *self, const gchar *font_name)
{
        g_return_if_fail(IS_CHMSEE (self));

        ChmseePrivate *priv = CHMSEE_GET_PRIVATE (self);

        if (priv->chmfile != NULL) {
        ///        cs_html_webkit_set_fixed_font(font_name);
                cs_chmfile_set_fixed_font(priv->chmfile, font_name);
        } else {
                g_free(priv->config->fixed_font);
                priv->config->fixed_font = g_strdup(font_name);
        }
}

const gchar *
chmsee_get_charset(Chmsee *self)
{
        g_return_val_if_fail(IS_CHMSEE (self), NULL);

        ChmseePrivate *priv = CHMSEE_GET_PRIVATE (self);
        if (priv->chmfile != NULL)
                return cs_chmfile_get_charset(priv->chmfile);
        else
                return priv->config->charset;
}

void
chmsee_set_charset(Chmsee *self, const gchar *charset)
{
        g_return_if_fail(IS_CHMSEE (self));

        ChmseePrivate *priv = CHMSEE_GET_PRIVATE (self);

        if (priv->chmfile != NULL) {
                cs_chmfile_set_charset(priv->chmfile, charset);
                cs_book_reload_current_page(CS_BOOK (priv->book));
        } else {
                g_free(priv->config->charset);
                priv->config->charset = g_strdup(charset);
        }
}

gboolean
chmsee_has_book(Chmsee *self)
{
        g_return_val_if_fail(IS_CHMSEE (self), FALSE);

        return CHMSEE_GET_PRIVATE (self)->chmfile != NULL;
}

void
chmsee_close_book(Chmsee *self)
{
        g_return_if_fail(IS_CHMSEE (self));

        ChmseePrivate *priv = CHMSEE_GET_PRIVATE (self);

        if (priv->chmfile != NULL) {
                g_object_unref(priv->chmfile);
                priv->chmfile = NULL;
        }

        book_model_changed_cb(self, NULL, NULL);
        priv->state = CHMSEE_STATE_NORMAL;
}

const gchar *
chmsee_get_bookshelf(Chmsee *self)
{
        g_return_val_if_fail(IS_CHMSEE (self), NULL);

        return CHMSEE_GET_PRIVATE (self)->config->bookshelf;
}
