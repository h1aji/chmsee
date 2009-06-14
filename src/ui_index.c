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

#include "booktree.h"
#include "models/link.h"

#define selfp (self->priv)

/* Signals */
enum {
        LINK_SELECTED,
        LAST_SIGNAL
};

static gint              signals[LAST_SIGNAL] = { 0 };

struct _ChmseeUiIndexPrivate {
	ChmIndex* chmIndex;
};

static GtkViewportClass* parent_class;

#define CHMSEE_UI_INDEX_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), CHMSEE_TYPE_UI_INDEX, ChmseeUiIndexPrivate))

G_DEFINE_TYPE(ChmseeUiIndex, chmsee_ui_index, GTK_TYPE_VIEWPORT);

static void chmsee_ui_index_dispose(GObject* object);
static void chmsee_ui_index_finalize(GObject* object);
static void chmsee_ui_index_on_link_selected(ChmseeUiIndex* self, Link* link);

static void
chmsee_ui_index_class_init(ChmseeUiIndexClass* klass) {
	g_type_class_add_private(klass, sizeof(ChmseeUiIndexPrivate));
	G_OBJECT_CLASS(klass)->dispose = chmsee_ui_index_dispose;
	G_OBJECT_CLASS(klass)->finalize = chmsee_ui_index_finalize;

	parent_class = g_type_class_peek_parent(klass);

    signals[LINK_SELECTED] =
            g_signal_new ("link_selected",
                          G_TYPE_FROM_CLASS (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (ChmseeUiIndexClass, link_selected),
                          NULL,
                          NULL,
                          g_cclosure_marshal_VOID__POINTER,
                          G_TYPE_NONE,
                          1,
                          G_TYPE_POINTER);
}

static void
chmsee_ui_index_init(ChmseeUiIndex* self) {
	self->priv = CHMSEE_UI_INDEX_GET_PRIVATE(self);
	selfp->chmIndex = NULL;
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
	GtkWidget* widget = NULL;
	if(selfp->chmIndex != NULL) {
		GNode* node = chmindex_get_data(selfp->chmIndex);
		if(node != NULL) {
			widget = booktree_new(node);
		}
	}

	GtkWidget* child = gtk_bin_get_child(GTK_BIN(self));
	if(child != NULL) {
		gtk_widget_destroy(child);
	}

	if(widget != NULL) {
		gtk_container_add(GTK_CONTAINER(self), widget);
		g_signal_connect_swapped(widget, "link_selected", (GCallback)chmsee_ui_index_on_link_selected, self);
	} else {
		gtk_container_add(GTK_CONTAINER(self), gtk_tree_view_new());
	}
}

void chmsee_ui_index_on_link_selected(ChmseeUiIndex* self, Link* link) {
	g_signal_emit(self, signals[LINK_SELECTED], 0, link);
}

