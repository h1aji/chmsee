/*
 *  Copyright (c) 2006           Ji YongGang <jungle@soforge-studio.com>
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

#ifndef __CHMSEE_UI_CHMFILE_H__
#define __CHMSEE_UI_CHMFILE_H__

#include <glib-object.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtk.h>

#include "models/ichmfile.h"
#include "ihtml.h"

G_BEGIN_DECLS

typedef struct _ChmseeUiChmfile			ChmseeUiChmfile;
typedef struct _ChmseeUiChmfilePrivate		ChmseeUiChmfilePrivate;
typedef struct _ChmseeUiChmfileClass		ChmseeUiChmfileClass;

#define CHMSEE_TYPE_UI_CHMFILE \
        (chmsee_ui_chmfile_get_type ())
#define CHMSEE_UI_CHMFILE(o) \
        (G_TYPE_CHECK_INSTANCE_CAST ((o), CHMSEE_TYPE_UI_CHMFILE, ChmseeUiChmfile))
#define CHMSEE_IS_UI_CHMFILE(o) \
        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CHMSEE_TYPE_UI_CHMFILE))


struct _ChmseeUiChmfile {
	GtkHPaned parent;
	ChmseeUiChmfilePrivate* priv;
};

struct _ChmseeUiChmfileClass {
	GtkHPanedClass parent_class;
};

GType chmsee_ui_chmfile_get_type(void);
GtkWidget* chmsee_ui_chmfile_new(void);
void chmsee_ui_chmfile_set_model(ChmseeUiChmfile* self, ChmseeIchmfile* model);
ChmseeIhtml* chmsee_ui_chmfile_get_active_html(ChmseeUiChmfile* self);
gboolean chmsee_ui_chmfile_jump_index_by_name(ChmseeUiChmfile* self, const gchar* name);
void chmsee_ui_chmfile_new_tab(ChmseeUiChmfile *self, const gchar *location);
void chmsee_ui_chmfile_close_current_tab(ChmseeUiChmfile* self);

G_END_DECLS

#endif /* !__CHMSEE_UI_CHMFILE_H__ */
