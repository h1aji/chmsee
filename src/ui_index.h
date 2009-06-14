/*
 *  Copyright (c) 2009 LI Daobing <lidaobing@gmail.com>
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

#ifndef _CHMSEE_UI_INDEX_H_
#define _CHMSEE_UI_INDEX_H_

#include <gtk/gtk.h>
#include "models/chmindex.h"

#define CHMSEE_TYPE_UI_INDEX (chmsee_ui_index_get_type())
#define CHMSEE_UI_INDEX(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), CHMSEE_TYPE_UI_INDEX, ChmseeUiIndex))
#define CHMSEE_IS_UI_INDEX(obj) (G_TYPE_CHECK_INSTANCE_TYPR((obj), CHMSEE_TYPE_UI_INDEX))

typedef struct _ChmseeUiIndex ChmseeUiIndex;
typedef struct _ChmseeUiIndexClass ChmseeUiIndexClass;
typedef struct _ChmseeUiIndexPrivate ChmseeUiIndexPrivate;

struct _ChmseeUiIndex {
	GtkViewport parent_instance;
	ChmseeUiIndexPrivate* priv;
};

struct _ChmseeUiIndexClass {
	GtkViewportClass parent_class;
};

GType chmsee_ui_index_get_type(void);
GtkWidget* chmsee_ui_index_new(ChmIndex* chmIndex);
void chmsee_ui_index_set_model(ChmseeUiIndex* self, ChmIndex* chmIndex);
void chmsee_ui_index_refresh(ChmseeUiIndex* self);


#endif /* UI_INDEX_H_ */
