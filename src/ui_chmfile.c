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
 *  along with ChmseeUiChmfile; see the file COPYING.  If not, write to
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
#include "ui_chmfile.h"

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

struct _ChmseeUiChmfilePrivate {
  GtkWidget* menubar;
  GtkWidget* toolbar;
    GtkWidget       *control_notebook;
    GtkWidget* topic_page;
    GtkWidget       *html_notebook;

    GtkWidget       *booktree;
    GtkWidget       *bookmark_tree;

    GtkWidget* uiIndex; /* the gtktreeview */
    GtkWidget* indexPage; /* the index tab under control_notebook */

    GtkWidget       *statusbar;


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
#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), CHMSEE_TYPE_UI_CHMFILE, ChmseeUiChmfilePrivate))

static void chmsee_ui_chmfile_init(ChmseeUiChmfile *);
static void chmsee_ui_chmfile_finalize(GObject *);
static void chmsee_ui_chmfile_dispose(GObject* self);
static void chmsee_set_context_menu_link(ChmseeUiChmfile* self, const gchar* link);

static void chmsee_refresh_index(ChmseeUiChmfile* self);
static GtkWidget* chmsee_new_index_page(ChmseeUiChmfile* self);
static void chmsee_on_ui_index_link_selected(ChmseeUiChmfile* self, Link* link);

static void booktree_link_selected_cb(GObject *, Link *, ChmseeUiChmfile *);
static void bookmarks_link_selected_cb(GObject *, Link *, ChmseeUiChmfile *);
static void control_switch_page_cb(GtkNotebook *, GtkNotebookPage *, guint , ChmseeUiChmfile *);
static void html_switch_page_cb(GtkNotebook *, GtkNotebookPage *, guint , ChmseeUiChmfile *);
static void html_location_changed_cb(ChmseeIhtml *, const gchar *, ChmseeUiChmfile *);
static gboolean html_open_uri_cb(ChmseeIhtml *, const gchar *, ChmseeUiChmfile *);
static void html_title_changed_cb(ChmseeIhtml *, const gchar *, ChmseeUiChmfile *);
static void html_context_normal_cb(ChmseeIhtml *, ChmseeUiChmfile *);
static void html_context_link_cb(ChmseeIhtml *, const gchar *, ChmseeUiChmfile *);
static void html_open_new_tab_cb(ChmseeIhtml *, const gchar *, ChmseeUiChmfile *);
static void html_link_message_cb(ChmseeIhtml *, const gchar *, ChmseeUiChmfile *);
static void show_sidepane(ChmseeUiChmfile* self);
static void hide_sidepane(ChmseeUiChmfile* self);
static void set_sidepane_state(ChmseeUiChmfile* self, gboolean state);

static void on_close_tab(GtkWidget *, ChmseeUiChmfile *);
static void on_copy(GtkWidget *, ChmseeUiChmfile *);
static void on_copy_page_location(GtkWidget*, ChmseeUiChmfile*);
static void on_select_all(GtkWidget *, ChmseeUiChmfile *);
static void on_back(GtkWidget *, ChmseeUiChmfile *);
static void on_forward(GtkWidget *, ChmseeUiChmfile *);
static void on_home(GtkWidget *, ChmseeUiChmfile *);
static void on_zoom_in(GtkWidget *, ChmseeUiChmfile *);
static void on_zoom_reset(GtkWidget *, ChmseeUiChmfile *);
static void on_zoom_out(GtkWidget *, ChmseeUiChmfile *);
static void on_open_new_tab(GtkWidget *, ChmseeUiChmfile *);
static void on_close_current_tab(GtkWidget *, ChmseeUiChmfile *);
static void on_context_new_tab(GtkWidget *, ChmseeUiChmfile *);
static void on_context_copy_link(GtkWidget *, ChmseeUiChmfile *);
static void on_sidepane_toggled(GtkWidget*, ChmseeUiChmfile* self);
static gboolean on_scroll_event(ChmseeUiChmfile* self, GdkEventScroll* event);

static void chmsee_open_uri(ChmseeUiChmfile *chmsee, const gchar *uri);
static void chmsee_open_file(ChmseeUiChmfile *self, const gchar *filename);
static void chmsee_ui_chmfile_populate_window(ChmseeUiChmfile *);
static void chmsee_ui_chmfile_close_current_book(ChmseeUiChmfile *);
static void new_tab(ChmseeUiChmfile *, const gchar *);
static ChmseeIhtml *get_active_html(ChmseeUiChmfile *);
static void check_history(ChmseeUiChmfile *, ChmseeIhtml *);
static void update_tab_title(ChmseeUiChmfile *, ChmseeIhtml *);
static void tab_set_title(ChmseeUiChmfile *, ChmseeIhtml *, const gchar *);
static void open_homepage(ChmseeUiChmfile *);
static void reload_current_page(ChmseeUiChmfile*);
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

/* Toggle items */
static const GtkToggleActionEntry toggle_entries[] = {
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
		"      <menuitem action='SelectAll'/>"
		"      <menuitem action='Copy'/>"
		"      <separator/>"
		"      <menuitem action='Preferences'/>"
		"    </menu>"
		"    <menu action='ViewMenu'>"
		"      <menuitem action='FullScreen'/>"
		"      <menuitem action='SidePane'/>"
		"      <separator/>"
		"      <menuitem action='Home'/>"
		"      <menuitem action='Back'/>"
		"      <menuitem action='Forward'/>"
		"      <separator/>"
		"      <menuitem action='ZoomIn'/>"
		"      <menuitem action='ZoomReset'/>"
		"      <menuitem action='ZoomOut'/>"
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
		"<accelerator action='OnKeyboardEscape'/>"
		"<accelerator action='OnKeyboardControlEqual'/>"
		"</ui>";


G_DEFINE_TYPE (ChmseeUiChmfile, chmsee_ui_chmfile, GTK_TYPE_HPANED);

static void
chmsee_ui_chmfile_class_init(ChmseeUiChmfileClass *klass)
{
	g_type_class_add_private(klass, sizeof(ChmseeUiChmfilePrivate));
	G_OBJECT_CLASS(klass)->finalize = chmsee_ui_chmfile_finalize;
	G_OBJECT_CLASS(klass)->dispose = chmsee_ui_chmfile_dispose;
}

static void
chmsee_ui_chmfile_init(ChmseeUiChmfile* self)
{
	self->priv = GET_PRIVATE(self);
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


	g_signal_connect(G_OBJECT(self),
			"scroll-event",
			G_CALLBACK(on_scroll_event),
			NULL);
    /* Init gecko */
    chmsee_html_init_system();
    chmsee_html_set_default_lang(selfp->lang);

    chmsee_ui_chmfile_populate_window(self);
}

static void
chmsee_ui_chmfile_finalize(GObject *object)
{
	ChmseeUiChmfile* self = CHMSEE_UI_CHMFILE(object);

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
	G_OBJECT_CLASS (chmsee_ui_chmfile_parent_class)->finalize (object);
}

static void chmsee_ui_chmfile_dispose(GObject* gobject)
{
	ChmseeUiChmfile* self = CHMSEE_UI_CHMFILE(gobject);

	if(selfp->book) {
		g_object_unref(selfp->book);
		selfp->book = NULL;
	}

	G_OBJECT_CLASS(chmsee_ui_chmfile_parent_class)->dispose(gobject);
}

static void
booktree_link_selected_cb(GObject *ignored, Link *link, ChmseeUiChmfile *self)
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
bookmarks_link_selected_cb(GObject *ignored, Link *link, ChmseeUiChmfile *chmsee)
{
  chmsee_ihtml_open_uri(get_active_html(chmsee), link->uri);
  check_history(chmsee, get_active_html(chmsee));
}

static void
control_switch_page_cb(GtkNotebook *notebook, GtkNotebookPage *page, guint new_page_num, ChmseeUiChmfile *chmsee)
{
        g_debug("switch page : current page = %d", gtk_notebook_get_current_page(notebook));
}

static void
html_switch_page_cb(GtkNotebook *notebook, GtkNotebookPage *page, guint new_page_num, ChmseeUiChmfile *self)
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
      if (title && title[0]) {
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
    gtk_window_set_title(GTK_WINDOW (self), "ChmseeUiChmfile");
    check_history(self, NULL);
  }
}

static void
html_location_changed_cb(ChmseeIhtml *html, const gchar *location, ChmseeUiChmfile *chmsee)
{
        g_debug("html location changed cb: %s", location);

        if (html == get_active_html(chmsee))
                check_history(chmsee, html);
}

static gboolean
html_open_uri_cb(ChmseeIhtml* html, const gchar *uri, ChmseeUiChmfile *self)
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
html_title_changed_cb(ChmseeIhtml *html, const gchar *title, ChmseeUiChmfile *self)
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
html_context_normal_cb(ChmseeIhtml *html, ChmseeUiChmfile *self)
{
  g_message("html context-normal event");
  /* TODO:
   * gtk_menu_popup(GTK_MENU(gtk_ui_manager_get_widget(selfp->ui_manager, "/HtmlContextNormal")),
                 NULL, NULL, NULL, NULL, 0, GDK_CURRENT_TIME); */
}

/* Popup html context menu when mouse over hyper link */
static void
html_context_link_cb(ChmseeIhtml *html, const gchar *link, ChmseeUiChmfile* self)
{
	g_debug("html context-link event: %s", link);
	chmsee_set_context_menu_link(self, link);
	/*
	 * TODO:
	gtk_action_set_sensitive(gtk_action_group_get_action(selfp->action_group, "OpenLinkInNewTab"),
			g_str_has_prefix(selfp->context_menu_link, "file://"));

	gtk_menu_popup(GTK_MENU(gtk_ui_manager_get_widget(selfp->ui_manager, "/HtmlContextLink")),
			NULL, NULL, NULL, NULL, 0, GDK_CURRENT_TIME);
			*/

}

static void
html_open_new_tab_cb(ChmseeIhtml *html, const gchar *location, ChmseeUiChmfile *chmsee)
{
        g_debug("html open new tab callback: %s", location);

        new_tab(chmsee, location);
}

static void
html_link_message_cb(ChmseeIhtml *html, const gchar *url, ChmseeUiChmfile *chmsee)
{
}

static void
on_close_tab(GtkWidget *widget, ChmseeUiChmfile *self)
{
        gint num_pages, number, i;
        GtkWidget *tab_label, *page;

        number = -1;
        num_pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK (selfp->html_notebook));

        if (num_pages == 1) {
        	/* TODO: should open a new empty tab */
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
on_copy(GtkWidget *widget, ChmseeUiChmfile *self)
{
        g_message("On Copy");

        g_return_if_fail(GTK_IS_NOTEBOOK (selfp->html_notebook));

        chmsee_ihtml_copy_selection(get_active_html(self));
}

static void
on_copy_page_location(GtkWidget* widget, ChmseeUiChmfile* chmsee) {
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
on_select_all(GtkWidget *widget, ChmseeUiChmfile *self)
{
        ChmseeIhtml *html;

        g_message("On Select All");

        g_return_if_fail(GTK_IS_NOTEBOOK (selfp->html_notebook));

        html = get_active_html(self);
        chmsee_ihtml_select_all(html);
}

static void
on_back(GtkWidget *widget, ChmseeUiChmfile *chmsee)
{
  chmsee_ihtml_go_back(get_active_html(chmsee));
}

static void
on_forward(GtkWidget *widget, ChmseeUiChmfile *chmsee)
{
  chmsee_ihtml_go_forward(get_active_html(chmsee));
}

static void
on_home(GtkWidget *widget, ChmseeUiChmfile *self)
{
  if (chmsee_ichmfile_get_home(selfp->book) != NULL) {
    open_homepage(self);
  }
}

static void
on_zoom_in(GtkWidget *widget, ChmseeUiChmfile *self)
{
	ChmseeIhtml* html = get_active_html(self);
	if(html != NULL) {
		chmsee_ihtml_increase_size(html);
	}
}

static void
on_zoom_reset(GtkWidget *widget, ChmseeUiChmfile *chmsee)
{
  chmsee_ihtml_reset_size(get_active_html(chmsee));
}

static void
on_zoom_out(GtkWidget *widget, ChmseeUiChmfile *self)
{
	ChmseeIhtml* html = get_active_html(self);
	if(html != NULL) {
		chmsee_ihtml_decrease_size(html);
	}
}

static void
on_open_new_tab(GtkWidget *widget, ChmseeUiChmfile *self)
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
on_close_current_tab(GtkWidget *widget, ChmseeUiChmfile *self)
{
        g_return_if_fail(GTK_IS_NOTEBOOK (selfp->html_notebook));

        if (gtk_notebook_get_n_pages(GTK_NOTEBOOK (selfp->html_notebook)) == 1) {
        	/* TODO: should open a new tab */
        }

        gint page_num;

        page_num = gtk_notebook_get_current_page(GTK_NOTEBOOK (selfp->html_notebook));

        if (page_num >= 0)
                gtk_notebook_remove_page(GTK_NOTEBOOK (selfp->html_notebook), page_num);
}

static void
on_context_new_tab(GtkWidget *widget, ChmseeUiChmfile *self)
{
	g_debug("On context open new tab: %s", selfp->context_menu_link);

	if (selfp->context_menu_link != NULL) {
		new_tab(self, selfp->context_menu_link);
	}
}

static void
on_context_copy_link(GtkWidget *widget, ChmseeUiChmfile *self)
{
	g_debug("On context copy link: %s", selfp->context_menu_link);

	if (selfp->context_menu_link != NULL) {
		gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY),
				selfp->context_menu_link, -1);
		gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD),
				selfp->context_menu_link, -1);
	}
}


static void
chmsee_ui_chmfile_populate_window(ChmseeUiChmfile *self)
{
	GtkWidget* control_vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(control_vbox);

	GtkWidget* control_notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX (control_vbox),
			GTK_WIDGET (control_notebook),
			TRUE,
			TRUE,
			2);
	g_signal_connect(G_OBJECT(control_notebook),
			"switch-page",
			G_CALLBACK (control_switch_page_cb),
			self);
	gtk_widget_show(control_notebook);


	GtkWidget* topic_page = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (topic_page),
			GTK_POLICY_NEVER,
			GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW (topic_page),
			GTK_SHADOW_IN);
	gtk_container_set_border_width(GTK_CONTAINER (topic_page), 2);

	GtkWidget* booktree = GTK_WIDGET(g_object_ref_sink(booktree_new(NULL)));
	g_signal_connect_swapped(booktree,
			"scroll-event",
			G_CALLBACK(on_scroll_event),
			self);

	gtk_container_add(GTK_CONTAINER (topic_page), booktree);
	gtk_widget_show_all(topic_page);
	gtk_notebook_append_page(GTK_NOTEBOOK (control_notebook),
			topic_page,
			gtk_label_new(_("Topics")));

	g_signal_connect(G_OBJECT (booktree),
			"link-selected",
			G_CALLBACK (booktree_link_selected_cb),
			self);

	gtk_notebook_append_page(GTK_NOTEBOOK (control_notebook),
			chmsee_new_index_page(self),
			gtk_label_new(_("Index")));

	selfp->bookmark_tree = GTK_WIDGET (ui_bookmarks_new(NULL));

	gtk_notebook_append_page(GTK_NOTEBOOK (control_notebook),
			selfp->bookmark_tree,
			gtk_label_new (_("Bookmarks")));

	g_signal_connect(G_OBJECT (selfp->bookmark_tree),
			"link-selected",
			G_CALLBACK (bookmarks_link_selected_cb),
			self);

	gtk_paned_add1(GTK_PANED(self), control_vbox);

	/* HTML tabs notebook */
	GtkWidget* html_notebook = gtk_notebook_new();
	g_signal_connect(G_OBJECT (html_notebook),
			"switch-page",
			G_CALLBACK (html_switch_page_cb),
			self);

	gtk_widget_show(html_notebook);
	gtk_paned_add2 (GTK_PANED (self), html_notebook);
	gtk_widget_show_all(GTK_WIDGET(self));

	g_signal_connect(G_OBJECT (html_notebook),
			"switch-page",
			G_CALLBACK (html_switch_page_cb),
			self);


	selfp->control_notebook = control_notebook;
	selfp->html_notebook = html_notebook;
	selfp->booktree = booktree;
	selfp->topic_page = topic_page;

	new_tab(self, NULL);
}

void
chmsee_ui_chmfile_set_model(ChmseeUiChmfile* self, ChmseeIchmfile *book)
{
	GNode *link_tree;
	GList *bookmarks_list;

	GtkWidget *booktree_sw;

	g_debug("display book");
	selfp->state = CHMSEE_STATE_LOADING;

	/* Close currently opened book */
	if (selfp->book) {
		chmsee_ui_chmfile_close_current_book(self);
	}

	selfp->book = g_object_ref(book);

	/* TOC */
	if (chmsee_ichmfile_get_link_tree(selfp->book) != NULL) {
		booktree_set_model(BOOKTREE(selfp->booktree), link_tree = chmsee_ichmfile_get_link_tree(selfp->book));
		gtk_widget_show(selfp->topic_page);
		selfp->has_toc = TRUE;
	} else {
		gtk_widget_hide(selfp->topic_page);
		selfp->has_toc = FALSE;
	}

	/* Index */
	chmsee_refresh_index(self);

	/* Bookmarks */
	bookmarks_list = chmsee_ichmfile_get_bookmarks_list(selfp->book);
	ui_bookmarks_set_model(selfp->bookmark_tree, bookmarks_list);


	gtk_notebook_set_current_page(GTK_NOTEBOOK (selfp->control_notebook),
			g_list_length(bookmarks_list) && selfp->has_toc ? 1 : 0);

	chmsee_ihtml_set_variable_font(get_active_html(self),
			chmsee_ichmfile_get_variable_font(selfp->book));
	chmsee_ihtml_set_fixed_font(get_active_html(self),
			chmsee_ichmfile_get_fixed_font(selfp->book));

	if (chmsee_ichmfile_get_home(selfp->book)) {
		open_homepage(self);

	}
	selfp->state = CHMSEE_STATE_NORMAL;

    selfp->last_dir = g_strdup_printf("%s", g_path_get_dirname(
    		chmsee_ichmfile_get_filename(book)));
}

static void
chmsee_ui_chmfile_close_current_book(ChmseeUiChmfile *self)
{
  gchar* bookmark_fname = g_build_filename(chmsee_ichmfile_get_dir(selfp->book), CHMSEE_BOOKMARK_FILE, NULL);
  bookmarks_save(ui_bookmarks_get_list(UIBOOKMARKS (selfp->bookmark_tree)), bookmark_fname);
  g_free(bookmark_fname);
  g_object_unref(selfp->book);

  selfp->book = NULL;
  selfp->state = CHMSEE_STATE_INIT;
}

static GtkWidget*
new_tab_content(ChmseeUiChmfile *chmsee, const gchar *str)
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
new_tab(ChmseeUiChmfile *self, const gchar *location)
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

        if (location != NULL) {
                chmsee_ihtml_open_uri(html, location);

                if (selfp->has_toc)
                        booktree_select_uri(BOOKTREE (selfp->booktree), location);
        } else {
                /* chmsee_ihtml_clear(html); */
        }

        gtk_notebook_set_current_page(GTK_NOTEBOOK (selfp->html_notebook), num);
}

static void
open_homepage(ChmseeUiChmfile *self)
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
reload_current_page(ChmseeUiChmfile *self)
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
get_active_html(ChmseeUiChmfile *self)
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
check_history(ChmseeUiChmfile *self, ChmseeIhtml *html)
{
	gboolean back_state, forward_state;

	back_state = chmsee_ihtml_can_go_back(html);
	forward_state = chmsee_ihtml_can_go_forward(html);
}

static void
update_tab_title(ChmseeUiChmfile *self, ChmseeIhtml *html)
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
        g_free(html_title);
}

static void
tab_set_title(ChmseeUiChmfile *self, ChmseeIhtml *html, const gchar *title)
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

/* external functions */

void
chmsee_open_file(ChmseeUiChmfile *self, const gchar *filename)
{
        ChmseeIchmfile* book;

        g_return_if_fail(IS_CHMSEE (self));

        /* Extract chm and get file infomation */
        book = chmsee_chmfile_new(filename);

        if (book) {
                chmsee_ui_chmfile_set_model(self, book);
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
      chmsee_open_uri(CHMSEE_UI_CHMFILE(widget), uri);
      break;
    }
  }

  gtk_drag_finish (context, TRUE, FALSE, time);

  g_strfreev (uris);
}

void chmsee_open_uri(ChmseeUiChmfile *chmsee, const gchar *uri) {
  if(!g_str_has_prefix(uri, "file://")) {
    return;
  }

  gchar* fname = g_uri_unescape_string(uri+7, NULL);
  chmsee_open_file(chmsee, fname);
  g_free(fname);
}

int chmsee_ui_chmfile_get_hpaned_position(ChmseeUiChmfile* self) {
	return gtk_paned_get_position(GTK_PANED(self));
}

void chmsee_ui_chmfile_set_hpaned_position(ChmseeUiChmfile* self, int hpaned_position) {
	selfp->hpaned_position = hpaned_position;
	/*
	g_object_set(G_OBJECT(get_widget(self, "hpaned1")),
			"position", hpaned_position,
			NULL
			);
			*/
}

void on_sidepane_toggled(GtkWidget* menu, ChmseeUiChmfile* self) {
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

void set_sidepane_state(ChmseeUiChmfile* self, gboolean state) {
	GtkWidget* icon_widget;

	if(state) {
		gtk_widget_show(selfp->control_notebook);
	} else {
		gtk_widget_hide(selfp->control_notebook);
	}

    if (state) {
            icon_widget = gtk_image_new_from_file(get_resource_path("hide-pane.png"));
    } else {
            icon_widget = gtk_image_new_from_file(get_resource_path("show-pane.png"));
    }
    gtk_widget_show(icon_widget);
};

void show_sidepane(ChmseeUiChmfile* self) {
	set_sidepane_state(self, TRUE);
}

void hide_sidepane(ChmseeUiChmfile* self) {
	set_sidepane_state(self, FALSE);
}


static gboolean on_scroll_event(ChmseeUiChmfile* self, GdkEventScroll* event) {
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

const gchar* chmsee_ui_chmfile_get_cache_dir(ChmseeUiChmfile* self) {
	return selfp->cache_dir;
}

const gchar* chmsee_ui_chmfile_get_variable_font(ChmseeUiChmfile* self) {
	g_return_val_if_fail(selfp->book, NULL);
	return chmsee_ichmfile_get_variable_font(selfp->book);
}

void chmsee_ui_chmfile_set_variable_font(ChmseeUiChmfile* self, const gchar* font_name) {
	g_return_if_fail(selfp->book);
    chmsee_ichmfile_set_variable_font(selfp->book, font_name);
}

const gchar* chmsee_ui_chmfile_get_fixed_font(ChmseeUiChmfile* self) {
	g_return_val_if_fail(selfp->book, NULL);
	return chmsee_ichmfile_get_fixed_font(selfp->book);
}

void chmsee_ui_chmfile_set_fixed_font(ChmseeUiChmfile* self, const gchar* font_name) {
	g_return_if_fail(selfp->book);
    chmsee_ichmfile_set_fixed_font(selfp->book, font_name);
}

int chmsee_ui_chmfile_get_lang(ChmseeUiChmfile* self) {
	return selfp->lang;
}
void chmsee_ui_chmfile_set_lang(ChmseeUiChmfile* self, int lang) {
	selfp->lang = lang;
}

gboolean chmsee_ui_chmfile_has_book(ChmseeUiChmfile* self) {
	return selfp->book != NULL;
}

void chmsee_refresh_index(ChmseeUiChmfile* self) {
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

static GtkWidget* chmsee_new_index_page(ChmseeUiChmfile* self) {
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

void chmsee_on_ui_index_link_selected(ChmseeUiChmfile* self, Link* link) {
	booktree_link_selected_cb(NULL, link, self);
}


gboolean chmsee_ui_chmfile_jump_index_by_name(ChmseeUiChmfile* self, const gchar* name) {
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

static void chmsee_set_context_menu_link(ChmseeUiChmfile* self, const gchar* link) {
	g_free(selfp->context_menu_link);
	selfp->context_menu_link = g_strdup(link);
}

GtkWidget* chmsee_ui_chmfile_new() {
	return GTK_WIDGET(g_object_new(CHMSEE_TYPE_UI_CHMFILE, NULL));
}
