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

#include "index.h"
#include "treeview.h"
#include "utils.h"
#include "models/link.h"

/* Signals */
enum {
        LINK_SELECTED,
        LAST_SIGNAL
};

typedef struct _CsIndexPrivate CsIndexPrivate;

struct _CsIndexPrivate {
        GtkWidget *treeview;
        GtkEntry  *filter_entry;
};

static gint signals[LAST_SIGNAL] = { 0 };

static void link_selected_cb(CsIndex *, Link *);
static void filter_changed_cb(GtkEntry *, CsIndex *);

#define CS_INDEX_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), CS_TYPE_INDEX, CsIndexPrivate))

G_DEFINE_TYPE (CsIndex, cs_index, GTK_TYPE_VBOX);

static void
cs_index_class_init(CsIndexClass* klass)
{
        g_type_class_add_private(klass, sizeof(CsIndexPrivate));

        signals[LINK_SELECTED] =
                g_signal_new("link-selected",
                             G_TYPE_FROM_CLASS (klass),
                             G_SIGNAL_RUN_LAST,
                             G_STRUCT_OFFSET (CsIndexClass, link_selected),
                             NULL,
                             NULL,
                             g_cclosure_marshal_VOID__POINTER,
                             G_TYPE_NONE,
                             1,
                             G_TYPE_POINTER);
}

static void
cs_index_init(CsIndex* self)
{
        CsIndexPrivate *priv = CS_INDEX_GET_PRIVATE(self);

        priv->filter_entry = GTK_ENTRY (gtk_entry_new());
        gtk_entry_set_max_length(GTK_ENTRY (priv->filter_entry), ENTRY_MAX_LENGTH);
        g_signal_connect(priv->filter_entry,
                         "changed",
                         G_CALLBACK (filter_changed_cb),
                         self);
        gtk_box_pack_start(GTK_BOX (self), GTK_WIDGET (priv->filter_entry), FALSE, FALSE, 0);

        GtkWidget *index_sw = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (index_sw),
                                       GTK_POLICY_NEVER,
                                       GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW (index_sw),
                                            GTK_SHADOW_NONE);

        priv->treeview = cs_tree_view_new(TRUE);
        g_signal_connect_swapped(priv->treeview,
                                 "link-selected",
                                 G_CALLBACK(link_selected_cb),
                                 self);

        gtk_container_add(GTK_CONTAINER (index_sw), priv->treeview);
        gtk_box_pack_start(GTK_BOX (self), index_sw, TRUE, TRUE, 0);

        gtk_widget_show_all(GTK_WIDGET (self));
}

/* Callbacks */

static void
link_selected_cb(CsIndex* self, Link* link)
{
        g_signal_emit(self, signals[LINK_SELECTED], 0, link);
}

static void
filter_changed_cb(GtkEntry *entry, CsIndex *self)
{
        cs_tree_view_set_filter_string(CS_TREE_VIEW (CS_INDEX_GET_PRIVATE (self)->treeview),
                                       gtk_entry_get_text(entry));
}

/* External functions */

GtkWidget *
cs_index_new(void)
{
        g_debug("CS_INDEX >>> create");
        CsIndex* self = CS_INDEX (g_object_new(CS_TYPE_INDEX, NULL));

        return GTK_WIDGET (self);
}

void
cs_index_set_model(CsIndex *self, GList *model)
{
        g_debug("CS_INDEX >>> set model");
        g_return_if_fail(IS_CS_INDEX (self));

        CsIndexPrivate *priv = CS_INDEX_GET_PRIVATE (self);

        gtk_entry_set_text(priv->filter_entry, "");
        cs_tree_view_set_model(CS_TREE_VIEW (priv->treeview), model);
}
