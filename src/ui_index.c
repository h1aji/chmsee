/*
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

#include "ui_index.h"
#include "config.h"

#include "models/link.h"

#define selfp (self->priv)

enum {
        COL_TITLE,
        COL_URI,
        N_COLUMNS
};

struct _ChmseeUiIndexPrivate {
	ChmIndex* chmIndex;
	GtkListStore* store;
};

static GtkTreeViewClass* parent_class;

#define CHMSEE_UI_INDEX_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), CHMSEE_TYPE_UI_INDEX, ChmseeUiIndexPrivate))

G_DEFINE_TYPE(ChmseeUiIndex, chmsee_ui_index, GTK_TYPE_TREE_VIEW);

static void chmsee_ui_index_dispose(GObject* object);
static void chmsee_ui_index_finalize(GObject* object);


static void
chmsee_ui_index_class_init(ChmseeUiIndexClass* klass) {
	g_type_class_add_private(klass, sizeof(ChmseeUiIndexPrivate));
	G_OBJECT_CLASS(klass)->dispose = chmsee_ui_index_dispose;
	G_OBJECT_CLASS(klass)->finalize = chmsee_ui_index_finalize;

	parent_class = g_type_class_peek_parent(klass);
}

static void
chmsee_ui_index_init(ChmseeUiIndex* self) {
	self->priv = CHMSEE_UI_INDEX_GET_PRIVATE(self);
	selfp->chmIndex = NULL;
	selfp->store = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(self), GTK_TREE_MODEL(selfp->store));
	GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer,
			"ellipsize", PANGO_ELLIPSIZE_END,
	        NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW (self),
                                                -1,
                                                "Index Title", renderer,
                                                "text", COL_TITLE,
                                                NULL);
}

GtkWidget* chmsee_ui_index_new(ChmIndex* chmIndex) {
	ChmseeUiIndex* self = CHMSEE_UI_INDEX(g_object_new(CHMSEE_TYPE_UI_INDEX, NULL));
	chmsee_ui_index_set_model(self, chmIndex);
	return GTK_WIDGET(self);
}

static void chmsee_ui_index_dispose(GObject* object) {
	ChmseeUiIndex* self = CHMSEE_UI_INDEX(object);
	if(selfp->chmIndex) {
		g_object_unref(selfp->chmIndex);
		selfp->chmIndex = NULL;
	}
	if(selfp->store) {
		g_object_unref(selfp->store);
		selfp->store = NULL;
	}
	G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void chmsee_ui_index_finalize(GObject* object) {
	G_OBJECT_CLASS(parent_class)->finalize(object);
}

void chmsee_ui_index_set_model(ChmseeUiIndex* self, ChmIndex* chmIndex) {
	if(selfp->chmIndex) {
		g_object_unref(selfp->chmIndex);
		selfp->chmIndex = NULL;
	}

	if(chmIndex != NULL) {
		selfp->chmIndex = g_object_ref(chmIndex);
	}

	chmsee_ui_index_refresh(self);
}


void chmsee_ui_index_refresh(ChmseeUiIndex* self) {
	gtk_list_store_clear(selfp->store);
	if(selfp->chmIndex == NULL) {
		return;
	}

	GList* data = chmindex_get_data(selfp->chmIndex);

	GtkTreeIter iter;
	for(; data; data = data->next) {
		Link* link = data->data;
		gtk_list_store_append(selfp->store, &iter);
		gtk_list_store_set(selfp->store, &iter,
				COL_TITLE, link->name,
				COL_URI, link->uri,
				-1);
	}
}


