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

#ifndef __CS_BOOKMARKS_H__
#define __CS_BOOKMARKS_H__

#include <glib-object.h>
#include <gtk/gtk.h>

#include "models/link.h"

G_BEGIN_DECLS

#define CS_TYPE_BOOKMARKS        (cs_bookmarks_get_type())
#define CS_BOOKMARKS(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), CS_TYPE_BOOKMARKS, CsBookmarks))
#define CS_BOOKMARKS_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST    ((k), CS_TYPE_BOOKMARKS, CsBookmarksClass))
#define IS_CS_BOOKMARKS(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), CS_TYPE_BOOKMARKS))
#define IS_CS_BOOKMARKS_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE    ((k), CS_TYPE_BOOKMARKS))

typedef struct _CsBookmarks       CsBookmarks;
typedef struct _CsBookmarksClass  CsBookmarksClass;

struct _CsBookmarks {
        GtkVBox        vbox;
};

struct _CsBookmarksClass {
        GtkVBoxClass   parent_class;

        /* Signals */
        void (*link_selected) (CsBookmarks *self, Link *link);
};

GType      cs_bookmarks_get_type(void);
GtkWidget *cs_bookmarks_new(void);

void       cs_bookmarks_set_model(CsBookmarks *, GList *);
GList     *cs_bookmarks_get_model(CsBookmarks *);
void       cs_bookmarks_set_current_link(CsBookmarks *, const Link *);
void       cs_bookmarks_grab_focus(CsBookmarks *);

G_END_DECLS

#endif /* !__CS_BOOKMARKS_H__ */
