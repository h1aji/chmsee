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
 *  along with Chmsee; see the file COPYING.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "book.h"
#include "toc.h"
#include "bookmarks.h"
#include "index.h"
#include "html-webkit.h"
#include "utils.h"
#include "models/chmfile.h"
#include "models/link.h"

/* Signals */
enum {
        MODEL_CHANGED,
        HTML_CHANGED,
        LAST_SIGNAL
};
static gint signals[LAST_SIGNAL] = { 0 };

enum {
        PROP_0,

        PROP_SIDEPANE_VISIBLE,
        PROP_BOOK_MESSAGE
};

typedef struct _CsBookPrivate CsBookPrivate;

struct _CsBookPrivate {
        GtkWidget       *hpaned;
        GtkWidget       *findbar;

        GtkWidget       *control_notebook;
        GtkWidget       *html_notebook;

        GtkWidget       *toc_page;
        GtkWidget       *index_page;
        GtkWidget       *bookmarks_page;

        GtkActionGroup  *action_group;
        GtkUIManager    *ui_manager;

        CsChmfile       *model;
        CsHtmlWebkit     *active_html;

        gchar           *context_menu_link;
        gchar           *book_message;
};

#define CS_BOOK_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), CS_TYPE_BOOK, CsBookPrivate))

static void cs_book_class_init(CsBookClass *);
static void cs_book_init(CsBook *);
static void cs_book_dispose(GObject *);
static void cs_book_finalize(GObject *);
static void cs_book_set_property(GObject *, guint, const GValue *, GParamSpec *);
static void cs_book_get_property(GObject *, guint, GValue *, GParamSpec *);

static void find_entry_changed_cb(GtkEntry *, CsBook *);
static void find_entry_activate_cb(GtkEntry *, CsBook *);
static void link_selected_cb(GtkWidget *, Link *, CsBook *);
static void html_notebook_switch_page_cb(GtkNotebook *, GtkNotebookPage *, guint , CsBook *);
static void html_location_changed_cb(CsHtmlWebkit *, const gchar *, CsBook *);
static gboolean html_open_uri_cb(CsHtmlWebkit *, const gchar *, CsBook *);
static void html_title_changed_cb(CsHtmlWebkit *, const gchar *, CsBook *);
static void html_context_normal_cb(CsHtmlWebkit *, CsBook *);
static void html_context_link_cb(CsHtmlWebkit *, const gchar *, CsBook *);
static void html_open_new_tab_cb(CsHtmlWebkit *, const gchar *, CsBook *);
static void html_link_message_cb(CsHtmlWebkit *, const gchar *, CsBook *);

static void on_tab_close(GtkWidget *, CsBook *);
static void on_copy(GtkAction *, CsBook *);
static void on_copy_page_location(GtkAction *, CsBook *);
static void on_select_all(GtkAction *, CsBook *);
static void on_back(GtkAction *, CsBook *);
static void on_forward(GtkAction *, CsBook *);
static void on_context_new_tab(GtkAction *, CsBook *);
static void on_context_copy_link(GtkAction *, CsBook *);
static void on_findbar_hide(GtkWidget *, CsBook *);
static void on_findbar_back(GtkWidget *, CsBook *);
static void on_findbar_forward(GtkWidget *, CsBook *);

static void update_book_message(CsBook *, const gchar *);
static gint new_html_tab(CsBook *);
static GtkWidget *new_tab_label(CsBook *, const gchar *);
static void update_tab_title(CsBook *, CsHtmlWebkit *, const gchar *);
static void update_tab_label_state(CsBook *);
static void set_context_menu_link(CsBook *, const gchar *);
static void find_text(GtkWidget *, CsBook *, gboolean);
static gint uri_compare(gconstpointer, gconstpointer);
static const gchar *get_short_uri(CsChmfile *, const gchar *);
static GList *get_toc_current(CsBook *);

static const GtkActionEntry entries[] = {
        { "Copy", GTK_STOCK_COPY, N_("_Copy"), "<control>C", NULL, G_CALLBACK(on_copy)},
        { "Back", GTK_STOCK_GO_BACK, N_("_Back"), "<alt>Left", NULL, G_CALLBACK(on_back)},
        { "Forward", GTK_STOCK_GO_FORWARD, N_("_Forward"), "<alt>Right", NULL, G_CALLBACK(on_forward)},
        { "OpenLinkInNewTab", NULL, N_("Open Link in New _Tab"), NULL, NULL, G_CALLBACK(on_context_new_tab)},
        { "CopyLinkLocation", NULL, N_("_Copy Link Location"), NULL, NULL, G_CALLBACK(on_context_copy_link)},
        { "SelectAll", NULL, N_("Select _All"), NULL, NULL, G_CALLBACK(on_select_all)},
        { "CopyPageLocation", NULL, N_("Copy Page _Location"), NULL, NULL, G_CALLBACK(on_copy_page_location)},
};

/* Radio items */
static const GtkRadioActionEntry radio_entries[] = {
};

static const char *ui_description =
        "<ui>"
        "  <popup name='HtmlContextLink'>"
        "    <menuitem action='OpenLinkInNewTab' name='OpenLinkInNewTab'/>"
        "    <menuitem action='CopyLinkLocation'/>"
        "  </popup>"
        "  <popup name='HtmlContextNormal'>"
        "    <menuitem action='Back'/>"
        "    <menuitem action='Forward'/>"
        "    <menuitem action='SelectAll'/>"
        "    <menuitem action='CopyPageLocation'/>"
        "  </popup>"
        "  <popup name='HtmlContextNormalCopy'>"
        "    <menuitem action='Copy'/>"
        "    <menuitem action='SelectAll'/>"
        "    <menuitem action='CopyPageLocation'/>"
        "  </popup>"
        "</ui>";


/* GObject functions */

G_DEFINE_TYPE (CsBook, cs_book, GTK_TYPE_VBOX);

static void
cs_book_class_init(CsBookClass *klass)
{
        GParamSpec* pspec;

        g_type_class_add_private(klass, sizeof(CsBookPrivate));

        G_OBJECT_CLASS(klass)->finalize = cs_book_finalize;
        G_OBJECT_CLASS(klass)->dispose = cs_book_dispose;

        signals[MODEL_CHANGED] =
                g_signal_new("model-changed",
                             G_TYPE_FROM_CLASS (klass),
                             G_SIGNAL_RUN_LAST,
                             0,
                             NULL,
                             NULL,
                             gtk_marshal_VOID__POINTER_POINTER,
                             G_TYPE_NONE,
                             2,
                             G_TYPE_POINTER, G_TYPE_POINTER);

        signals[HTML_CHANGED] =
                g_signal_new("html-changed",
                             G_TYPE_FROM_CLASS(klass),
                             G_SIGNAL_RUN_LAST,
                             0,
                             NULL,
                             NULL,
                             gtk_marshal_VOID__POINTER,
                             G_TYPE_NONE,
                             1,
                             G_TYPE_POINTER);

        G_OBJECT_CLASS(klass)->set_property = cs_book_set_property;
        G_OBJECT_CLASS(klass)->get_property = cs_book_get_property;

        pspec = g_param_spec_boolean("sidepane-visible", NULL, NULL, TRUE, G_PARAM_READWRITE);
        g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_SIDEPANE_VISIBLE, pspec);

        pspec = g_param_spec_string("book-message", NULL, NULL, "", G_PARAM_READABLE);
        g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_BOOK_MESSAGE, pspec);
}

static void
cs_book_init(CsBook *self)
{
        CsBookPrivate *priv = CS_BOOK_GET_PRIVATE (self);

        priv->context_menu_link = NULL;

        priv->model = NULL;
        priv->active_html = NULL;
        priv->book_message = NULL;
        priv->toc_page = NULL;

        priv->hpaned = gtk_hpaned_new();
        gtk_box_pack_start(GTK_BOX (self), priv->hpaned, TRUE, TRUE, 0);

        priv->control_notebook = gtk_notebook_new();
        gtk_notebook_set_tab_vborder(GTK_NOTEBOOK (priv->control_notebook), 4);
        gtk_notebook_set_show_border(GTK_NOTEBOOK (priv->control_notebook), FALSE);
        gtk_paned_add1(GTK_PANED(priv->hpaned), priv->control_notebook);

        priv->html_notebook = gtk_notebook_new();
        g_signal_connect(G_OBJECT (priv->html_notebook),
                         "switch-page",
                         G_CALLBACK (html_notebook_switch_page_cb),
                         self);

        gtk_notebook_set_tab_vborder(GTK_NOTEBOOK (priv->html_notebook), 0);
        gtk_notebook_set_show_border(GTK_NOTEBOOK (priv->html_notebook), FALSE);
        gtk_paned_add2(GTK_PANED (priv->hpaned), priv->html_notebook);

        /* string find bar */
        priv->findbar = GTK_WIDGET (gtk_hbox_new(FALSE, 2));

        GtkWidget *close_button = gtk_button_new();
        gtk_button_set_relief(GTK_BUTTON(close_button), GTK_RELIEF_NONE);
        GtkWidget *close_image = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_SMALL_TOOLBAR);
        gtk_container_add(GTK_CONTAINER (close_button), close_image);

        g_signal_connect(G_OBJECT (close_button),
                         "clicked",
                         G_CALLBACK (on_findbar_hide),
                         self);
        gtk_box_pack_start(GTK_BOX (priv->findbar), close_button, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX (priv->findbar), gtk_label_new(_("Find:")), FALSE, FALSE, 0);

        GtkWidget *find_entry = gtk_entry_new();
        gtk_entry_set_max_length(GTK_ENTRY (find_entry), ENTRY_MAX_LENGTH);
        gtk_box_pack_start(GTK_BOX (priv->findbar), find_entry, FALSE, FALSE, 0);
        g_object_set_data(G_OBJECT (priv->findbar), "find-entry", find_entry);
        g_signal_connect(find_entry,
                         "changed",
                         G_CALLBACK (find_entry_changed_cb),
                         self);
        g_signal_connect(find_entry,
                         "activate",
                         G_CALLBACK (find_entry_activate_cb),
                         self);

        GtkWidget *find_back = gtk_button_new_with_label(_("Previous"));
        gtk_button_set_image(GTK_BUTTON (find_back),
                             gtk_image_new_from_stock(GTK_STOCK_GO_BACK, GTK_ICON_SIZE_SMALL_TOOLBAR));
        g_signal_connect(G_OBJECT (find_back),
                         "clicked",
                         G_CALLBACK (on_findbar_back),
                         self);
        gtk_box_pack_start(GTK_BOX (priv->findbar), find_back, FALSE, FALSE, 0);
        gtk_button_set_relief(GTK_BUTTON(find_back), GTK_RELIEF_NONE);

        GtkWidget *find_forward = gtk_button_new_with_label(_("Next"));
        gtk_button_set_image(GTK_BUTTON (find_forward),
                             gtk_image_new_from_stock(GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_SMALL_TOOLBAR));
        g_signal_connect(G_OBJECT (find_forward),
                         "clicked",
                         G_CALLBACK (on_findbar_forward),
                         self);
        gtk_box_pack_start(GTK_BOX (priv->findbar), find_forward, FALSE, FALSE, 0);
        gtk_button_set_relief(GTK_BUTTON(find_forward), GTK_RELIEF_NONE);

        GtkWidget *find_case = gtk_check_button_new_with_label(_("Match case"));
        gtk_box_pack_start(GTK_BOX (priv->findbar), find_case, FALSE, FALSE, 0);
        g_object_set_data(G_OBJECT (priv->findbar), "find-case", find_case);

        gtk_box_pack_start(GTK_BOX (self), priv->findbar, FALSE, FALSE, 0);

        /* HTML content popup menu */
        priv->action_group = gtk_action_group_new("BookActions");
        gtk_action_group_set_translation_domain(priv->action_group, NULL);
        gtk_action_group_add_actions(priv->action_group, entries, G_N_ELEMENTS (entries), self);

        gtk_action_set_sensitive(gtk_action_group_get_action(priv->action_group, "Back"), FALSE);
        gtk_action_set_sensitive(gtk_action_group_get_action(priv->action_group, "Forward"), FALSE);

        priv->ui_manager = gtk_ui_manager_new();
        gtk_ui_manager_insert_action_group(priv->ui_manager, priv->action_group, 0);

        GError* error = NULL;
        if (!gtk_ui_manager_add_ui_from_string(priv->ui_manager, ui_description, -1, &error)) {
                g_warning("CS_BOOK >>> building menus failed %s", error->message);
                g_error_free(error);
                exit(EXIT_FAILURE);
        }

        gtk_box_set_homogeneous(GTK_BOX (self), FALSE);
        gtk_widget_show_all(GTK_WIDGET (self));
}

static void
cs_book_dispose(GObject* gobject)
{
        g_debug("CS_BOOK >>> dispose");

        CsBook *self = CS_BOOK(gobject);
        CsBookPrivate *priv = CS_BOOK_GET_PRIVATE (self);

        if (priv->model != NULL) {
                GList *old_list = cs_bookmarks_get_model(CS_BOOKMARKS (priv->bookmarks_page));
                cs_chmfile_update_bookmarks_list(priv->model, old_list);

                g_object_unref(priv->model);
                priv->model = NULL;
        }

        if (priv->action_group != NULL) {
                g_object_unref(priv->action_group);
                g_object_unref(priv->ui_manager);
                priv->action_group = NULL;
                priv->ui_manager = NULL;
        }

        G_OBJECT_CLASS(cs_book_parent_class)->dispose(gobject);
}

static void
cs_book_finalize(GObject *object)
{
        g_debug("CS_BOOK >>> finalize");
        CsBookPrivate *priv = CS_BOOK_GET_PRIVATE (CS_BOOK(object));

        g_free(priv->context_menu_link);
        g_free(priv->book_message);

        G_OBJECT_CLASS (cs_book_parent_class)->finalize(object);
}

/* Callbacks */

static void
find_entry_changed_cb(GtkEntry *entry, CsBook *self)
{
        find_text(GTK_WIDGET (entry), self, FALSE);
}

static void
find_entry_activate_cb(GtkEntry *entry, CsBook *self)
{
        find_text(GTK_WIDGET (entry), self, FALSE);
}

static void
link_selected_cb(GtkWidget *widget, Link *link, CsBook *self)
{
        g_debug("CS_BOOK >>> link selected callback, url = %s", link->uri);
        if (!g_ascii_strcasecmp(CHMSEE_NO_LINK, link->uri) || strlen(link->uri) == 0)
                return;

        char *scheme = g_uri_parse_scheme(link->uri);
        if (scheme && g_strcmp0(scheme, "file")) {
                g_message("%s is unsupported protocol.", scheme);
                gchar *message = g_strdup_printf("URI %s with unsupported protocol: %s", link->uri, scheme);
                update_book_message(self, message);
                g_free(message);
        } else {
                cs_book_load_url(self, link->uri, FALSE);
        }
        g_free(scheme);
}

static void
html_notebook_switch_page_cb(GtkNotebook *notebook, GtkNotebookPage *page, guint new_page_num, CsBook *self)
{
        g_debug("CS_BOOK >>> enter switch page callback");
        CsBookPrivate *priv = CS_BOOK_GET_PRIVATE (self);

        GtkWidget *new_page = gtk_notebook_get_nth_page(notebook, new_page_num);
        g_debug("CS_BOOK >>> switch page new_page_num = %d, new_page = %p", new_page_num, new_page);

        if (new_page != NULL) {
                g_debug("CS_BOOK >>> switch page callback, set active_html = %p", priv->active_html);
                priv->active_html = CS_HTML_WEBKIT (new_page);
                cs_book_reload_current_page(self);
        }

        g_signal_emit(self, signals[HTML_CHANGED], 0, self);
}

static void
html_location_changed_cb(CsHtmlWebkit *html, const gchar *location, CsBook *self)
{
        g_debug("CS_BOOK >>> html location changed cb: %s", location);

        g_signal_emit(self, signals[HTML_CHANGED], 0, self);

        CsBookPrivate *priv = CS_BOOK_GET_PRIVATE (self);

        char *scheme = g_uri_parse_scheme(location);

        if (!g_strcmp0(scheme, "file") && priv->toc_page) {
                gchar *real_uri = get_real_uri(location);
                gchar *filename = g_filename_from_uri(real_uri, NULL, NULL);

                const gchar *uri = get_short_uri(priv->model, filename);
                gchar *toc_uri = g_strdup_printf("%s%s", uri, location + strlen(real_uri));

                cs_toc_sync(CS_TOC (priv->toc_page), toc_uri);

                g_free(real_uri);
                g_free(filename);
                g_free(toc_uri);
        }
}

static gboolean
html_open_uri_cb(CsHtmlWebkit *html, const gchar *full_uri, CsBook *self)
{
        g_debug("CS_BOOK >>> enter html_open_uri_cb with uri = %s", full_uri);
        if (full_uri == NULL || strlen(full_uri) == 0)
                return TRUE;

        CsBookPrivate *priv = CS_BOOK_GET_PRIVATE (self);

        char *scheme = g_uri_parse_scheme(full_uri);

        if (scheme != NULL) {
                const gchar *bookfolder = cs_chmfile_get_bookfolder(priv->model);

                if (!g_strcmp0(scheme, "file")) {
                        /* DND chmfile check */
                        if (g_str_has_suffix(full_uri, ".chm") || g_str_has_suffix(full_uri, ".CHM")) {
                                g_debug("CS_BOOK >>> open chm file = %s", full_uri);
                                g_signal_emit(self, signals[MODEL_CHANGED], 0, NULL, full_uri);
                        } else if (g_strrstr(full_uri, bookfolder)) {
                                const gchar *uri = get_short_uri(priv->model, full_uri);

                                g_debug("CS_BOOK >>> html_open_uri call load url = %s", uri);
                                cs_book_load_url(self, uri, TRUE);
                        }
                } else if (!g_strcmp0(scheme, "about") || !g_strcmp0(scheme, "jar")) {
                        return FALSE;
                }
        }

        return TRUE;
}

static void
html_title_changed_cb(CsHtmlWebkit *html, const gchar *title, CsBook *self)
{
        g_debug("CS_BOOK >>> html title changed cb title = %s", title);

        CsBookPrivate *priv = CS_BOOK_GET_PRIVATE (self);

        const gchar *label_text = _("No Title");

        if (title && title[0] != '\0')
                label_text = title;

        update_tab_title(self, html, label_text);

        /* update bookmarks title entry */
        gchar *location = cs_html_webkit_get_location(html);

        if (location != NULL && strlen(location)) {
                if (!g_str_has_prefix(location, "about:")) {
                        Link *link = link_new(LINK_TYPE_PAGE,
                                              label_text,
                                              get_short_uri(priv->model, location));

                        cs_bookmarks_set_current_link(CS_BOOKMARKS (priv->bookmarks_page), link);
                        link_free(link);
                }
                g_free(location);
        }
}

static void
html_context_normal_cb(CsHtmlWebkit *html, CsBook *self)
{
        g_debug("CS_BOOK >>> html context-normal event");

        CsBookPrivate *priv = CS_BOOK_GET_PRIVATE (self);

        gboolean can_copy = cs_html_webkit_can_copy_selection(priv->active_html);
        gtk_action_set_sensitive(gtk_action_group_get_action(priv->action_group, "Copy"),
                                 can_copy);
        gtk_action_set_sensitive(gtk_action_group_get_action(priv->action_group, "Back"),
                                 cs_book_can_go_back(self));
        gtk_action_set_sensitive(gtk_action_group_get_action(priv->action_group, "Forward"),
                                 cs_book_can_go_forward(self));

        const gchar *pop_menu = can_copy ? "/HtmlContextNormalCopy" : "/HtmlContextNormal";
        gtk_menu_popup(GTK_MENU (gtk_ui_manager_get_widget(priv->ui_manager, pop_menu)),
                       NULL, NULL, NULL, NULL, 0, GDK_CURRENT_TIME);
}

static void
html_context_link_cb(CsHtmlWebkit *html, const gchar *link, CsBook *self)
{
        g_debug("CS_BOOK >>> html context-link event: %s", link);

        CsBookPrivate *priv = CS_BOOK_GET_PRIVATE (self);

        set_context_menu_link(self, link);
        gtk_action_set_sensitive(gtk_action_group_get_action(priv->action_group, "OpenLinkInNewTab"),
                                 g_str_has_prefix(priv->context_menu_link, "file://"));

        gtk_menu_popup(GTK_MENU(gtk_ui_manager_get_widget(priv->ui_manager, "/HtmlContextLink")),
                       NULL, NULL, NULL, NULL, 0, GDK_CURRENT_TIME);
}

static void
html_open_new_tab_cb(CsHtmlWebkit *html, const gchar *location, CsBook *self)
{
        g_debug("CS_BOOK >>> html open new tab callback: %s", location);
        cs_book_new_tab_with_fulluri(self, location);
}

static void
html_link_message_cb(CsHtmlWebkit *html, const gchar *url, CsBook *self)
{
        update_book_message(self, url);
}

static void
on_tab_close(GtkWidget *widget, CsBook *self)
{
        CsBookPrivate *priv = CS_BOOK_GET_PRIVATE (self);

        if (gtk_notebook_get_n_pages(GTK_NOTEBOOK (priv->html_notebook)) >= 1) {
                GtkWidget *hbox = gtk_widget_get_ancestor(widget, GTK_TYPE_HBOX);
                GtkWidget *html = g_object_get_data(G_OBJECT (hbox), "html");

                gint num = gtk_notebook_page_num(GTK_NOTEBOOK (priv->html_notebook), html);
                g_debug("CS_BOOK >>> close tab find page %d", num);
                if (num >= 0)
                        gtk_notebook_remove_page(GTK_NOTEBOOK (priv->html_notebook), num);

                update_tab_label_state(self);
        }
}

static void
on_copy(GtkAction *action, CsBook *self)
{
        cs_book_copy(self);
}

static void
on_copy_page_location(GtkAction *action, CsBook *self)
{
        gchar *location = cs_html_webkit_get_location(CS_BOOK_GET_PRIVATE (self)->active_html);
        GdkAtom selection = location != NULL ? GDK_SELECTION_PRIMARY : GDK_SELECTION_CLIPBOARD;
        gtk_clipboard_set_text(gtk_clipboard_get(selection), location, -1);
        g_free(location);
}

static void
on_select_all(GtkAction *action, CsBook *self)
{
        cs_book_select_all(self);
}

static void
on_back(GtkAction *action, CsBook *self)
{
        cs_book_go_back(self);
}

static void
on_forward(GtkAction *action, CsBook *self)
{
        cs_book_go_forward(self);
}

static void
on_context_new_tab(GtkAction *action, CsBook *self)
{
        CsBookPrivate *priv = CS_BOOK_GET_PRIVATE (self);
        g_debug("CS_BOOK >>> On context open new tab: %s", priv->context_menu_link);

        if (priv->context_menu_link != NULL)
                cs_book_new_tab_with_fulluri(self, priv->context_menu_link);
}

static void
on_context_copy_link(GtkAction *action, CsBook *self)
{
        CsBookPrivate *priv = CS_BOOK_GET_PRIVATE (self);
        g_debug("CS_BOOK >>> On context copy link: %s", priv->context_menu_link);
        GdkAtom selection = priv->context_menu_link != NULL ? GDK_SELECTION_PRIMARY : GDK_SELECTION_CLIPBOARD;
        gtk_clipboard_set_text(gtk_clipboard_get(selection), priv->context_menu_link, -1);
}

static void
on_findbar_hide(GtkWidget *widget, CsBook *self)
{
        gtk_widget_hide(CS_BOOK_GET_PRIVATE (self)->findbar);
}

static void
on_findbar_back(GtkWidget *widget, CsBook *self)
{
        find_text(widget, self, TRUE);
}

static void
on_findbar_forward(GtkWidget *widget, CsBook *self)
{
        find_text(widget, self, FALSE);
}

/* Internal functions */

static void
cs_book_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
        GtkWidget *pane = gtk_paned_get_child1(GTK_PANED (CS_BOOK_GET_PRIVATE (CS_BOOK (object))->hpaned));

        switch (property_id) {
        case PROP_SIDEPANE_VISIBLE:
                g_value_get_boolean(value) ? gtk_widget_show(pane) : gtk_widget_hide(pane);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
cs_book_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
        CsBookPrivate *priv = CS_BOOK_GET_PRIVATE (CS_BOOK (object));

        switch (property_id) {
        case PROP_SIDEPANE_VISIBLE:
                g_value_set_boolean(value, GTK_WIDGET_VISIBLE (gtk_paned_get_child1(GTK_PANED (priv->hpaned))));
                break;
        case PROP_BOOK_MESSAGE:
                g_value_set_string(value, priv->book_message);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
update_book_message(CsBook *self, const gchar *message)
{
        CsBookPrivate *priv = CS_BOOK_GET_PRIVATE (self);
        if (priv->book_message)
                g_free(priv->book_message);

        priv->book_message = g_strdup(message);
        g_object_notify(G_OBJECT(self), "book-message");
}

static gint
new_html_tab(CsBook *self)
{
        g_debug("CS_BOOK >>> cs_book_new_tab");
        g_return_val_if_fail(IS_CS_BOOK (self), 0);

        CsBookPrivate *priv = CS_BOOK_GET_PRIVATE (self);

        GtkWidget *html = cs_html_webkit_new();
        gtk_widget_show(html);

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

        /* customized label, add a close button rightmost */
        GtkWidget *tab_label = new_tab_label(self, _("No Title"));
        g_object_set_data(G_OBJECT (tab_label), "html", html);

        gint page_num = gtk_notebook_append_page(GTK_NOTEBOOK (priv->html_notebook),
                                                 html,
                                                 tab_label);
        gtk_notebook_set_tab_label_packing(GTK_NOTEBOOK (priv->html_notebook),
                                           html,
                                           TRUE, TRUE,
                                           GTK_PACK_START);

        g_debug("CS_BOOK >>> new tab html_notebook append page = %d", page_num);

        return page_num;
}

static GtkWidget*
new_tab_label(CsBook *self, const gchar *str)
{
        GtkWidget *hbox  = gtk_hbox_new(FALSE, 2);

        GtkWidget *label = gtk_label_new(str);
        gtk_label_set_ellipsize(GTK_LABEL (label), PANGO_ELLIPSIZE_END);
        gtk_label_set_single_line_mode(GTK_LABEL (label), TRUE);
        gtk_misc_set_alignment(GTK_MISC (label), 0.0, 0.5);
        gtk_misc_set_padding(GTK_MISC (label), 0, 0);
        gtk_box_pack_start(GTK_BOX (hbox), label, TRUE, TRUE, 0);
        g_object_set_data(G_OBJECT (hbox), "label", label);

        GtkWidget *close_button = gtk_button_new();
        gtk_button_set_relief(GTK_BUTTON (close_button), GTK_RELIEF_NONE);
        gtk_container_set_border_width(GTK_CONTAINER (close_button), 0);

        GtkWidget *close_image = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
        gtk_misc_set_padding(GTK_MISC (close_image), 0, 0);
        gtk_button_set_image(GTK_BUTTON (close_button), close_image);

        /* gtk_container_add(GTK_CONTAINER (close_button), close_image); */

        g_signal_connect(G_OBJECT (close_button),
                         "clicked",
                         G_CALLBACK (on_tab_close),
                         self);

        gtk_box_pack_start(GTK_BOX (hbox), close_button, FALSE, FALSE, 0);

        gtk_widget_show_all(hbox);

        return hbox;
}

static void
update_tab_title(CsBook *self, CsHtmlWebkit *html, const gchar *title)
{
        g_debug("CS_BOOK >>> update tab title = %s", title);
        GtkWidget *widget = gtk_notebook_get_tab_label(GTK_NOTEBOOK (CS_BOOK_GET_PRIVATE (self)->html_notebook),
                                                       GTK_WIDGET (html));
        GtkWidget *label = g_object_get_data(G_OBJECT (widget), "label");
        gtk_label_set_text(GTK_LABEL (label), title);
}

static void
set_context_menu_link(CsBook *self, const gchar *link)
{
        CsBookPrivate *priv = CS_BOOK_GET_PRIVATE (self);
        g_free(priv->context_menu_link);
        priv->context_menu_link = g_strdup(link);
}

static void
find_text(GtkWidget *widget, CsBook *self, gboolean backward)
{
        CsBookPrivate *priv = CS_BOOK_GET_PRIVATE (self);

        GtkWidget *match_case = g_object_get_data(G_OBJECT (priv->findbar), "find-case");
        gboolean mcase = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (match_case));

        GtkWidget *find_entry = g_object_get_data(G_OBJECT (priv->findbar), "find-entry");
        const gchar *text = gtk_entry_get_text(GTK_ENTRY (find_entry));

        if (GTK_IS_BUTTON (widget) && backward && mcase) {
                if (g_str_has_prefix(text, "webkit:about:")) {
                    g_debug("CS_BOOK >>> call Webkit about: protocol = %s", text);
                    cs_html_webkit_load_url(priv->active_html, text+6);
                    return;
                }
        }

        cs_html_webkit_find(priv->active_html, text, backward, mcase);
}

static void
update_tab_label_state(CsBook *self)
{
        CsBookPrivate *priv = CS_BOOK_GET_PRIVATE (self);

        gboolean show = gtk_notebook_get_n_pages(GTK_NOTEBOOK (priv->html_notebook)) > 1 ? TRUE : FALSE;
        gtk_notebook_set_show_tabs(GTK_NOTEBOOK (priv->html_notebook), show);
}

static gint
uri_compare(gconstpointer a, gconstpointer b)
{
        return ncase_compare_utf8_string(((Link *)a)->uri, (gchar *)b);
}

static const gchar *
get_short_uri(CsChmfile *chmfile, const gchar *uri)
{
        const gchar *bookfolder = cs_chmfile_get_bookfolder(chmfile);

        const gchar *short_uri = g_strrstr(uri, bookfolder);
        if (short_uri == NULL)
                short_uri = uri;
        else
                short_uri = short_uri + strlen(bookfolder);

        if (short_uri[0] == '/')
                short_uri = short_uri + 1;

        return short_uri;
}

static GList *get_toc_current(CsBook *self)
{
        g_return_val_if_fail(IS_CS_BOOK (self), NULL);
        CsBookPrivate *priv = CS_BOOK_GET_PRIVATE (self);

        GList *toc_list = cs_chmfile_get_toc_list(priv->model);
        gchar *location = cs_html_webkit_get_location(priv->active_html);
        const gchar *short_uri = get_short_uri(priv->model, location);
        GList *current = g_list_find_custom(toc_list, short_uri, uri_compare);
        g_free(location);

        return current;
}

/* External functions*/

GtkWidget *
cs_book_new(void)
{
        g_debug("CS_BOOK >>> create");
        return GTK_WIDGET (g_object_new(CS_TYPE_BOOK, NULL));
}

void
cs_book_set_model(CsBook *self, CsChmfile *model)
{
        g_return_if_fail(IS_CS_BOOK (self));

        g_debug("CS_BOOK >>> set model, file = %p", model);

        CsBookPrivate *priv = CS_BOOK_GET_PRIVATE (self);

        /* close opened book */
        if (priv->model) {
                GList *old_list = cs_bookmarks_get_model(CS_BOOKMARKS (priv->bookmarks_page));
                cs_chmfile_update_bookmarks_list(priv->model, old_list);

                /* remove all notebook page tab */
                gint i, num;
                num = gtk_notebook_get_n_pages(GTK_NOTEBOOK (priv->control_notebook));
                for (i = 0; i < num; i++) {
                        gtk_notebook_remove_page(GTK_NOTEBOOK (priv->control_notebook), -1);
                }

                num = gtk_notebook_get_n_pages(GTK_NOTEBOOK (priv->html_notebook));
                for (i = 0; i < num; i++) {
                        gtk_notebook_remove_page(GTK_NOTEBOOK (priv->html_notebook), -1);
                }

                g_object_unref(priv->model);
        }

        priv->model = g_object_ref(model);

        cs_html_webkit_set_variable_font(CS_BOOK_GET_PRIVATE (self)->active_html,cs_chmfile_get_variable_font(model));
        cs_html_webkit_set_fixed_font(CS_BOOK_GET_PRIVATE (self)->active_html,cs_chmfile_get_fixed_font(model));

        gint cur_page = 0;

        /* TOC */
        GNode *toc_tree = cs_chmfile_get_toc_tree(model);
        if (toc_tree != NULL) {
                priv->toc_page = GTK_WIDGET (cs_toc_new());
                cs_toc_set_model(CS_TOC (priv->toc_page), toc_tree);
                cur_page = gtk_notebook_append_page(GTK_NOTEBOOK (priv->control_notebook),
                                                    priv->toc_page,
                                                    gtk_label_new(_("Topics")));

                g_signal_connect(G_OBJECT (priv->toc_page),
                                 "link-selected",
                                 G_CALLBACK (link_selected_cb),
                                 self);
        } else {
                g_message("CS_BOOK >>> this book dose not include a toc");
        }

        /* index */
        GList* index_list = cs_chmfile_get_index_list(model);
        if(index_list != NULL) {
                priv->index_page = GTK_WIDGET (cs_index_new());
                cs_index_set_model(CS_INDEX (priv->index_page), index_list);
                cur_page = gtk_notebook_append_page(GTK_NOTEBOOK (priv->control_notebook),
                                                    priv->index_page,
                                                    gtk_label_new(_("Index")));

                g_signal_connect(G_OBJECT (priv->index_page),
                                 "link-selected",
                                 G_CALLBACK (link_selected_cb),
                                 self);
        } else {
                g_message("CS_BOOK >>> this book dose not include an index");
        }

        /* bookmarks */
        GList *bookmarks_list = cs_chmfile_get_bookmarks_list(model);
        priv->bookmarks_page = GTK_WIDGET (cs_bookmarks_new());
        cs_bookmarks_set_model(CS_BOOKMARKS (priv->bookmarks_page), bookmarks_list);
        cur_page = gtk_notebook_append_page(GTK_NOTEBOOK (priv->control_notebook),
                                            priv->bookmarks_page,
                                            gtk_label_new (_("Bookmarks")));

        g_signal_connect(G_OBJECT (priv->bookmarks_page),
                         "link-selected",
                         G_CALLBACK (link_selected_cb),
                         self);

        if (g_list_length(bookmarks_list) == 0)
                cur_page = 0;

        gtk_notebook_set_current_page(GTK_NOTEBOOK (priv->control_notebook), cur_page);
        gtk_widget_show_all(priv->control_notebook);

        cur_page = new_html_tab(self);
        update_tab_label_state(self);

        gtk_notebook_set_current_page(GTK_NOTEBOOK (priv->html_notebook), cur_page);
        gtk_widget_show_all(priv->html_notebook);

        const gchar *page = cs_chmfile_get_page(model);
        if (page) {
                cs_book_load_url(self, page, TRUE);
        } else {
                cs_book_homepage(self);
        }

        g_signal_emit(self, signals[MODEL_CHANGED], 0, model, NULL);
}

void
cs_book_load_url(CsBook *self, const gchar *uri, gboolean force_reload)
{
        g_debug("CS_BOOK >>> load uri %s", uri);

        g_return_if_fail(IS_CS_BOOK (self));

        if (uri == NULL || strlen(uri) == 0)
                return;

        CsBookPrivate *priv = CS_BOOK_GET_PRIVATE (self);

        /* Concatenate bookfolder and short uri */
        const gchar *pattern = NULL;
        if (uri[0] == '/')
                pattern = "file://%s%s";
        else
                pattern = "file://%s/%s";

        gchar *full_uri = g_strdup_printf(pattern, cs_chmfile_get_bookfolder(priv->model), uri);

        /* Check file exist */
        gchar *real_uri = get_real_uri(full_uri);
        gchar *filename = g_filename_from_uri(real_uri, NULL, NULL);
        gboolean file_exist = g_file_test(filename, G_FILE_TEST_EXISTS);

        if (!file_exist) {
                /* search again with case insensitive name */
                gchar *found = file_exist_ncase(filename);
                if (found != NULL) {
                        g_free(full_uri);

                        full_uri = g_strdup_printf("file://%s%s", found, full_uri+strlen(real_uri));
                        g_free(found);
                        file_exist = TRUE;
                }
        }

        if (file_exist) {
                gchar *location = cs_html_webkit_get_location(priv->active_html);
                if (force_reload || g_strcmp0(full_uri, location)) {
                        /* set user specified charset */
                        const gchar *charset = cs_chmfile_get_charset(CS_CHMFILE (priv->model));
                        if (charset && strlen(charset))
                                cs_html_webkit_set_charset(priv->active_html, charset);

                        g_signal_handlers_block_by_func(priv->active_html, html_open_uri_cb, self);
                        cs_html_webkit_load_url(priv->active_html, full_uri);
                        g_signal_handlers_unblock_by_func(priv->active_html, html_open_uri_cb, self);
                }
                g_free(location);
        } else {
                GtkWidget *msg_dialog;

                msg_dialog = gtk_message_dialog_new(NULL,
                                                    GTK_DIALOG_MODAL,
                                                    GTK_MESSAGE_ERROR,
                                                    GTK_BUTTONS_CLOSE,
                                                    _("Can not find link target file at \"%s\""),
                                                    filename);
                gtk_dialog_run(GTK_DIALOG (msg_dialog));
                gtk_widget_destroy(msg_dialog);

                g_message("CS_BOOK >>> cannot find target file = %s", filename);
        }

        g_free(full_uri);
        g_free(real_uri);
        g_free(filename);
}

void
cs_book_new_tab_with_fulluri(CsBook *self, const gchar *full_uri)
{
        g_debug("CS_BOOK >>> new tab with full url %s", full_uri);

        g_return_if_fail(IS_CS_BOOK (self));

        if (full_uri == NULL || strlen(full_uri) == 0)
                return;

        CsBookPrivate *priv = CS_BOOK_GET_PRIVATE (self);

        char *scheme = g_uri_parse_scheme(full_uri);
        if (scheme && g_strcmp0(scheme, "file")) {
                gchar *message = g_strdup_printf("URI %s with unsupported protocol: %s", full_uri, scheme);
                g_message("%s", message);
                update_book_message(self, message);
                g_free(message);
        } else {
                gtk_notebook_set_current_page(GTK_NOTEBOOK (priv->html_notebook),
                                              new_html_tab(self));
                update_tab_label_state(self);

                cs_book_load_url(self,
                                 get_short_uri(priv->model, full_uri),
                                 TRUE);
        }
        g_free(scheme);
}

gboolean
cs_book_can_close_tab(CsBook *self)
{
        g_return_val_if_fail(IS_CS_BOOK (self), FALSE);

        return (gtk_notebook_get_n_pages(GTK_NOTEBOOK (CS_BOOK_GET_PRIVATE (self)->html_notebook)) > 1);
}

void
cs_book_close_current_tab(CsBook *self)
{
        g_return_if_fail(IS_CS_BOOK (self));

        CsBookPrivate *priv = CS_BOOK_GET_PRIVATE (self);

        if (gtk_notebook_get_n_pages(GTK_NOTEBOOK (priv->html_notebook)) == 1)
                return;

        gint page_num = gtk_notebook_get_current_page(GTK_NOTEBOOK (priv->html_notebook));
        g_debug("CS_BOOK >>> cs_book_close_current_tab current page No = %d", page_num);

        if (page_num >= 0)
                gtk_notebook_remove_page(GTK_NOTEBOOK (priv->html_notebook), page_num);

        update_tab_label_state(self);
}

void
cs_book_reload_current_page(CsBook *self)
{
        g_debug("CS_BOOK >>> Reload current page");
        g_return_if_fail(IS_CS_BOOK (self));

        CsBookPrivate *priv = CS_BOOK_GET_PRIVATE (self);

        if (priv->model != NULL) {
                const gchar *charset = cs_chmfile_get_charset(CS_CHMFILE (priv->model));
                if (charset && strlen(charset))
                        cs_html_webkit_set_charset(priv->active_html, charset);
                cs_html_webkit_reload(priv->active_html);
        }
}

void
cs_book_homepage(CsBook *self)
{
        g_return_if_fail(IS_CS_BOOK (self));

        const gchar *homepage = cs_chmfile_get_homepage(CS_BOOK_GET_PRIVATE (self)->model);

        if (homepage != NULL)
                cs_book_load_url(self, homepage, FALSE);
}

gboolean
cs_book_has_homepage(CsBook *self)
{
        g_return_val_if_fail(IS_CS_BOOK (self), FALSE);

        return cs_chmfile_get_homepage(CS_BOOK_GET_PRIVATE (self)->model) != NULL ? TRUE : FALSE;
}

gboolean
cs_book_can_go_back(CsBook *self)
{
        g_return_val_if_fail(IS_CS_BOOK (self), FALSE);

        return cs_html_webkit_can_go_back(CS_BOOK_GET_PRIVATE (self)->active_html);
}

gboolean
cs_book_can_go_forward(CsBook *self)
{
        g_return_val_if_fail(IS_CS_BOOK (self), FALSE);

        return cs_html_webkit_can_go_forward(CS_BOOK_GET_PRIVATE (self)->active_html);
}

void
cs_book_go_back(CsBook *self)
{
        g_return_if_fail(IS_CS_BOOK (self));

        cs_html_webkit_go_back(CS_BOOK_GET_PRIVATE (self)->active_html);
}

void
cs_book_go_forward(CsBook *self)
{
        g_return_if_fail(IS_CS_BOOK (self));

        cs_html_webkit_go_forward(CS_BOOK_GET_PRIVATE (self)->active_html);
}

void
cs_book_go_prev(CsBook *self)
{
        g_debug("CS_BOOK >>> go prev");
        g_return_if_fail(IS_CS_BOOK (self));

        GList *current = get_toc_current(self);

        if (current && current->prev) {
                gchar *uri = ((Link *)current->prev->data)->uri;
                cs_book_load_url(self, uri, FALSE);
        }
}

void
cs_book_go_next(CsBook *self)
{
        g_debug("CS_BOOK >>> go next");
        g_return_if_fail(IS_CS_BOOK (self));

        GList *current = get_toc_current(self);

        if (current && current->next) {
                gchar *uri = ((Link *)current->next->data)->uri;
                cs_book_load_url(self, uri, FALSE);
        }
}

void
cs_book_zoom_in(CsBook *self)
{
        g_return_if_fail(IS_CS_BOOK (self));

        cs_html_webkit_increase_size(CS_BOOK_GET_PRIVATE (self)->active_html);
}

void
cs_book_zoom_out(CsBook *self)
{
        g_return_if_fail(IS_CS_BOOK (self));

        cs_html_webkit_decrease_size(CS_BOOK_GET_PRIVATE (self)->active_html);
}

void
cs_book_zoom_reset(CsBook *self)
{
        g_return_if_fail(IS_CS_BOOK (self));

        cs_html_webkit_reset_size(CS_BOOK_GET_PRIVATE (self)->active_html);
}

gboolean
cs_book_can_copy(CsBook *self)
{
        g_return_val_if_fail(IS_CS_BOOK (self), FALSE);

        CsBookPrivate *priv = CS_BOOK_GET_PRIVATE (self);

        if (priv->active_html != NULL)
                return cs_html_webkit_can_copy_selection(priv->active_html);
        else
                return FALSE;
}

void
cs_book_copy(CsBook *self)
{
        g_return_if_fail(IS_CS_BOOK (self));

        cs_html_webkit_copy_selection(CS_BOOK_GET_PRIVATE (self)->active_html);
}

void
cs_book_select_all(CsBook *self)
{
        g_return_if_fail(IS_CS_BOOK (self));

        cs_html_webkit_select_all(CS_BOOK_GET_PRIVATE (self)->active_html);
}

gchar *
cs_book_get_location(CsBook *self)
{
        g_return_val_if_fail(IS_CS_BOOK (self), NULL);

        return cs_html_webkit_get_location(CS_BOOK_GET_PRIVATE (self)->active_html);
}

int
cs_book_get_hpaned_position(CsBook *self)
{
        g_return_val_if_fail(IS_CS_BOOK (self), 0);

        return gtk_paned_get_position(GTK_PANED (CS_BOOK_GET_PRIVATE (self)->hpaned));
}

void
cs_book_set_hpaned_position(CsBook *self, gint position)
{
        g_return_if_fail(IS_CS_BOOK (self));

        gtk_paned_set_position(GTK_PANED (CS_BOOK_GET_PRIVATE (self)->hpaned), position);
}

void
cs_book_findbar_show(CsBook *self)
{
        g_return_if_fail(IS_CS_BOOK (self));

        CsBookPrivate *priv = CS_BOOK_GET_PRIVATE (self);

        gtk_widget_show(priv->findbar);
        gtk_widget_grab_focus(g_object_get_data(G_OBJECT (priv->findbar), "find-entry"));
}

void
cs_book_findbar_hide(CsBook *self)
{
        g_return_if_fail(IS_CS_BOOK (self));

        gtk_widget_hide(CS_BOOK_GET_PRIVATE (self)->findbar);
}
