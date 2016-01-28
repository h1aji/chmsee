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

#include "config.h"

#include <string.h>

#include "toc.h"
#include "models/parser.h"
#include "utils.h"

typedef struct {
        GdkPixbuf *pixbuf_opened;
        GdkPixbuf *pixbuf_closed;
        GdkPixbuf *pixbuf_doc;
} TocPixbufs;

typedef struct {
        const gchar *uri;
        gboolean     found;
        GtkTreeIter  iter;
        GtkTreePath *path;
} FindURIData;

typedef struct _CsTocPrivate CsTocPrivate;

struct _CsTocPrivate {
        GtkTreeView        *treeview;
        GtkTreeStore       *store;
        TocPixbufs         *pixbufs;
};

/* Signals */
enum {
        LINK_SELECTED,
        LAST_SIGNAL
};

enum {
        COL_OPEN_PIXBUF,
        COL_CLOSED_PIXBUF,
        COL_TITLE,
        COL_LINK,
        N_COLUMNS
};

static gint signals[LAST_SIGNAL] = { 0 };

static void cs_toc_class_init(CsTocClass *);
static void cs_toc_init(CsToc *);

static void cs_toc_dispose(GObject *);
static void cs_toc_finalize(GObject *);

static void row_activated_cb(GtkTreeView *, GtkTreePath *, GtkTreeViewColumn *);
static void cursor_changed_cb(GtkTreeView *, CsToc *);

static TocPixbufs *create_pixbufs(void);
static GtkTreeViewColumn *create_columns(void);
static void insert_node(CsToc *, GNode *, GtkTreeIter *);

static gboolean find_uri_foreach(GtkTreeModel *, GtkTreePath *, GtkTreeIter *, FindURIData *);

#define CS_TOC_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), CS_TYPE_TOC, CsTocPrivate))

/* GObject functions */

G_DEFINE_TYPE (CsToc, cs_toc, GTK_TYPE_VBOX);

static void
cs_toc_class_init(CsTocClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        g_type_class_add_private(klass, sizeof(CsTocPrivate));

        object_class->dispose  = cs_toc_dispose;
        object_class->finalize = cs_toc_finalize;

        signals[LINK_SELECTED] =
                g_signal_new ("link-selected",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (CsTocClass, link_selected),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__POINTER,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_POINTER);
}

static void
cs_toc_init(CsToc *self)
{
        CsTocPrivate *priv = CS_TOC_GET_PRIVATE (self);

        GtkWidget  *toc_sw = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (toc_sw),
                                       GTK_POLICY_NEVER,
                                       GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW (toc_sw),
                                            GTK_SHADOW_NONE);

        priv->treeview = GTK_TREE_VIEW (gtk_tree_view_new());

        priv->store = gtk_tree_store_new(N_COLUMNS,
                                         GDK_TYPE_PIXBUF,
                                         GDK_TYPE_PIXBUF,
                                         G_TYPE_STRING,
                                         G_TYPE_POINTER);

        gtk_tree_view_set_model(GTK_TREE_VIEW (priv->treeview),
                                GTK_TREE_MODEL (priv->store));

        gtk_tree_view_set_headers_visible(GTK_TREE_VIEW (priv->treeview), FALSE);
        gtk_tree_view_set_enable_search(GTK_TREE_VIEW (priv->treeview), FALSE);

        priv->pixbufs = create_pixbufs();
        gtk_tree_view_append_column(GTK_TREE_VIEW (priv->treeview), create_columns());

        gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW (priv->treeview)),
                                    GTK_SELECTION_BROWSE);

        g_signal_connect(G_OBJECT (priv->treeview),
                         "row-activated",
                         G_CALLBACK(row_activated_cb),
                         NULL);
        g_signal_connect(G_OBJECT (priv->treeview),
                         "cursor-changed",
                         G_CALLBACK(cursor_changed_cb),
                         self);

        gtk_container_add(GTK_CONTAINER (toc_sw), GTK_WIDGET (priv->treeview));
        gtk_box_pack_start(GTK_BOX (self), toc_sw, TRUE, TRUE, 0);

        gtk_widget_show_all(GTK_WIDGET (self));
}

static void
cs_toc_dispose(GObject* object)
{
        g_debug("CS_TOC >>> dispose");
        CsToc        *self = CS_TOC (object);
        CsTocPrivate *priv = CS_TOC_GET_PRIVATE (self);

        if (priv->store) {
                g_object_unref(priv->store);
                g_object_unref(priv->pixbufs->pixbuf_opened);
                g_object_unref(priv->pixbufs->pixbuf_closed);
                g_object_unref(priv->pixbufs->pixbuf_doc);
                priv->store = NULL;
        }

        G_OBJECT_CLASS (cs_toc_parent_class)->dispose(object);
}

static void
cs_toc_finalize(GObject *object)
{
        g_debug("CS_TOC >>> finalize");
        CsTocPrivate *priv = CS_TOC_GET_PRIVATE (CS_TOC (object));

        g_slice_free(TocPixbufs, priv->pixbufs);

        G_OBJECT_CLASS (cs_toc_parent_class)->finalize(object);
}

/* Callbacks */

static void
row_activated_cb(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column)
{
        if (gtk_tree_view_row_expanded(treeview, path))
                gtk_tree_view_collapse_row(treeview, path);
        else
                gtk_tree_view_expand_row(treeview, path, FALSE);
}

static void
cursor_changed_cb(GtkTreeView *treeview, CsToc *self)
{
        g_debug("CS_TOC >>> cursor changed callback");
        CsTocPrivate *priv = CS_TOC_GET_PRIVATE (self);

        GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (priv->treeview));

        GtkTreeIter iter;
        if (gtk_tree_selection_get_selected(selection, NULL, &iter)) {
                Link *link;
                gtk_tree_model_get(GTK_TREE_MODEL (priv->store),
                                   &iter, COL_LINK, &link, -1);

                g_debug("CS_TOC >>> emiting link-selected signal '%s'", link->uri);
                g_signal_emit(self, signals[LINK_SELECTED], 0, link);
        }
}

/* Internal functions */

static TocPixbufs *
create_pixbufs(void)
{
        TocPixbufs *pixbufs;

        pixbufs = g_slice_new(TocPixbufs);

        pixbufs->pixbuf_closed = gdk_pixbuf_new_from_file(RESOURCE_FILE ("book-closed.png"), NULL);
        pixbufs->pixbuf_opened = gdk_pixbuf_new_from_file(RESOURCE_FILE ("book-open.png"), NULL);
        pixbufs->pixbuf_doc    = gdk_pixbuf_new_from_file(RESOURCE_FILE ("helpdoc.png"), NULL);

        return pixbufs;
}

static GtkTreeViewColumn *
create_columns(void)
{
        GtkTreeViewColumn *column = gtk_tree_view_column_new();
        GtkCellRenderer *cell = gtk_cell_renderer_pixbuf_new();

        gtk_tree_view_column_pack_start(column, cell, FALSE);
        gtk_tree_view_column_set_attributes(
                column,
                cell,
                "pixbuf", COL_OPEN_PIXBUF,
                "pixbuf-expander-open", COL_OPEN_PIXBUF,
                "pixbuf-expander-closed", COL_CLOSED_PIXBUF,
                NULL);

        cell = gtk_cell_renderer_text_new();
        g_object_set(cell,
                     "ellipsize", PANGO_ELLIPSIZE_END,
                     NULL);
        gtk_tree_view_column_pack_start(column, cell, TRUE);
        gtk_tree_view_column_set_attributes(column,
                                            cell,
                                            "text", COL_TITLE,
                                            NULL);

        return column;
}

static void
insert_node(CsToc *self, GNode *node, GtkTreeIter *parent_iter)
{
        CsTocPrivate *priv = CS_TOC_GET_PRIVATE (self);

        Link *link = node->data;

        if (g_node_n_children(node))
                link_change_type(link, LINK_TYPE_BOOK);

        GtkTreeIter iter;
        gtk_tree_store_append(priv->store, &iter, parent_iter);

        /* g_debug("CS_TOC >>> insert node::name = %s", link->name); */
        /* g_debug("CS_TOC >>> insert node::uri = %s", link->uri); */

        if (link->type == LINK_TYPE_BOOK) {
                gtk_tree_store_set(priv->store, &iter,
                                   COL_OPEN_PIXBUF, priv->pixbufs->pixbuf_opened,
                                   COL_CLOSED_PIXBUF, priv->pixbufs->pixbuf_closed,
                                   COL_TITLE, link->name,
                                   COL_LINK, link,
                                   -1);
        } else {
                gtk_tree_store_set(priv->store, &iter,
                                   COL_OPEN_PIXBUF, priv->pixbufs->pixbuf_doc,
                                   COL_CLOSED_PIXBUF, priv->pixbufs->pixbuf_doc,
                                   COL_TITLE, link->name,
                                   COL_LINK, link,
                                   -1);
        }

        GNode *child;
        for (child = g_node_first_child(node);
             child;
             child = g_node_next_sibling(child)) {
                insert_node(self, child, &iter);
        }
}

static gboolean
find_uri_foreach(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, FindURIData *data)
{
        Link *link;
        gtk_tree_model_get(model, iter, COL_LINK, &link, -1);

        if (!ncase_compare_utf8_string(data->uri, link->uri)) {
                /* g_debug("CS_TOC >>> found data->uri: %s", data->uri); */
                /* g_debug("CS_TOC >>> found link->uri: %s", link->uri); */

                data->found = TRUE;
                data->iter = *iter;
                data->path = gtk_tree_path_copy(path);
        }

        return data->found;
}

/* External functions */

GtkWidget *
cs_toc_new(void)
{
        g_debug("CS_TOC >>> create");
        CsToc *self = g_object_new(CS_TYPE_TOC, NULL);

        return GTK_WIDGET (self);
}

void
cs_toc_set_model(CsToc *self, GNode *model)
{
        g_return_if_fail(IS_CS_TOC (self));

        gtk_tree_store_clear(CS_TOC_GET_PRIVATE (self)->store);

        GNode *node;
        for (node = g_node_first_child(model);
             node;
             node = g_node_next_sibling(node)) {
                insert_node(self, node, NULL);
        }
}

void
cs_toc_sync(CsToc *self, const gchar *uri)
{
        g_debug("CS_TOC >>> sync uri %s", uri);
        g_return_if_fail(IS_CS_TOC (self));

        CsTocPrivate *priv = CS_TOC_GET_PRIVATE (self);

        FindURIData data;
        data.found = FALSE;
        if (uri[0] == '/')
                data.uri = uri + 1;
        else
                data.uri = uri;

        gtk_tree_model_foreach(GTK_TREE_MODEL (priv->store),
                               (GtkTreeModelForeachFunc) find_uri_foreach,
                               &data);

        if (!data.found) {
                gchar *real_uri = get_real_uri(uri);
                data.uri = real_uri;
                gtk_tree_model_foreach(GTK_TREE_MODEL (priv->store),
                                       (GtkTreeModelForeachFunc) find_uri_foreach,
                                       &data);
                g_free(real_uri);
        }

        if (!data.found) {
                g_debug("CS_TOC >>> sync: cannot find link uri");
                return;
        }

        g_signal_handlers_block_by_func(priv->treeview,
                                        cursor_changed_cb,
                                        self);

        gtk_tree_view_expand_to_path(GTK_TREE_VIEW (priv->treeview), data.path);
        gtk_tree_view_set_cursor(GTK_TREE_VIEW (priv->treeview), data.path, NULL, 0);

        g_signal_handlers_unblock_by_func(priv->treeview,
                                          cursor_changed_cb,
                                          self);

        gtk_tree_path_free(data.path);
}
