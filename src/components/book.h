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

#ifndef __CS_BOOK_H__
#define __CS_BOOK_H__

#include <glib-object.h>
#include <gtk/gtk.h>

#include "models/chmfile.h"

G_BEGIN_DECLS

#define CS_TYPE_BOOK        (cs_book_get_type ())
#define CS_BOOK(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), CS_TYPE_BOOK, CsBook))
#define CS_BOOK_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST    ((k), CS_TYPE_BOOK, CsBookClass))
#define IS_CS_BOOK(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), CS_TYPE_BOOK))
#define IS_CS_BOOK_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE    ((k), CS_TYPE_BOOK))

typedef struct _CsBook			CsBook;
typedef struct _CsBookClass		CsBookClass;

struct _CsBook {
        GtkVBox      vbox;
};

struct _CsBookClass {
        GtkVBoxClass parent_class;
};

GType        cs_book_get_type(void);

GtkWidget   *cs_book_new(void);
void         cs_book_set_model(CsBook *, CsChmfile *);

void         cs_book_new_tab_with_fulluri(CsBook *, const gchar *);
void         cs_book_load_url(CsBook *, const gchar *, gboolean);
gboolean     cs_book_can_close_tab(CsBook *);
void         cs_book_close_current_tab(CsBook *);
void         cs_book_reload_current_page(CsBook *);

gboolean     cs_book_has_homepage(CsBook *);
void         cs_book_homepage(CsBook *);
gboolean     cs_book_can_go_back(CsBook *);
void         cs_book_go_back(CsBook *);
gboolean     cs_book_can_go_forward(CsBook *);
void         cs_book_go_forward(CsBook *);
void         cs_book_go_prev(CsBook *);
void         cs_book_go_next(CsBook *);

void         cs_book_zoom_in(CsBook *);
void         cs_book_zoom_out(CsBook *);
void         cs_book_zoom_reset(CsBook *);

gboolean     cs_book_can_copy(CsBook *);
void         cs_book_copy(CsBook *);
void         cs_book_select_all(CsBook *);

gchar       *cs_book_get_location(CsBook *);
int          cs_book_get_hpaned_position(CsBook *);
void         cs_book_set_hpaned_position(CsBook *, gint);

void         cs_book_findbar_show(CsBook *);
void         cs_book_findbar_hide(CsBook *);

G_END_DECLS

#endif /* !__CS_BOOK_H__ */
