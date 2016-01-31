/*
 *  Copyright (C) 2010 Ji YongGang <jungleji@gmail.com>
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

#include <stdio.h>
#include <string.h>

#include "treeview.h"
#include "utils.h"

/* Signals */
enum {
        LINK_SELECTED,
        LAST_SIGNAL
};

/* TreeView Model */
enum {
        COL_TITLE,
        COL_URI,
        N_COLUMNS
};

typedef struct _CsTreeViewPrivate CsTreeViewPrivate;

struct _CsTreeViewPrivate {
        GtkListStore       *store;
        GtkTreeModel       *filter_model;
        gchar              *filter_string;
};

static gint signals[LAST_SIGNAL] = { 0 };

static void cs_tree_view_init(CsTreeView *);
static void cs_tree_view_class_init(CsTreeViewClass *);
static void cs_tree_view_dispose(GObject *);
static void cs_tree_view_finalize(GObject *);

static void row_activated_cb(CsTreeView *, GtkTreePath *, GtkTreeViewColumn *);
static void selection_changed_cb(GtkTreeSelection *, CsTreeView *);

static gboolean visible_func(GtkTreeModel *, GtkTreeIter *, gpointer);
static void apply_filter_model(CsTreeView *);

#define CS_TREE_VIEW_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), CS_TYPE_TREE_VIEW, CsTreeViewPrivate))

/* GObject functions */

G_DEFINE_TYPE (CsTreeView, cs_tree_view, GTK_TYPE_TREE_VIEW);

static void
cs_tree_view_class_init(CsTreeViewClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        g_type_class_add_private(klass, sizeof(CsTreeViewPrivate));

        object_class->dispose  = cs_tree_view_dispose;
        object_class->finalize = cs_tree_view_finalize;

        signals[LINK_SELECTED] =
                g_signal_new("link-selected",
                             G_TYPE_FROM_CLASS (klass),
                             G_SIGNAL_RUN_LAST,
                             G_STRUCT_OFFSET (CsTreeViewClass, link_selected),
                             NULL,
                             NULL,
                             g_cclosure_marshal_VOID__POINTER,
                             G_TYPE_NONE,
                             1,
                             G_TYPE_POINTER);
}

static void
cs_tree_view_init(CsTreeView *self)
{
        CsTreeViewPrivate *priv = CS_TREE_VIEW_GET_PRIVATE (self);

        priv->store = NULL;
        priv->filter_model = NULL;
        priv->filter_string = NULL;

        GtkCellRenderer *cell = gtk_cell_renderer_text_new();
        g_object_set(cell,
                     "ellipsize", PANGO_ELLIPSIZE_END,
                     NULL);

        gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW (self),
                                                    -1,
                                                    "", cell,
                                                    "text", 0,
                                                    NULL);

        gtk_tree_view_set_headers_visible(GTK_TREE_VIEW (self), FALSE);
        gtk_tree_view_set_enable_search(GTK_TREE_VIEW (self), FALSE);
        gtk_tree_view_set_search_column(GTK_TREE_VIEW (self), FALSE);

        GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (self));
        gtk_tree_selection_set_mode(selection, GTK_SELECTION_BROWSE);
        g_signal_connect(selection,
                         "changed",
                         G_CALLBACK (selection_changed_cb),
                         self);

        g_signal_connect(self,
                         "row_activated",
                         G_CALLBACK (row_activated_cb),
                         NULL);

        gtk_widget_show_all(GTK_WIDGET (self));
}

static void
cs_tree_view_dispose(GObject *object)
{
        g_debug("CS_TREE_VIEW >>> dispose");

        CsTreeViewPrivate *priv = CS_TREE_VIEW_GET_PRIVATE (CS_TREE_VIEW (object));

        if (priv->store) {
                g_object_unref(priv->store);
                priv->store = NULL;
        }

        if (priv->filter_model) {
                g_object_unref(priv->filter_model);
                priv->filter_model = NULL;
        }

        G_OBJECT_CLASS (cs_tree_view_parent_class)->dispose(object);
}

static void
cs_tree_view_finalize(GObject *object)
{
        g_debug("CS_TREE_VIEW >>> finalize");

        CsTreeViewPrivate *priv = CS_TREE_VIEW_GET_PRIVATE (CS_TREE_VIEW (object));

        if (priv->filter_string)
                g_free(priv->filter_string);

        G_OBJECT_CLASS (cs_tree_view_parent_class)->finalize(object);
}

/* Callbacks */

static void
row_activated_cb(CsTreeView *self, GtkTreePath *path, GtkTreeViewColumn *column)
{
        g_debug("CS_TREE_VIEW >>> row_activate callback");

        CsTreeViewPrivate *priv  = CS_TREE_VIEW_GET_PRIVATE (self);
        GtkTreeModel      *model;
        GtkTreeIter        iter;
        gchar             *title, *uri;

        if (priv->filter_model)
                model = priv->filter_model;
        else
                model = GTK_TREE_MODEL (priv->store);

        gtk_tree_model_get_iter(model, &iter, path);
        gtk_tree_model_get(model,
                           &iter,
                           COL_TITLE, &title,
                           COL_URI, &uri,
                           -1);

        Link *link = link_new(LINK_TYPE_PAGE, title, uri);
        g_debug("CS_TREE_VIEW >>> row activated, link: name = %s uri = %s\n", link->name, link->uri);
        g_signal_emit(self, signals[LINK_SELECTED], 0, link);
}

static void
selection_changed_cb(GtkTreeSelection *selection, CsTreeView *self)
{
        g_debug("CS_TREE_VIEW >>> selection changed");
}

/* Internal functions */

static gboolean
visible_func(GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
        CsTreeViewPrivate *priv = CS_TREE_VIEW_GET_PRIVATE (CS_TREE_VIEW (data));

        if (priv->filter_string == NULL || strlen(priv->filter_string) == 0)
                return TRUE;

        gchar *text = NULL;
        gboolean visible = FALSE;

        gtk_tree_model_get(model, iter, COL_TITLE, &text, -1);

        if (text != NULL) {
                gchar *normalized_string = g_utf8_normalize(text, -1, G_NORMALIZE_ALL);
                gchar *case_normalized_string = g_utf8_casefold(normalized_string, -1);

                if (!strncasecmp(priv->filter_string, case_normalized_string, strlen(priv->filter_string)))
                        visible = TRUE;

                g_free(normalized_string);
                g_free(case_normalized_string);
        }

        return visible;
}

static void
apply_filter_model(CsTreeView *self)
{
        g_debug("CS_TREEVIEW >>> apply filter model");
        CsTreeViewPrivate *priv = CS_TREE_VIEW_GET_PRIVATE (self);

        priv->filter_model = gtk_tree_model_filter_new(GTK_TREE_MODEL (priv->store), NULL);

        gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER (priv->filter_model),
                                               visible_func,
                                               self,
                                               NULL);
        gtk_tree_view_set_model(GTK_TREE_VIEW (self), priv->filter_model);
}


/* External functions */

GtkWidget *
cs_tree_view_new(gboolean with_filter)
{
        g_debug("CS_TREE_VIEW >>> create");
        CsTreeView *self = g_object_new(CS_TYPE_TREE_VIEW, NULL);
        CsTreeViewPrivate *priv = CS_TREE_VIEW_GET_PRIVATE (self);

        priv->store = gtk_list_store_new(N_COLUMNS,
                                         G_TYPE_STRING,
                                         G_TYPE_STRING);
        if (with_filter)
                apply_filter_model(self);
        else
                gtk_tree_view_set_model(GTK_TREE_VIEW (self),
                                        GTK_TREE_MODEL (priv->store));

        return GTK_WIDGET (self);
}

void
cs_tree_view_set_model(CsTreeView *self, GList *model)
{
        g_debug("CS_TREEVIEW >>> set model");
        g_return_if_fail(IS_CS_TREE_VIEW (self));

        CsTreeViewPrivate *priv = CS_TREE_VIEW_GET_PRIVATE (self);
        GtkTreeIter        iter;
        GList             *l;

        if (priv->store)
                gtk_list_store_clear(priv->store);

        for (l = model; l; l = l->next) {
                Link *link = l->data;

                gtk_list_store_append(priv->store, &iter);
                gtk_list_store_set(priv->store, &iter,
                                   COL_TITLE, link->name,
                                   COL_URI, link->uri,
                                   -1);
        }
}

void
cs_tree_view_add_link(CsTreeView *self, Link *link)
{
        g_debug("CS_TREEVIEW >>> add link = %p", link);
        g_return_if_fail(IS_CS_TREE_VIEW (self));

        GtkTreeIter        iter;
        CsTreeViewPrivate *priv = CS_TREE_VIEW_GET_PRIVATE (self);

        gtk_list_store_append(priv->store, &iter);
        gtk_list_store_set(priv->store, &iter,
                           COL_TITLE, link->name,
                           COL_URI, link->uri,
                           -1);
}

void
cs_tree_view_remove_link(CsTreeView *self, Link *link)
{
        g_return_if_fail(IS_CS_TREE_VIEW (self));

        CsTreeViewPrivate *priv = CS_TREE_VIEW_GET_PRIVATE (self);

        GtkTreeIter iter;
        gchar      *uri;

        gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL (priv->store), &iter, "0");

        do {
                gtk_tree_model_get(GTK_TREE_MODEL (priv->store), &iter, COL_URI, &uri, -1);

                if (ncase_compare_utf8_string(link->uri, uri) == 0) {
                        gtk_list_store_remove(priv->store, &iter);
                        break;
                }
        } while (gtk_tree_model_iter_next(GTK_TREE_MODEL (priv->store), &iter));
}

Link *
cs_tree_view_get_selected_link(CsTreeView *self)
{
        g_return_val_if_fail(IS_CS_TREE_VIEW (self), NULL);

        CsTreeViewPrivate *priv = CS_TREE_VIEW_GET_PRIVATE (self);
        GtkTreeSelection  *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (self));

        GtkTreeIter iter;
        Link       *link = NULL;

        if (gtk_tree_selection_get_selected(selection, NULL, &iter)) {
                gchar *title, *uri;

                gtk_tree_model_get(GTK_TREE_MODEL (priv->store),
                                   &iter,
                                   COL_TITLE, &title,
                                   COL_URI, &uri,
                                   -1);

                link = link_new(LINK_TYPE_PAGE, title, uri);
        }

        return link;
}

void
cs_tree_view_select_link(CsTreeView *self, Link *link)
{
        g_return_if_fail(IS_CS_TREE_VIEW (self));

        CsTreeViewPrivate *priv = CS_TREE_VIEW_GET_PRIVATE (self);
        GtkTreeSelection  *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (self));

        GtkTreeIter        iter;
        gchar             *uri;

        gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL (priv->store), &iter, "0");

        do {
                gtk_tree_model_get(GTK_TREE_MODEL (priv->store), &iter, COL_URI, &uri, -1);

                if (ncase_compare_utf8_string(link->uri, uri) == 0) {
                        gtk_tree_selection_select_iter(selection, &iter);
                        break;
                }
        } while (gtk_tree_model_iter_next(GTK_TREE_MODEL (priv->store), &iter));
}

void
cs_tree_view_set_filter_string(CsTreeView *self, const gchar *string)
{
        g_debug("CS_TREEVIEW >>> set filter string = %s", string);
        g_return_if_fail(IS_CS_TREE_VIEW (self));

        CsTreeViewPrivate *priv = CS_TREE_VIEW_GET_PRIVATE (self);

        if (!priv->filter_model)
                return;

        if (priv->filter_string) {
                g_free(priv->filter_string);
                priv->filter_string = NULL;
        }

        priv->filter_string = g_strdup(string);
        gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER (priv->filter_model));
}
