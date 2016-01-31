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

#ifndef __CS_INDEX_H_
#define __CS_INDEX_H_

#include <gtk/gtk.h>

#include "models/link.h"

G_BEGIN_DECLS

#define CS_TYPE_INDEX        (cs_index_get_type())
#define CS_INDEX(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), CS_TYPE_INDEX, CsIndex))
#define CS_INDEX_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST    ((k), CS_TYPE_INDEX, CsIndexClass))
#define IS_CS_INDEX(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), CS_TYPE_INDEX))
#define IS_CS_INDEX_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE    ((k), CS_TYPE_INDEX))

typedef struct _CsIndex      CsIndex;
typedef struct _CsIndexClass CsIndexClass;

struct _CsIndex {
        GtkVBox      vbox;
};

struct _CsIndexClass {
        GtkVBoxClass parent_class;

        /* Signals */
        void (*link_selected) (CsIndex* self, Link *link);
};

GType      cs_index_get_type(void);
GtkWidget *cs_index_new(void);

void       cs_index_set_model(CsIndex *, GList *);
void       cs_index_set_filter_string(CsIndex *, const gchar *);

G_END_DECLS

#endif /* !__CS_INDEX_H_ */
