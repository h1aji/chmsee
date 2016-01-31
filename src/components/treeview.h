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

#ifndef __CS_TREE_VIEW_H__
#define __CS_TREE_VIEW_H__

#include <gtk/gtk.h>

#include "models/link.h"

G_BEGIN_DECLS

#define CS_TYPE_TREE_VIEW        (cs_tree_view_get_type())
#define CS_TREE_VIEW(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), CS_TYPE_TREE_VIEW, CsTreeView))
#define CS_TREE_VIEW_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST    ((k), CS_TYPE_TREE_VIEW, CsTreeViewClass))
#define IS_CS_TREE_VIEW(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), CS_TYPE_TREE_VIEW))
#define IS_CS_TREE_VIEW_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE    ((k), CS_TYPE_TREE_VIEW))

typedef struct _CsTreeView       CsTreeView;
typedef struct _CsTreeViewClass  CsTreeViewClass;

struct _CsTreeView {
        GtkTreeView treeview;
};

struct _CsTreeViewClass {
        GtkTreeViewClass   parent_class;

        /* Signals */
        void (*link_selected) (CsTreeView *self, Link *link);
};

GType      cs_tree_view_get_type(void);
GtkWidget *cs_tree_view_new(gboolean);

void       cs_tree_view_set_model(CsTreeView *, GList *);
void       cs_tree_view_set_filter_string(CsTreeView *, const gchar *);

void       cs_tree_view_add_link(CsTreeView *, Link *);
void       cs_tree_view_remove_link(CsTreeView *, Link *);
void       cs_tree_view_select_link(CsTreeView *, Link *);
Link      *cs_tree_view_get_selected_link(CsTreeView *);

G_END_DECLS

#endif /* !__CS_TREE_VIEW_H__ */
