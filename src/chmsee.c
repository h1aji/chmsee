/*
 *  Copyright (C) 2006 Ji YongGang <jungle@soforge-studio.com>
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
#include "chmsee.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <unistd.h>             /* R_OK */

#include <glib.h>
#include <glib/gstdio.h>
#include <gdk/gdkkeysyms.h>
#include <glade/glade.h>

#include "ihtml.h"
#include "html-factory.h"
#include "booktree.h"
#include "ui_bookmarks.h"
#include "ui_index.h"
#include "setup.h"
#include "link.h"
#include "utils/utils.h"

#include "models/chmfile-factory.h"

enum {
	CHMSEE_STATE_INIT,    /* init state, no book is loaded */
	CHMSEE_STATE_LOADING, /* loading state, don't pop up an error window when open homepage failed */
	CHMSEE_STATE_NORMAL   /* normal state, one book is loaded */
};

struct _ChmSeePrivate {
  GtkWidget* menubar;
  GtkWidget* toolbar;
    GtkWidget       *control_notebook;
    GtkWidget       *html_notebook;

    GtkWidget       *booktree;
    GtkWidget       *bookmark_tree;

    GtkWidget* uiIndex; /* the gtktreeview */
    GtkWidget* indexPage; /* the index tab under control_notebook */

    GtkWidget       *statusbar;


    GtkActionGroup* action_group;
    GtkUIManager* ui_manager;
    guint            scid_default;

    gboolean         has_toc;
    gboolean         has_index;
    gint             pos_x;
    gint             pos_y;
    gint             width;
    gint             height;
    gint             hpaned_position;
    gint             lang;
    gboolean         fullscreen;
    gboolean         expect_fullscreen;

    ChmseeIchmfile  *book;

    gchar           *home;
    gchar           *cache_dir;
    gchar           *last_dir;
    gchar* context_menu_link;
    gint            state; /* see enum CHMSEE_STATE_* */
};



#define selfp (self->priv)
#define CHMSEE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TYPE_CHMSEE, ChmSeePrivate))

static void chmsee_class_init(ChmSeeClass *);
static void chmsee_init(ChmSee *);
static void chmsee_finalize(GObject *);
static void chmsee_dispose(GObject* self);
static void chmsee_load_config(ChmSee *self);
static void chmsee_save_config(ChmSee *self);
static void chmsee_set_fullscreen(ChmSee* self, gboolean fullscreen);
static void chmsee_set_context_menu_link(ChmSee* self, const gchar* link);

static void chmsee_refresh_index(ChmSee* self);
static GtkWidget* chmsee_new_index_page(ChmSee* self);
static void chmsee_on_ui_index_link_selected(ChmSee* self, Link* link);

static gboolean delete_cb(GtkWidget *, GdkEvent *, ChmSee *);
static void destroy_cb(GtkWidget *, ChmSee *);
static gboolean on_configure_event(GtkWidget *, GdkEventConfigure *, ChmSee *);

static gboolean on_keypress_event(GtkWidget *, GdkEventKey *, ChmSee *);
static void open_response_cb(GtkWidget *, gint, ChmSee *);
static void about_response_cb(GtkDialog *, gint, gpointer);
static void booktree_link_selected_cb(GObject *, Link *, ChmSee *);
static void bookmarks_link_selected_cb(GObject *, Link *, ChmSee *);
static void control_switch_page_cb(GtkNotebook *, GtkNotebookPage *, guint , ChmSee *);
static void html_switch_page_cb(GtkNotebook *, GtkNotebookPage *, guint , ChmSee *);
static void html_location_changed_cb(ChmseeIhtml *, const gchar *, ChmSee *);
static gboolean html_open_uri_cb(ChmseeIhtml *, const gchar *, ChmSee *);
static void html_title_changed_cb(ChmseeIhtml *, const gchar *, ChmSee *);
static void html_context_normal_cb(ChmseeIhtml *, ChmSee *);
static void html_context_link_cb(ChmseeIhtml *, const gchar *, ChmSee *);
static void html_open_new_tab_cb(ChmseeIhtml *, const gchar *, ChmSee *);
static void html_link_message_cb(ChmseeIhtml *, const gchar *, ChmSee *);
static void show_sidepane(ChmSee* self);
static void hide_sidepane(ChmSee* self);
static void set_sidepane_state(ChmSee* self, gboolean state);

static void on_open(GtkWidget *, ChmSee *);
static void on_close_tab(GtkWidget *, ChmSee *);
static void on_setup(GtkWidget *, ChmSee *);
static void on_copy(GtkWidget *, ChmSee *);
static void on_copy_page_location(GtkWidget*, ChmSee*);
static void on_select_all(GtkWidget *, ChmSee *);
static void on_back(GtkWidget *, ChmSee *);
static void on_forward(GtkWidget *, ChmSee *);
static void on_home(GtkWidget *, ChmSee *);
static void on_zoom_in(GtkWidget *, ChmSee *);
static void on_zoom_reset(GtkWidget *, ChmSee *);
static void on_zoom_out(GtkWidget *, ChmSee *);
static void on_about(GtkWidget *);
static void on_open_new_tab(GtkWidget *, ChmSee *);
static void on_close_current_tab(GtkWidget *, ChmSee *);
static void on_context_new_tab(GtkWidget *, ChmSee *);
static void on_context_copy_link(GtkWidget *, ChmSee *);
static void on_fullscreen_toggled(GtkWidget*, ChmSee* self);
static void on_sidepane_toggled(GtkWidget*, ChmSee* self);
static void on_map(ChmSee* self);
static gboolean on_window_state_event(ChmSee* self, GdkEventWindowState* event);
static gboolean on_scroll_event(ChmSee* self, GdkEventScroll* event);

static void chmsee_quit(ChmSee *);
static void chmsee_open_uri(ChmSee *chmsee, const gchar *uri);
static void chmsee_open_file(ChmSee *self, const gchar *filename);
static GtkWidget *get_widget(ChmSee *, gchar *);
static void populate_window(ChmSee *);
static void close_current_book(ChmSee *);
static void new_tab(ChmSee *, const gchar *);
static ChmseeIhtml *get_active_html(ChmSee *);
static void check_history(ChmSee *, ChmseeIhtml *);
static void update_tab_title(ChmSee *, ChmseeIhtml *);
static void tab_set_title(ChmSee *, ChmseeIhtml *, const gchar *);
static void open_homepage(ChmSee *);
static void reload_current_page(ChmSee *);
static void update_status_bar(ChmSee *, const gchar *);
static void
chmsee_drag_data_received (GtkWidget          *widget,
                           GdkDragContext     *context,
                           gint                x,
                           gint                y,
                           GtkSelectionData   *selection_data,
                           guint               info,
                           guint               time);

/* static gchar *context_menu_link = NULL; */
static const GtkTargetEntry view_drop_targets[] = {
	{ "text/uri-list", 0, 0 }
};

/* Normal items */
static const GtkActionEntry entries[] = {
  { "FileMenu", NULL, "_File" },
  { "EditMenu", NULL, "_Edit" },
  { "ViewMenu", NULL, "_View" },
  { "HelpMenu", NULL, "_Help" },

  { "Open", GTK_STOCK_OPEN, "_Open", "<control>O", "Open a file", G_CALLBACK(on_open)},
  { "NewTab", NULL, "_New Tab", "<control>T", NULL, G_CALLBACK(on_open_new_tab)},
  { "CloseTab", NULL, "_Close Tab", "<control>W", NULL, G_CALLBACK(on_close_current_tab)},
  { "Exit", GTK_STOCK_QUIT, "E_xit", "<control>Q", "Exit the program", G_CALLBACK(destroy_cb)},

  { "Copy", GTK_STOCK_COPY, "_Copy", "<control>C", NULL, G_CALLBACK(on_copy)},
  { "Preferences", GTK_STOCK_PREFERENCES, "_Preferences", NULL, NULL, G_CALLBACK(on_setup)},

  { "Home", GTK_STOCK_HOME, "_Home", NULL, NULL, G_CALLBACK(on_home)},
  { "Back", GTK_STOCK_GO_BACK, "_Back", NULL, NULL, G_CALLBACK(on_back)},
  { "Forward", GTK_STOCK_GO_FORWARD, "_Forward", NULL, NULL, G_CALLBACK(on_forward)},

  { "About", GTK_STOCK_ABOUT, "_About", NULL, NULL, G_CALLBACK(on_about)},

  { "ZoomIn", GTK_STOCK_ZOOM_IN, "Zoom _In", "plus", "Zoom into the image", G_CALLBACK(on_zoom_in)},
  { "ZoomReset", GTK_STOCK_ZOOM_100, "Normal Size", NULL, NULL, G_CALLBACK(on_zoom_reset)},
  { "ZoomOut", GTK_STOCK_ZOOM_OUT, "Zoom _Out", "minus", "Zoom away from the image", G_CALLBACK(on_zoom_out)},
  
  { "OpenLinkInNewTab", NULL, "Open Link in New _Tab", NULL, NULL, G_CALLBACK(on_context_new_tab)},
  { "CopyLinkLocation", NULL, "_Copy Link Location", NULL, NULL, G_CALLBACK(on_context_copy_link)},
  { "SelectAll", NULL, "Select _All", NULL, NULL, G_CALLBACK(on_select_all)},
  { "CopyPageLocation", NULL, "Copy Page _Location", NULL, NULL, G_CALLBACK(on_copy_page_location)}
};

/* Toggle items */
static const GtkToggleActionEntry toggle_entries[] = {
  { "FullScreen", NULL, "_Full Screen", "F11", "Switch between full screen and windowed mode", G_CALLBACK(on_fullscreen_toggled), FALSE },
  { "SidePane", NULL, "Side _Pane", "F9", NULL, G_CALLBACK(on_sidepane_toggled), TRUE }
};

/* Radio items */
static const GtkRadioActionEntry radio_entries[] = {
};

static const char *ui_description =
		"<ui>"
		"  <menubar name='MainMenu'>"
		"    <menu action='FileMenu'>"
		"      <menuitem action='Open'/>"
		"      <menuitem action='NewTab'/>"
		"      <menuitem action='CloseTab'/>"
		"      <separator/>"
		"      <menuitem action='Exit'/>"
		"    </menu>"
		"    <menu action='EditMenu'>"
		"      <menuitem action='Copy'/>"
		"      <separator/>"
		"      <menuitem action='Preferences'/>"
		"    </menu>"
		"    <menu action='ViewMenu'>"
		"      <menuitem action='Home'/>"
		"      <menuitem action='Back'/>"
		"      <menuitem action='Forward'/>"
		"      <separator/>"
		"      <menuitem action='FullScreen'/>"
		"      <menuitem action='SidePane'/>"
		"    </menu>"
		"    <menu action='HelpMenu'>"
		"      <menuitem action='About'/>"
		"    </menu>"
		"  </menubar>"
		"	<toolbar name='toolbar'>"
		"		<toolitem action='Open'/>"
		"		<separator/>"
		"		<toolitem action='SidePane' name='sidepane'/>"
		"		<toolitem action='Back'/>"
		"		<toolitem action='Forward'/>"
		"		<toolitem action='Home'/>"
		"		<toolitem action='ZoomIn'/>"
		"		<toolitem action='ZoomReset'/>"
		"		<toolitem action='ZoomOut'/>"
		"		<toolitem action='Preferences'/>"
		"		<toolitem action='About'/>"
		"	</toolbar>"
                " <popup name='HtmlContextLink'>"
                "   <menuitem action='OpenLinkInNewTab' name='OpenLinkInNewTab'/>"
                "   <menuitem action='CopyLinkLocation'/>"
                " </popup>"
                " <popup name='HtmlContextNormal'>"
                "   <menuitem action='Back'/>"
                "   <menuitem action='Forward'/>"
                "   <menuitem action='Copy'/>"
                "   <menuitem action='SelectAll'/>"
                "   <menuitem action='CopyPageLocation'/>"
                " </popup>"
		"</ui>";


G_DEFINE_TYPE (ChmSee, chmsee, GTK_TYPE_WINDOW);

static void
chmsee_class_init(ChmSeeClass *klass)
{
	g_type_class_add_private(klass, sizeof(ChmSeePrivate));
	G_OBJECT_CLASS(klass)->finalize = chmsee_finalize;
	G_OBJECT_CLASS(klass)->dispose = chmsee_dispose;
	GTK_WIDGET_CLASS(klass)->drag_data_received = chmsee_drag_data_received;
}

static void
chmsee_init(ChmSee* self)
{
	self->priv = CHMSEE_GET_PRIVATE(self);
	selfp->home = g_build_filename(g_get_home_dir(), ".chmsee", NULL);

	g_debug("chmsee home = %s", selfp->home);

	if (!g_file_test(selfp->home, G_FILE_TEST_IS_DIR))
		mkdir(selfp->home, 0777);

	selfp->cache_dir = g_build_filename(selfp->home, "bookshelf", NULL);

	if (!g_file_test(selfp->cache_dir, G_FILE_TEST_IS_DIR))
		mkdir(selfp->cache_dir, 0777);

	selfp->lang = 0;
	selfp->last_dir = g_strdup(g_get_home_dir());
	selfp->context_menu_link = NULL;

	selfp->uiIndex = NULL;
	selfp->book = NULL;
	selfp->html_notebook = NULL;
	selfp->pos_x = -100;
	selfp->pos_y = -100;
	selfp->width = 0;
	selfp->height = 0;
	selfp->hpaned_position = -1;
	selfp->has_toc = FALSE;
	selfp->has_index = FALSE;
	selfp->fullscreen = FALSE;
	selfp->expect_fullscreen = FALSE;
	selfp->state = CHMSEE_STATE_INIT;

	gtk_widget_add_events(GTK_WIDGET(self),
			GDK_STRUCTURE_MASK | GDK_BUTTON_PRESS_MASK );

	g_signal_connect(G_OBJECT (self),
			"key-press-event",
			G_CALLBACK (on_keypress_event),
			self);
	g_signal_connect(G_OBJECT(self),
			"scroll-event",
			G_CALLBACK(on_scroll_event),
			NULL);
	g_signal_connect(G_OBJECT(self),
			"map",
			G_CALLBACK(on_map),
			NULL);
	g_signal_connect(G_OBJECT(self),
			"window-state-event",
			G_CALLBACK(on_window_state_event),
			NULL);
	gtk_drag_dest_set (GTK_WIDGET (self),
			GTK_DEST_DEFAULT_ALL,
			view_drop_targets,
			G_N_ELEMENTS (view_drop_targets),
			GDK_ACTION_COPY);
    /* Quit event handle */
    g_signal_connect(G_OBJECT (self),
                     "delete_event",
                     G_CALLBACK (delete_cb),
                     self);
    g_signal_connect(G_OBJECT (self),
                     "destroy",
                     G_CALLBACK (destroy_cb),
                     self);

    /* Widget size changed event handle */
    g_signal_connect(G_OBJECT (self),
                     "configure-event",
                     G_CALLBACK (on_configure_event),
                     self);

    /* Init gecko */
    chmsee_html_init_system();
    chmsee_html_set_default_lang(selfp->lang);

    populate_window(self);
    chmsee_load_config(self);
    if (selfp->pos_x >= 0 && selfp->pos_y >= 0)
            gtk_window_move(GTK_WINDOW (self), selfp->pos_x, selfp->pos_y);

    if (selfp->width > 0 && selfp->height > 0)
            gtk_window_resize(GTK_WINDOW (self), selfp->width, selfp->height);
    else
            gtk_window_resize(GTK_WINDOW (self), 800, 600);

    gtk_window_set_title(GTK_WINDOW (self), "ChmSee");
    gtk_window_set_icon_from_file(GTK_WINDOW (self), get_resource_path("chmsee-icon.png"), NULL);

}

static void
chmsee_finalize(GObject *object)
{
	ChmSee* self = CHMSEE(object);

	if(selfp->home) {
		g_free(selfp->home);
		selfp->home = NULL;
	}

	if(selfp->cache_dir) {
		g_free(selfp->cache_dir);
		selfp->cache_dir = NULL;
	}

	if(selfp->last_dir) {
		g_free(selfp->last_dir);
		selfp->last_dir = NULL;
	}

	g_free(selfp->context_menu_link);
	selfp->context_menu_link = NULL;
	G_OBJECT_CLASS (chmsee_parent_class)->finalize (object);
}

static void chmsee_dispose(GObject* gobject)
{
	ChmSee* self = CHMSEE(gobject);

	if(selfp->book) {
		g_object_unref(selfp->book);
		selfp->book = NULL;
	}

	if(selfp->action_group) {
		g_object_unref(selfp->action_group);
		selfp->action_group = NULL;
	}

	if(selfp->ui_manager) {
		g_object_unref(selfp->ui_manager);
		selfp->ui_manager = NULL;
	}

	G_OBJECT_CLASS(chmsee_parent_class)->dispose(gobject);
}


/* callbacks */

static gboolean
delete_cb(GtkWidget *widget, GdkEvent *event, ChmSee *chmsee)
{
        g_message("window delete");
        return FALSE;
}

static void
destroy_cb(GtkWidget *widget, ChmSee *chmsee)
{
        chmsee_quit(chmsee);
}

static gboolean
on_configure_event(GtkWidget *widget, GdkEventConfigure *event, ChmSee *self)
{
        if (selfp->html_notebook != NULL
            && (event->width != selfp->width || event->height != selfp->height))
                reload_current_page(self);

        if(!selfp->fullscreen) {
          selfp->width = event->width;
          selfp->height = event->height;
          selfp->pos_x = event->x;
          selfp->pos_y = event->y;
        }

        return FALSE;
}

static gboolean
on_keypress_event_when_fullscreen(GtkWidget *widget, GdkEventKey *event, ChmSee *self) {
	switch(event->keyval) {
	case GDK_Escape:
	case GDK_F11:
        /* chmsee_set_fullscreen(self, FALSE);
            return TRUE; */
		break;
	case GDK_F9:
		set_sidepane_state(self,
				!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(get_widget(self, "menu_sidepane"))));
		return TRUE;
		break;
	case GDK_Left:
		if(event->state & GDK_MOD1_MASK) {
			on_back(NULL, self);
			return TRUE;
		}
		break;
	case GDK_Right:
		if(event->state & GDK_MOD1_MASK) {
			on_forward(NULL, self);
			return TRUE;
		}
		break;
	case GDK_minus:
		if(event->state & GDK_CONTROL_MASK) {
			on_zoom_out(NULL, self);
			return TRUE;
		}
		break;
	case GDK_plus:
		if(event->state & GDK_CONTROL_MASK) {
			on_zoom_in(NULL, self);
			return TRUE;
		}
		break;
	default:
		break;
	}
	return FALSE;
}

static gboolean
on_keypress_event_when_unfullscreen(GtkWidget *widget, GdkEventKey *event, ChmSee *self) {
	switch(event->keyval) {
	case GDK_Escape:
		gtk_window_iconify(GTK_WINDOW (self));
		return TRUE;
		break;
	default:
		break;
	}
	return FALSE;
}

static gboolean
on_keypress_event(GtkWidget *widget, GdkEventKey *event, ChmSee *self)
{
	g_debug("enter on_keypress_event with event->keyval = %d and event->state = %d", event->keyval, event->state);
	if(selfp->fullscreen) {
		return on_keypress_event_when_fullscreen(widget, event, self);
	} else {
		return on_keypress_event_when_unfullscreen(widget, event, self);
	}
}

static void
open_response_cb(GtkWidget *widget, gint response_id, ChmSee *chmsee)
{
        gchar *filename = NULL;

        if (response_id == GTK_RESPONSE_OK)
                filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (widget));

        gtk_widget_destroy(widget);

        if (filename != NULL)
                chmsee_open_file(chmsee, filename);

        g_free(filename);
}

static void
booktree_link_selected_cb(GObject *ignored, Link *link, ChmSee *self)
{
        ChmseeIhtml* html;

        g_debug("booktree link selected: %s", link->uri);
        if (!g_ascii_strcasecmp(CHMSEE_NO_LINK, link->uri))
                return;

        html = get_active_html(self);

        g_signal_handlers_block_by_func(html, html_open_uri_cb, self);

        chmsee_ihtml_open_uri(html, g_build_filename(
                        chmsee_ichmfile_get_dir(selfp->book), link->uri, NULL));

        g_signal_handlers_unblock_by_func(html, html_open_uri_cb, self);

        check_history(self, html);
}

static void
bookmarks_link_selected_cb(GObject *ignored, Link *link, ChmSee *chmsee)
{
  chmsee_ihtml_open_uri(get_active_html(chmsee), link->uri);
  check_history(chmsee, get_active_html(chmsee));
}

static void
control_switch_page_cb(GtkNotebook *notebook, GtkNotebookPage *page, guint new_page_num, ChmSee *chmsee)
{
        g_debug("switch page : current page = %d", gtk_notebook_get_current_page(notebook));
}

static void
html_switch_page_cb(GtkNotebook *notebook, GtkNotebookPage *page, guint new_page_num, ChmSee *self)
{
  GtkWidget *new_page;

  new_page = gtk_notebook_get_nth_page(notebook, new_page_num);

  if (new_page) {
    ChmseeIhtml* new_html;
    const gchar* title;
    const gchar* location;

    new_html = g_object_get_data(G_OBJECT (new_page), "html");

    update_tab_title(self, new_html);

    title = chmsee_ihtml_get_title(new_html);
    location = chmsee_ihtml_get_location(new_html);

    if (location != NULL && strlen(location)) {
      if (strlen(title)) {
        ui_bookmarks_set_current_link(UIBOOKMARKS (selfp->bookmark_tree), title, location);
      } else {
        const gchar *book_title;

        book_title = booktree_get_selected_book_title(BOOKTREE (selfp->booktree));
        ui_bookmarks_set_current_link(UIBOOKMARKS (selfp->bookmark_tree), book_title, location);
      }

      /* Sync the book tree. */
      if (selfp->has_toc)
        booktree_select_uri(BOOKTREE (selfp->booktree), location);
    }

    check_history(self, new_html);
  } else {
    gtk_window_set_title(GTK_WINDOW (self), "ChmSee");
    check_history(self, NULL);
  }
}

static void
html_location_changed_cb(ChmseeIhtml *html, const gchar *location, ChmSee *chmsee)
{
        g_debug("html location changed cb: %s", location);

        if (html == get_active_html(chmsee))
                check_history(chmsee, html);
}

static gboolean
html_open_uri_cb(ChmseeIhtml* html, const gchar *uri, ChmSee *self)
{
	g_debug("enter html_open_uri_cb with uri = %s", uri);
  static const char* prefix = "file://";
  static int prefix_len = 7;

  if(g_str_has_prefix(uri, prefix)) {
    /* FIXME: can't disable the DND function of GtkMozEmbed */
    if(g_str_has_suffix(uri, ".chm")
       || g_str_has_suffix(uri, ".CHM")) {
      chmsee_open_uri(self, uri);
    }

    if(g_access(uri+prefix_len, R_OK) < 0) {
    	g_debug("%s:%d:html_open_uri_cb:%s does not exist", __FILE__, __LINE__, uri+prefix_len);
      gchar* newfname = correct_filename(uri+prefix_len);
      if(newfname) {
        g_message(_("URI redirect: \"%s\" -> \"%s\""), uri, newfname);
        chmsee_ihtml_open_uri(html, newfname);
        g_free(newfname);
        return TRUE;
      }

      if(selfp->state == CHMSEE_STATE_LOADING) {
    	  return TRUE;
      }
    }
  }

  if ((html == get_active_html(self)) && selfp->has_toc)
    booktree_select_uri(BOOKTREE (selfp->booktree), uri);

  return FALSE;
}

static void
html_title_changed_cb(ChmseeIhtml *html, const gchar *title, ChmSee *self)
{
        const gchar *location;

        g_debug("html title changed cb %s", title);

        update_tab_title(self, get_active_html(self));

        location = chmsee_ihtml_get_location(html);

        if (location != NULL && strlen(location)) {
                if (strlen(title))
                        ui_bookmarks_set_current_link(UIBOOKMARKS (selfp->bookmark_tree), title, location);
                else {
                        const gchar *book_title;

                        book_title = booktree_get_selected_book_title(BOOKTREE (selfp->booktree));
                        ui_bookmarks_set_current_link(UIBOOKMARKS (selfp->bookmark_tree), book_title, location);
                }
        }
}

/* Popup html context menu */
static void
html_context_normal_cb(ChmseeIhtml *html, ChmSee *self)
{
  g_message("html context-normal event");
  gtk_menu_popup(GTK_MENU(gtk_ui_manager_get_widget(selfp->ui_manager, "/HtmlContextNormal")),
                 NULL, NULL, NULL, NULL, 0, GDK_CURRENT_TIME);
}

/* Popup html context menu when mouse over hyper link */
static void
html_context_link_cb(ChmseeIhtml *html, const gchar *link, ChmSee* self)
{
	g_debug("html context-link event: %s", link);
	chmsee_set_context_menu_link(self, link);
	gtk_action_set_sensitive(gtk_action_group_get_action(selfp->action_group, "OpenLinkInNewTab"),
			g_str_has_prefix(selfp->context_menu_link, "file://"));

	gtk_menu_popup(GTK_MENU(gtk_ui_manager_get_widget(selfp->ui_manager, "/HtmlContextLink")),
			NULL, NULL, NULL, NULL, 0, GDK_CURRENT_TIME);

}

static void
html_open_new_tab_cb(ChmseeIhtml *html, const gchar *location, ChmSee *chmsee)
{
        g_debug("html open new tab callback: %s", location);

        new_tab(chmsee, location);
}

static void
html_link_message_cb(ChmseeIhtml *html, const gchar *url, ChmSee *chmsee)
{
        update_status_bar(chmsee, url);
}

/* Toolbar button events */

static void
on_open(GtkWidget *widget, ChmSee *self)
{
        GladeXML *glade;
        GtkWidget *dialog;
        GtkFileFilter *filter;

        /* create openfile dialog */
        glade = glade_xml_new(get_resource_path(GLADE_FILE), "openfile_dialog", NULL);
        dialog = glade_xml_get_widget(glade, "openfile_dialog");

        g_signal_connect(G_OBJECT (dialog),
                         "response",
                         G_CALLBACK (open_response_cb),
                         self);

        /* File list fiter */
        filter = gtk_file_filter_new();
        gtk_file_filter_set_name(filter, _("CHM Files"));
        gtk_file_filter_add_pattern(filter, "*.[cC][hH][mM]");
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER (dialog), filter);

        filter = gtk_file_filter_new();
        gtk_file_filter_set_name(filter, _("All Files"));
        gtk_file_filter_add_pattern(filter, "*");
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER (dialog), filter);

        /* Previous opened folder */
        if (selfp->last_dir) {
                gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER (dialog), selfp->last_dir);
        }

	g_object_unref(glade);
}

static void
on_close_tab(GtkWidget *widget, ChmSee *self)
{
        gint num_pages, number, i;
        GtkWidget *tab_label, *page;

        number = -1;
        num_pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK (selfp->html_notebook));

        if (num_pages == 1) {
                chmsee_quit(self);

                return;
        }

        for (i = 0; i < num_pages; i++) {
                GList *children, *l;

                g_debug("page %d", i);
                page = gtk_notebook_get_nth_page(GTK_NOTEBOOK (selfp->html_notebook), i);

                tab_label = gtk_notebook_get_tab_label(GTK_NOTEBOOK (selfp->html_notebook), page);
                g_message("tab_label");
                children = gtk_container_get_children(GTK_CONTAINER (tab_label));

                for (l = children; l; l = l->next) {
                        if (widget == l->data) {
                                g_debug("found tab on page %d", i);
                                number = i;
                                break;
                        }
                }

                if (number >= 0) {
                        gtk_notebook_remove_page(GTK_NOTEBOOK (selfp->html_notebook), number);

                        break;
                }
        }
}

static void
on_copy(GtkWidget *widget, ChmSee *self)
{
        g_message("On Copy");

        g_return_if_fail(GTK_IS_NOTEBOOK (selfp->html_notebook));

        chmsee_ihtml_copy_selection(get_active_html(self));
}

static void
on_copy_page_location(GtkWidget* widget, ChmSee* chmsee) {
  ChmseeIhtml* html = get_active_html(chmsee);
  const gchar* location = chmsee_ihtml_get_location(html);
  if(!location) return;

  gtk_clipboard_set_text(
    gtk_clipboard_get(GDK_SELECTION_PRIMARY),
    location,
    -1);
  gtk_clipboard_set_text(
    gtk_clipboard_get(GDK_SELECTION_CLIPBOARD),
    location,
    -1);
}

static void
on_select_all(GtkWidget *widget, ChmSee *self)
{
        ChmseeIhtml *html;

        g_message("On Select All");

        g_return_if_fail(GTK_IS_NOTEBOOK (selfp->html_notebook));

        html = get_active_html(self);
        chmsee_ihtml_select_all(html);
}

static void
on_setup(GtkWidget *widget, ChmSee *chmsee)
{
        setup_window_new(chmsee);
}

static void
on_back(GtkWidget *widget, ChmSee *chmsee)
{
  chmsee_ihtml_go_back(get_active_html(chmsee));
}

static void
on_forward(GtkWidget *widget, ChmSee *chmsee)
{
  chmsee_ihtml_go_forward(get_active_html(chmsee));
}

static void
on_home(GtkWidget *widget, ChmSee *self)
{
  if (chmsee_ichmfile_get_home(selfp->book) != NULL) {
    open_homepage(self);
  }
}

static void
on_zoom_in(GtkWidget *widget, ChmSee *self)
{
	ChmseeIhtml* html = get_active_html(self);
	if(html != NULL) {
		chmsee_ihtml_increase_size(html);
	}
}

static void
on_zoom_reset(GtkWidget *widget, ChmSee *chmsee)
{
  chmsee_ihtml_reset_size(get_active_html(chmsee));
}

static void
on_zoom_out(GtkWidget *widget, ChmSee *self)
{
	ChmseeIhtml* html = get_active_html(self);
	if(html != NULL) {
		chmsee_ihtml_decrease_size(html);
	}
}

static void
about_response_cb(GtkDialog *dialog, gint response_id, gpointer user_data)
{
        if (response_id == GTK_RESPONSE_CANCEL)
                gtk_widget_destroy(GTK_WIDGET (dialog));
}

static void
on_about(GtkWidget *widget)
{
        GladeXML *glade;
        GtkWidget *dialog;

        glade = glade_xml_new(get_resource_path(GLADE_FILE), "about_dialog", NULL);
        dialog = glade_xml_get_widget(glade, "about_dialog");

        g_signal_connect(G_OBJECT (dialog),
                         "response",
                         G_CALLBACK (about_response_cb),
                         NULL);

        gtk_about_dialog_set_version(GTK_ABOUT_DIALOG (dialog), PACKAGE_VERSION);

	g_object_unref(glade);
}

static void
on_open_new_tab(GtkWidget *widget, ChmSee *self)
{
        ChmseeIhtml *html;
        const gchar *location;

        g_message("Open new tab");

        g_return_if_fail(GTK_IS_NOTEBOOK (selfp->html_notebook));

        html = get_active_html(self);
        location = chmsee_ihtml_get_location(html);

        if (location != NULL) {
          new_tab(self, location);
        }
}

static void
on_close_current_tab(GtkWidget *widget, ChmSee *self)
{
        g_return_if_fail(GTK_IS_NOTEBOOK (selfp->html_notebook));

        if (gtk_notebook_get_n_pages(GTK_NOTEBOOK (selfp->html_notebook)) == 1)
                return chmsee_quit(self);

        gint page_num;

        page_num = gtk_notebook_get_current_page(GTK_NOTEBOOK (selfp->html_notebook));

        if (page_num >= 0)
                gtk_notebook_remove_page(GTK_NOTEBOOK (selfp->html_notebook), page_num);
}

static void
on_context_new_tab(GtkWidget *widget, ChmSee *self)
{
	g_debug("On context open new tab: %s", selfp->context_menu_link);

	if (selfp->context_menu_link != NULL) {
		new_tab(self, selfp->context_menu_link);
	}
}

static void
on_context_copy_link(GtkWidget *widget, ChmSee *self)
{
	g_debug("On context copy link: %s", selfp->context_menu_link);

	if (selfp->context_menu_link != NULL) {
		gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY),
				selfp->context_menu_link, -1);
		gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD),
				selfp->context_menu_link, -1);
	}
}


/* internal functions */

static void
chmsee_quit(ChmSee *self)
{
  if (selfp->book) {
    close_current_book(self);
  }

  chmsee_save_config(self);

  if(get_active_html(self)) {
    chmsee_ihtml_shutdown(get_active_html(self));
  }

  gtk_main_quit();
}

static GtkWidget *
get_widget(ChmSee *chmsee, gchar *widget_name)
{
        GladeXML *glade;
        GtkWidget *widget;

        glade = g_object_get_data(G_OBJECT (chmsee), "glade");

        widget = GTK_WIDGET (glade_xml_get_widget(glade, widget_name));

        return widget;
}

static void
populate_window(ChmSee *self)
{
	GtkWidget* vbox = gtk_vbox_new(FALSE, 0);

        GladeXML *glade;

        glade = glade_xml_new(get_resource_path(GLADE_FILE), "main_vbox", NULL);

        if (glade == NULL) {
                g_error("Cannot find glade file!");
                exit(1);
        }

        g_object_set_data(G_OBJECT (self), "glade", glade);

        GtkWidget *main_vbox;
        main_vbox = get_widget(self, "main_vbox");
        gtk_container_add(GTK_CONTAINER (self), vbox);

        GtkActionGroup* action_group = gtk_action_group_new ("MenuActions");
        selfp->action_group = action_group;
        gtk_action_group_add_actions (action_group, entries, G_N_ELEMENTS (entries), self);
        gtk_action_group_add_toggle_actions (action_group, toggle_entries, G_N_ELEMENTS (toggle_entries), self);

        gtk_action_set_sensitive(gtk_action_group_get_action(action_group, "NewTab"), FALSE);
        gtk_action_set_sensitive(gtk_action_group_get_action(action_group, "CloseTab"), FALSE);
        gtk_action_set_sensitive(gtk_action_group_get_action(action_group, "Home"), FALSE);
        gtk_action_set_sensitive(gtk_action_group_get_action(action_group, "Back"), FALSE);
        gtk_action_set_sensitive(gtk_action_group_get_action(action_group, "Forward"), FALSE);
        gtk_action_set_sensitive(gtk_action_group_get_action(action_group, "SidePane"), FALSE);
        gtk_action_set_sensitive(gtk_action_group_get_action(action_group, "ZoomIn"), FALSE);
        gtk_action_set_sensitive(gtk_action_group_get_action(action_group, "ZoomOut"), FALSE);
        gtk_action_set_sensitive(gtk_action_group_get_action(action_group, "ZoomReset"), FALSE);

        GtkUIManager* ui_manager = gtk_ui_manager_new ();
        selfp->ui_manager = ui_manager;
        gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);

        GtkAccelGroup* accel_group = gtk_ui_manager_get_accel_group (ui_manager);
        gtk_window_add_accel_group (GTK_WINDOW (self), accel_group);

        GError* error = NULL;
        if (!gtk_ui_manager_add_ui_from_string (ui_manager, ui_description, -1, &error))
          {
            g_message ("building menus failed: %s", error->message);
            g_error_free (error);
            exit (EXIT_FAILURE);
          }

        GtkWidget* menubar = gtk_handle_box_new();
        selfp->menubar = menubar;
        gtk_container_add(GTK_CONTAINER(menubar), gtk_ui_manager_get_widget (ui_manager, "/MainMenu"));
        gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, FALSE, 0);

        GtkWidget* toolbar = gtk_handle_box_new();
        selfp->toolbar = toolbar;
        gtk_container_add(GTK_CONTAINER(toolbar), gtk_ui_manager_get_widget(ui_manager, "/toolbar"));
        gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

        gtk_tool_button_set_icon_widget(
        		GTK_TOOL_BUTTON(gtk_ui_manager_get_widget(ui_manager, "/toolbar/sidepane")),
        		gtk_image_new_from_file(get_resource_path("show-pane.png")));

        gtk_box_pack_start (GTK_BOX (vbox), main_vbox, TRUE, TRUE, 0);
        gtk_widget_show_all(vbox);

        accel_group = g_object_new(GTK_TYPE_ACCEL_GROUP, NULL);
        gtk_window_add_accel_group(GTK_WINDOW (self), accel_group);

        GtkWidget *control_vbox;

        control_vbox = get_widget(self, "control_vbox");
        gtk_widget_hide(control_vbox);

        /* status bar */
        selfp->statusbar = glade_xml_get_widget(glade, "statusbar");
        selfp->scid_default = gtk_statusbar_get_context_id(GTK_STATUSBAR (selfp->statusbar),
                                                            "default");
        update_status_bar(self, _("Ready!"));
}

void
chmsee_set_model(ChmSee* self, ChmseeIchmfile *book)
{
	GNode *link_tree;
	GList *bookmarks_list;

	GtkWidget *booktree_sw;
	GtkWidget *control_vbox;

	g_debug("display book");
	selfp->state = CHMSEE_STATE_LOADING;

	/* Close currently opened book */
	if (selfp->book)
		close_current_book(self);

	selfp->book = book;

	control_vbox = get_widget(self, "control_vbox");

	/* Book contents TreeView widget */
	selfp->control_notebook = gtk_notebook_new();

	gtk_box_pack_start(GTK_BOX (control_vbox),
			GTK_WIDGET (selfp->control_notebook),
			TRUE,
			TRUE,
			2);
	g_signal_connect(G_OBJECT (selfp->control_notebook),
			"switch-page",
			G_CALLBACK (control_switch_page_cb),
			self);

	/* TOC */
	if (chmsee_ichmfile_get_link_tree(selfp->book) != NULL) {
		link_tree = chmsee_ichmfile_get_link_tree(selfp->book);

		booktree_sw = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (booktree_sw),
				GTK_POLICY_NEVER,
				GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW (booktree_sw),
				GTK_SHADOW_IN);
		gtk_container_set_border_width(GTK_CONTAINER (booktree_sw), 2);

		selfp->booktree = GTK_WIDGET(g_object_ref_sink(booktree_new(link_tree)));
		g_signal_connect_swapped(selfp->booktree,
				"scroll-event",
				G_CALLBACK(on_scroll_event),
				self);

		gtk_container_add(GTK_CONTAINER (booktree_sw), selfp->booktree);
		gtk_notebook_append_page(GTK_NOTEBOOK (selfp->control_notebook),
				booktree_sw,
				gtk_label_new(_("Topics")));

		g_signal_connect(G_OBJECT (selfp->booktree),
				"link-selected",
				G_CALLBACK (booktree_link_selected_cb),
				self);

		g_debug("chmsee has toc");
		selfp->has_toc = TRUE;
	}

	/* Index */
	gtk_notebook_append_page(GTK_NOTEBOOK (selfp->control_notebook),
			chmsee_new_index_page(self),
			gtk_label_new(_("Index")));
	chmsee_refresh_index(self);

	/* Bookmarks */
	bookmarks_list = chmsee_ichmfile_get_bookmarks_list(selfp->book);
	selfp->bookmark_tree = GTK_WIDGET (ui_bookmarks_new(bookmarks_list));

	gtk_notebook_append_page(GTK_NOTEBOOK (selfp->control_notebook),
			selfp->bookmark_tree,
			gtk_label_new (_("Bookmarks")));

	g_signal_connect(G_OBJECT (selfp->bookmark_tree),
			"link-selected",
			G_CALLBACK (bookmarks_link_selected_cb),
			self);

	GtkWidget *hpaned;

	hpaned = get_widget(self, "hpaned1");

	/* HTML tabs notebook */
	selfp->html_notebook = gtk_notebook_new();
	gtk_paned_add2 (GTK_PANED (hpaned), selfp->html_notebook);

	g_signal_connect(G_OBJECT (selfp->html_notebook),
			"switch-page",
			G_CALLBACK (html_switch_page_cb),
			self);

	gtk_widget_show_all(hpaned);
	new_tab(self, NULL);

	gtk_notebook_set_current_page(GTK_NOTEBOOK (selfp->control_notebook),
			g_list_length(bookmarks_list) && selfp->has_toc ? 1 : 0);

    gtk_action_set_sensitive(gtk_action_group_get_action(selfp->action_group, "SidePane"), TRUE);
    gtk_action_set_sensitive(gtk_action_group_get_action(selfp->action_group, "ZoomIn"), TRUE);
    gtk_action_set_sensitive(gtk_action_group_get_action(selfp->action_group, "ZoomReset"), TRUE);
    gtk_action_set_sensitive(gtk_action_group_get_action(selfp->action_group, "ZoomOut"), TRUE);
	g_object_set(gtk_action_group_get_action(selfp->action_group, "SidePane"), "active", TRUE, NULL);


	/* Window title */
	gchar *window_title;

	if (chmsee_ichmfile_get_title(selfp->book) != NULL
			&& g_ascii_strcasecmp(chmsee_ichmfile_get_title(selfp->book), "(null)") != 0 ) {
		window_title = g_strdup_printf("%s - ChmSee", chmsee_ichmfile_get_title(selfp->book));
	} else {
		window_title = g_strdup_printf("%s - ChmSee",
				g_path_get_basename(chmsee_ichmfile_get_filename(book)));
	}

	gtk_window_set_title(GTK_WINDOW (self), window_title);
	g_free(window_title);

	chmsee_ihtml_set_variable_font(get_active_html(self),
			chmsee_ichmfile_get_variable_font(selfp->book));
	chmsee_ihtml_set_fixed_font(get_active_html(self),
			chmsee_ichmfile_get_fixed_font(selfp->book));

	if (chmsee_ichmfile_get_home(selfp->book)) {
		open_homepage(self);

	    gtk_action_set_sensitive(gtk_action_group_get_action(selfp->action_group, "NewTab"), TRUE);
	    gtk_action_set_sensitive(gtk_action_group_get_action(selfp->action_group, "CloseTab"), TRUE);
	    gtk_action_set_sensitive(gtk_action_group_get_action(selfp->action_group, "Home"), TRUE);
	}
	selfp->state = CHMSEE_STATE_NORMAL;

    selfp->last_dir = g_strdup_printf("%s", g_path_get_dirname(
    		chmsee_ichmfile_get_filename(book)));
}

static void
close_current_book(ChmSee *self)
{
  gchar* bookmark_fname = g_build_filename(chmsee_ichmfile_get_dir(selfp->book), CHMSEE_BOOKMARK_FILE, NULL);
  bookmarks_save(ui_bookmarks_get_list(UIBOOKMARKS (selfp->bookmark_tree)), bookmark_fname);
  g_free(bookmark_fname);
  g_object_unref(selfp->book);
  gtk_widget_destroy(GTK_WIDGET (selfp->control_notebook));
  gtk_widget_destroy(GTK_WIDGET (selfp->html_notebook));

  selfp->book = NULL;
  selfp->control_notebook = NULL;
  selfp->html_notebook = NULL;
  selfp->state = CHMSEE_STATE_INIT;
}

static GtkWidget*
new_tab_content(ChmSee *chmsee, const gchar *str)
{
        GtkWidget *widget;
        GtkWidget *label;
        GtkWidget *close_button, *close_image;

        widget = gtk_hbox_new(FALSE, 3);

        label = gtk_label_new(str);
        gtk_label_set_ellipsize(GTK_LABEL (label), PANGO_ELLIPSIZE_END);
        gtk_label_set_single_line_mode(GTK_LABEL (label), TRUE);
        gtk_misc_set_alignment(GTK_MISC (label), 0.0, 0.5);
        gtk_misc_set_padding(GTK_MISC (label), 0, 0);
	gtk_box_pack_start(GTK_BOX (widget), label, TRUE, TRUE, 0);
        g_object_set_data(G_OBJECT (widget), "label", label);

        close_button = gtk_button_new();
	gtk_button_set_relief(GTK_BUTTON(close_button), GTK_RELIEF_NONE);
	close_image = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
	gtk_widget_show(close_image);
	gtk_container_add(GTK_CONTAINER (close_button), close_image);
	g_signal_connect(G_OBJECT (close_button),
                         "clicked",
	                 G_CALLBACK (on_close_tab),
                         chmsee);

        gtk_box_pack_start(GTK_BOX (widget), close_button, FALSE, FALSE, 0);

	gtk_widget_show_all(widget);

        return widget;
}

static void
new_tab(ChmSee *self, const gchar *location)
{
        ChmseeIhtml  *html;
        GtkWidget    *frame;
        GtkWidget    *view;
        GtkWidget    *tab_content;
        gint          num;

        g_debug("new_tab : %s", location);

        /* Ignore external link */
        if (location != NULL && !g_str_has_prefix(location, "file://"))
                return;

        html = chmsee_html_new();
        g_signal_connect_swapped(chmsee_ihtml_get_widget(html),
        		"dom-mouse-click",
        		G_CALLBACK(on_scroll_event),
        		self);

        view = chmsee_ihtml_get_widget(html);
        gtk_widget_show(view);

        frame = gtk_frame_new(NULL);
        gtk_widget_show(frame);

        gtk_frame_set_shadow_type(GTK_FRAME (frame), GTK_SHADOW_IN);
        gtk_container_set_border_width(GTK_CONTAINER (frame), 2);
        gtk_container_add(GTK_CONTAINER (frame), view);

        g_object_set_data(G_OBJECT (frame), "html", html);

        /* Custom label widget, with a close button */
        tab_content = new_tab_content(self, _("No Title"));

        g_signal_connect(G_OBJECT (html),
                         "title-changed",
                         G_CALLBACK (html_title_changed_cb),
                         self);
        g_signal_connect(G_OBJECT (html),
                         "open-uri",
                         G_CALLBACK (html_open_uri_cb),
                         self);
        g_signal_connect(G_OBJECT (html),
                         "location-changed",
                         G_CALLBACK (html_location_changed_cb),
                         self);
        g_signal_connect(G_OBJECT (html),
                         "context-normal",
                         G_CALLBACK (html_context_normal_cb),
                         self);
        g_signal_connect(G_OBJECT (html),
                         "context-link",
                         G_CALLBACK (html_context_link_cb),
                         self);
        g_signal_connect(G_OBJECT (html),
                         "open-new-tab",
                         G_CALLBACK (html_open_new_tab_cb),
                         self);
        g_signal_connect(G_OBJECT (html),
                         "link-message",
                         G_CALLBACK (html_link_message_cb),
                         self);
        g_signal_connect_swapped(chmsee_ihtml_get_widget(html),
        		"scroll-event",
        		G_CALLBACK(on_scroll_event),
        		self);

        num = gtk_notebook_append_page(GTK_NOTEBOOK (selfp->html_notebook),
                                       frame, tab_content);

        gtk_notebook_set_tab_label_packing(GTK_NOTEBOOK (selfp->html_notebook),
                                           frame,
                                           TRUE, TRUE,
                                           GTK_PACK_START);

        gtk_widget_realize(view);

        if (location != NULL) {
                chmsee_ihtml_open_uri(html, location);

                if (selfp->has_toc)
                        booktree_select_uri(BOOKTREE (selfp->booktree), location);
        } else {
                chmsee_ihtml_clear(html);
        }

        gtk_notebook_set_current_page(GTK_NOTEBOOK (selfp->html_notebook), num);
}

static void
open_homepage(ChmSee *self)
{
        ChmseeIhtml *html;

        html = get_active_html(self);

        /* g_signal_handlers_block_by_func(html, html_open_uri_cb, self); */

        chmsee_ihtml_open_uri(html, g_build_filename(chmsee_ichmfile_get_dir(selfp->book),
                                             chmsee_ichmfile_get_home(selfp->book), NULL));

        /* g_signal_handlers_unblock_by_func(html, html_open_uri_cb, self); */

        if (selfp->has_toc) {
          booktree_select_uri(BOOKTREE (selfp->booktree),
                              chmsee_ichmfile_get_home(selfp->book));
        }

        check_history(self, html);
}

static void
reload_current_page(ChmSee *self)
{
        ChmseeIhtml*html;
        const gchar *location;

        g_message("Reload current page");

        g_return_if_fail(GTK_IS_NOTEBOOK (selfp->html_notebook));

        html = get_active_html(self);
        location = chmsee_ihtml_get_location(html);

        if (location != NULL) {
          chmsee_ihtml_open_uri(html, location);
        }
}

static ChmseeIhtml *
get_active_html(ChmSee *self)
{
        GtkWidget *frame;
        gint page_num;

        if(!selfp->html_notebook) {
          return NULL;
        }
        page_num = gtk_notebook_get_current_page(GTK_NOTEBOOK (selfp->html_notebook));

        if (page_num == -1)
                return NULL;

        frame = gtk_notebook_get_nth_page(GTK_NOTEBOOK (selfp->html_notebook), page_num);

        return g_object_get_data(G_OBJECT (frame), "html");
}

static void
check_history(ChmSee *self, ChmseeIhtml *html)
{
	gboolean back_state, forward_state;

	back_state = chmsee_ihtml_can_go_back(html);
	forward_state = chmsee_ihtml_can_go_forward(html);


	gtk_action_set_sensitive(gtk_action_group_get_action(selfp->action_group, "Back"), back_state);
	gtk_action_set_sensitive(gtk_action_group_get_action(selfp->action_group, "Forward"), forward_state);
}

static void
update_tab_title(ChmSee *self, ChmseeIhtml *html)
{
  const gchar* html_title;
  const gchar* tab_title;
  const gchar* book_title;

        html_title = chmsee_ihtml_get_title(html);

        if (selfp->has_toc)
                book_title = booktree_get_selected_book_title(BOOKTREE (selfp->booktree));
        else
                book_title = "";

        if (book_title && book_title[0] != '\0' &&
            html_title && html_title[0] != '\0' &&
            ncase_compare_utf8_string(book_title, html_title))
                tab_title = g_strdup_printf("%s : %s", book_title, html_title);
        else if (book_title && book_title[0] != '\0')
                tab_title = g_strdup(book_title);
        else if (html_title && html_title[0] != '\0')
                tab_title = g_strdup(html_title);
        else
                tab_title = g_strdup("");

        tab_set_title(self, html, tab_title);
}

static void
tab_set_title(ChmSee *self, ChmseeIhtml *html, const gchar *title)
{
        GtkWidget *view;
        GtkWidget *page;
        GtkWidget *widget, *label;
        gint num_pages, i;

        view = chmsee_ihtml_get_widget(html);

        if (title == NULL || title[0] == '\0')
                title = _("No Title");

        num_pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK (selfp->html_notebook));

        for (i = 0; i < num_pages; i++) {
                page = gtk_notebook_get_nth_page(GTK_NOTEBOOK (selfp->html_notebook), i);

                if (gtk_bin_get_child(GTK_BIN (page)) == view) {
                        widget = gtk_notebook_get_tab_label(GTK_NOTEBOOK (selfp->html_notebook), page);

                        label = g_object_get_data(G_OBJECT (widget), "label");

                        if (label != NULL)
                                gtk_label_set_text(GTK_LABEL (label), title);

                        break;
                }
        }
}

static void
update_status_bar(ChmSee *self, const gchar *message)
{
        gchar *status;

        status = g_strdup_printf(" %s", message);

        gtk_statusbar_pop(GTK_STATUSBAR(selfp->statusbar), selfp->scid_default);
        gtk_statusbar_push(GTK_STATUSBAR(selfp->statusbar), selfp->scid_default, status);

        g_free(status);
}

/* external functions */

ChmSee *
chmsee_new(const gchar* filename)
{
        ChmSee *self;

        self = g_object_new(TYPE_CHMSEE, NULL);

        if(filename != NULL) {
        	chmsee_open_file(self, filename);
        }

        return self;
}

void
chmsee_open_file(ChmSee *self, const gchar *filename)
{
        ChmseeIchmfile* book;

        g_return_if_fail(IS_CHMSEE (self));

        /* Extract chm and get file infomation */
        book = chmsee_chmfile_new(filename);

        if (book) {
                chmsee_set_model(self, book);
        } else {
                /* Popup an error message dialog */
                GtkWidget *msg_dialog;

                msg_dialog = gtk_message_dialog_new(GTK_WINDOW (self),
                                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    GTK_MESSAGE_ERROR,
                                                    GTK_BUTTONS_CLOSE,
                                                    _("Error loading file '%s'"),
                                                    filename);
                gtk_dialog_run(GTK_DIALOG (msg_dialog));
                gtk_widget_destroy(msg_dialog);
        }
}

void
chmsee_drag_data_received (GtkWidget          *widget,
                           GdkDragContext     *context,
                           gint                x,
                           gint                y,
                           GtkSelectionData   *selection_data,
                           guint               info,
                           guint               time)
{
  gchar  **uris;
  gint     i = 0;

  uris = gtk_selection_data_get_uris (selection_data);
  if (!uris) {
    gtk_drag_finish (context, FALSE, FALSE, time);
    return;
  }

  for (i = 0; uris[i]; i++) {
    gchar* uri = uris[i];
    if(g_str_has_prefix(uri, "file://")
       && (g_str_has_suffix(uri, ".chm")
           || g_str_has_suffix(uri, ".CHM"))) {
      chmsee_open_uri(CHMSEE(widget), uri);
      break;
    }
  }

  gtk_drag_finish (context, TRUE, FALSE, time);

  g_strfreev (uris);
}

void chmsee_open_uri(ChmSee *chmsee, const gchar *uri) {
  if(!g_str_has_prefix(uri, "file://")) {
    return;
  }

  gchar* fname = g_uri_unescape_string(uri+7, NULL);
  chmsee_open_file(chmsee, fname);
  g_free(fname);
}

int chmsee_get_hpaned_position(ChmSee* self) {
	gint position;
	g_object_get(G_OBJECT(get_widget(self, "hpaned1")),
			"position", &position,
			NULL
			);
	return position;
}

void chmsee_set_hpaned_position(ChmSee* self, int hpaned_position) {
	selfp->hpaned_position = hpaned_position;
	/*
	g_object_set(G_OBJECT(get_widget(self, "hpaned1")),
			"position", hpaned_position,
			NULL
			);
			*/
}

void on_fullscreen_toggled(GtkWidget* menu, ChmSee* self) {
	g_return_if_fail(IS_CHMSEE(self));
	gboolean active;
	g_object_get(G_OBJECT(menu),
			"active", &active,
			NULL);
	g_debug("enter on_fullscreen_toggled with menu.active = %d", active);
	chmsee_set_fullscreen(self, active);
}

void on_sidepane_toggled(GtkWidget* menu, ChmSee* self) {
	g_return_if_fail(IS_CHMSEE(self));
	gboolean active;
	g_object_get(G_OBJECT(menu),
			"active", &active,
			NULL);
	if(active) {
		show_sidepane(self);
	} else {
		hide_sidepane(self);
	}
}

void set_sidepane_state(ChmSee* self, gboolean state) {
	GtkWidget* icon_widget;

	if(state) {
		gtk_widget_show(get_widget(self, "control_vbox"));
	} else {
		gtk_widget_hide(get_widget(self, "control_vbox"));
	}

    if (state) {
            icon_widget = gtk_image_new_from_file(get_resource_path("hide-pane.png"));
    } else {
            icon_widget = gtk_image_new_from_file(get_resource_path("show-pane.png"));
    }
    gtk_widget_show(icon_widget);
    gtk_tool_button_set_icon_widget(
    		GTK_TOOL_BUTTON(gtk_ui_manager_get_widget(selfp->ui_manager, "/toolbar/sidepane")),
    		icon_widget);
};

void show_sidepane(ChmSee* self) {
	set_sidepane_state(self, TRUE);
}

void hide_sidepane(ChmSee* self) {
	set_sidepane_state(self, FALSE);
}


void on_map(ChmSee* self) {
	if(selfp->hpaned_position >= 0) {
	g_object_set(G_OBJECT(get_widget(self, "hpaned1")),
			"position", selfp->hpaned_position,
			NULL
			);
	}
}


static void on_fullscreen(ChmSee* self) {
	g_debug("enter on_fullscreen");
	selfp->fullscreen = TRUE;
	gtk_widget_hide(selfp->menubar);
	gtk_widget_hide(selfp->toolbar);
	gtk_widget_hide(get_widget(self, "statusbar"));
}

static void on_unfullscreen(ChmSee* self) {
	g_debug("enter on_unfullscreen");
	selfp->fullscreen = FALSE;
	gtk_widget_show(selfp->menubar);
	gtk_widget_show(selfp->toolbar);
	gtk_widget_show(get_widget(self, "statusbar"));
}

gboolean on_window_state_event(ChmSee* self, GdkEventWindowState* event) {
	g_return_val_if_fail(IS_CHMSEE(self), FALSE);
	g_return_val_if_fail(event->type == GDK_WINDOW_STATE, FALSE);

	g_debug("enter on_window_state_event with event->changed_mask = %d and event->new_window_state = %d",
			event->changed_mask,
			event->new_window_state
			);

	if(!(event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN)) {
		return FALSE;
	}

	if(event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN) {
		if(selfp->expect_fullscreen) {
			on_fullscreen(self);
		} else {
			g_warning("expect not fullscreen but got a fullscreen event, restored");
			chmsee_set_fullscreen(self, FALSE);
			return TRUE;
		}
	} else {
		if(!selfp->expect_fullscreen) {
			on_unfullscreen(self);
		} else {
			g_warning("expect fullscreen but got an unfullscreen event, restored");
			chmsee_set_fullscreen(self, TRUE);
			return TRUE;
		}
	}

	return FALSE;
}

static gboolean on_scroll_event(ChmSee* self, GdkEventScroll* event) {
	if(event->direction == GDK_SCROLL_UP && (event->state & GDK_CONTROL_MASK)) {
		on_zoom_in(NULL, self);
		return TRUE;
	} else if(event->direction == GDK_SCROLL_DOWN && (event->state & GDK_CONTROL_MASK)) {
		on_zoom_out(NULL, self);
		return TRUE;
	} else {
		g_debug("event->direction: %d", event->direction);
		g_debug("event->state: %x", event->state);
	}

	return FALSE;
}

const gchar* chmsee_get_cache_dir(ChmSee* self) {
	return selfp->cache_dir;
}

const gchar* chmsee_get_variable_font(ChmSee* self) {
	g_return_val_if_fail(selfp->book, NULL);
	return chmsee_ichmfile_get_variable_font(selfp->book);
}

void chmsee_set_variable_font(ChmSee* self, const gchar* font_name) {
	g_return_if_fail(selfp->book);
    chmsee_ichmfile_set_variable_font(selfp->book, font_name);
}

const gchar* chmsee_get_fixed_font(ChmSee* self) {
	g_return_val_if_fail(selfp->book, NULL);
	return chmsee_ichmfile_get_fixed_font(selfp->book);
}

void chmsee_set_fixed_font(ChmSee* self, const gchar* font_name) {
	g_return_if_fail(selfp->book);
    chmsee_ichmfile_set_fixed_font(selfp->book, font_name);
}

int chmsee_get_lang(ChmSee* self) {
	return selfp->lang;
}
void chmsee_set_lang(ChmSee* self, int lang) {
	selfp->lang = lang;
}

gboolean chmsee_has_book(ChmSee* self) {
	return selfp->book != NULL;
}

void
chmsee_load_config(ChmSee *self)
{
	g_debug("enter chmsee_load_config");
	GList *pairs, *list;
	gchar *path;

	path = g_build_filename(selfp->home, "config", NULL);

	g_debug("config path = %s", path);

	pairs = parse_config_file("config", path);

	for (list = pairs; list; list = list->next) {
		Item *item;

		item = list->data;

		/* Get user prefered language */
		if (strstr(item->id, "LANG")) {
			selfp->lang = atoi(item->value);
			continue;
		}

		/* Get last directory */
		if (strstr(item->id, "LAST_DIR")) {
			selfp->last_dir = g_strdup(item->value);
			continue;
		}

		/* Get window position */
		if (strstr(item->id, "POS_X")) {
			selfp->pos_x = atoi(item->value);
			continue;
		}
		if (strstr(item->id, "POS_Y")) {
			selfp->pos_y = atoi(item->value);
			continue;
		}
		if (strstr(item->id, "WIDTH")) {
			selfp->width = atoi(item->value);
			continue;
		}
		if (strstr(item->id, "HEIGHT")) {
			selfp->height = atoi(item->value);
			continue;
		}
		if(strstr(item->id, "HPANED_POSTION")) {
			chmsee_set_hpaned_position(self, atoi(item->value));
			continue;
		}
                if(strstr(item->id, "FULLSCREEN")) {
                  if(strcmp(item->value, "true") == 0) {
                    chmsee_set_fullscreen(self, TRUE);
                  } else if(strcmp(item->value, "false") == 0) {
                    chmsee_set_fullscreen(self, FALSE);
                  } else {
                    g_warning("%s:%d:unknown value of FULLSCREEN %s", __FILE__, __LINE__, item->value);
                  }
                }
	}

	free_config_list(pairs);
	g_free(path);
}

void
chmsee_save_config(ChmSee *self)
{
        FILE *file;
        gchar *path;

        path = g_build_filename(selfp->home, "config", NULL);

        file = fopen(path, "w");

        if (!file) {
                g_print("Faild to open chmsee config: %s", path);
                return;
        }

        save_option(file, "LANG", g_strdup_printf("%d", selfp->lang));
        save_option(file, "LAST_DIR", selfp->last_dir);
        save_option(file, "POS_X", g_strdup_printf("%d", selfp->pos_x));
        save_option(file, "POS_Y", g_strdup_printf("%d", selfp->pos_y));
        save_option(file, "WIDTH", g_strdup_printf("%d", selfp->width));
        save_option(file, "HEIGHT", g_strdup_printf("%d", selfp->height));
        save_option(file, "HPANED_POSTION", g_strdup_printf("%d", chmsee_get_hpaned_position(self)));
        save_option(file, "FULLSCREEN", selfp->fullscreen ? "true" : "false" );

        fclose(file);
        g_free(path);
}

void chmsee_set_fullscreen(ChmSee* self, gboolean fullscreen) {
	g_debug("enter chmsee_set_fullscreen with fullscreen = %d", fullscreen);
	selfp->expect_fullscreen = fullscreen;

  if(fullscreen) {
	  g_debug("call gtk_window_fullscreen");
    gtk_window_fullscreen(GTK_WINDOW(self));
  } else {
	  g_debug("call gtk_window_unfullscreen");
    gtk_window_unfullscreen(GTK_WINDOW(self));
  }
}

void chmsee_refresh_index(ChmSee* self) {
	ChmIndex* chmIndex = NULL;
	if(selfp->book) {
		chmIndex = chmsee_ichmfile_get_index(selfp->book);
	}
	chmsee_ui_index_set_model(CHMSEE_UI_INDEX(selfp->uiIndex), chmIndex);
	if(chmIndex != NULL) {
		g_debug("chmIndex != NULL");
		gtk_widget_show(selfp->indexPage);
	} else {
		g_debug("chmIndex == NULL");
		gtk_widget_hide(selfp->indexPage);
	}
}

static GtkWidget* chmsee_new_index_page(ChmSee* self) {
	GtkWidget* booktree_sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (booktree_sw),
			GTK_POLICY_NEVER,
			GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW (booktree_sw),
			GTK_SHADOW_IN);
	gtk_container_set_border_width(GTK_CONTAINER (booktree_sw), 2);

	GtkWidget* uiIndex = chmsee_ui_index_new(NULL);
	gtk_container_add(GTK_CONTAINER (booktree_sw), uiIndex);
	g_signal_connect_swapped(uiIndex,
			"link-selected",
			G_CALLBACK (chmsee_on_ui_index_link_selected),
			self);

	selfp->indexPage = booktree_sw;
	selfp->uiIndex = uiIndex;
	return GTK_WIDGET(booktree_sw);
}

void chmsee_on_ui_index_link_selected(ChmSee* self, Link* link) {
	booktree_link_selected_cb(NULL, link, self);
}


gboolean chmsee_jump_index_by_name(ChmSee* self, const gchar* name) {
	g_return_val_if_fail(IS_CHMSEE(self), FALSE);

	gboolean res = chmsee_ui_index_select_link_by_name(
			CHMSEE_UI_INDEX(self->priv->uiIndex),
			name);

	if(res) {
		/* TODO: hard-code page num 1 */
		gtk_notebook_set_current_page(GTK_NOTEBOOK(self->priv->control_notebook), 1);
	}
	return res;
}

static void chmsee_set_context_menu_link(ChmSee* self, const gchar* link) {
	g_free(selfp->context_menu_link);
	selfp->context_menu_link = g_strdup(link);
}

