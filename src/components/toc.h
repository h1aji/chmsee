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

#ifndef __CS_TOC_H__
#define __CS_TOC_H__

#include <gtk/gtk.h>

#include "models/link.h"

G_BEGIN_DECLS

#define CS_TYPE_TOC        (cs_toc_get_type())
#define CS_TOC(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), CS_TYPE_TOC, CsToc))
#define CS_TOC_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST    ((k), CS_TYPE_TOC, CsTocClass))
#define IS_CS_TOC(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), CS_TYPE_TOC))
#define IS_CS_TOC_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE    ((o), CS_TYPE_TOC))

typedef struct _CsToc       CsToc;
typedef struct _CsTocClass  CsTocClass;

struct _CsToc {
        GtkVBox vbox;
};

struct _CsTocClass {
        GtkVBoxClass parent_class;

        /* Signals */
        void (*link_selected) (CsToc *toc, Link *link);
};

GType        cs_toc_get_type(void);
GtkWidget   *cs_toc_new(void);
void         cs_toc_set_model(CsToc *, GNode *);

void         cs_toc_sync(CsToc *, const gchar *);
const gchar *cs_toc_get_selected_book_title(CsToc *);

G_END_DECLS

#endif /* !__CS_TOC_H__ */
