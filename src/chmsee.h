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

/***************************************************************************
 *   Copyright (C) 2003 by zhong                                           *
 *   zhongz@163.com                                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef __CHMSEE_H__
#define __CHMSEE_H__

#include <glib-object.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _Chmsee      Chmsee;
typedef struct _ChmseeClass ChmseeClass;

#define CHMSEE_TYPE         (chmsee_get_type ())
#define CHMSEE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CHMSEE_TYPE, Chmsee))
#define CHMSEE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST    ((k), CHMSEE_TYPE, ChmseeClass))
#define CHMSEE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS  ((o), CHMSEE_TYPE, ChmseeClass))
#define IS_CHMSEE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CHMSEE_TYPE))
#define IS_CHMSEE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE    ((k), CHMSEE_TYPE))

struct _Chmsee {
        GtkWindow window;
};

struct _ChmseeClass {
        GtkWindowClass parent_class;
};

typedef struct _CsConfig CsConfig;

struct _CsConfig {
        gchar   *home;
        gchar   *bookshelf;
        gchar   *last_file;
        gchar   *charset;
        gchar   *variable_font;
        gchar   *fixed_font;

        gint     pos_x;
        gint     pos_y;
        gint     height;
        gint     width;
        gint     hpaned_pos;
        gboolean fullscreen;
        gboolean startup_lastfile;
};

GType        chmsee_get_type(void);
Chmsee      *chmsee_new(CsConfig *);
void         chmsee_open_file(Chmsee *, const gchar *);
void         chmsee_close_book(Chmsee *);

const gchar *chmsee_get_variable_font(Chmsee *);
void         chmsee_set_variable_font(Chmsee *, const gchar *);

const gchar *chmsee_get_fixed_font(Chmsee *);
void         chmsee_set_fixed_font(Chmsee *, const gchar *);

const gchar *chmsee_get_charset(Chmsee *);
void         chmsee_set_charset(Chmsee *, const gchar *);

gboolean     chmsee_get_startup_lastfile(Chmsee *);
void         chmsee_set_startup_lastfile(Chmsee *, gboolean);

gboolean     chmsee_has_book(Chmsee *);
const gchar *chmsee_get_bookshelf(Chmsee *);

G_END_DECLS

#endif /* !__CHMSEE_H__ */
