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

#ifndef __CS_HTML_WEBKIT_H__
#define __CS_HTML_WEBKIT_H__

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define CS_TYPE_HTML_WEBKIT        (cs_html_webkit_get_type())
#define CS_HTML_WEBKIT(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), CS_TYPE_HTML_WEBKIT, CsHtmlWebkit))
#define CS_HTML_WEBKIT_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), CS_TYPE_HTML_WEBKIT, CsHtmlWebkitClass))
#define IS_CS_HTML_WEBKIT(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), CS_TYPE_HTML_WEBKIT))
#define IS_CS_HTML_WEBKIT_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), CS_TYPE_HTML_WEBKIT))

typedef struct _CsHtmlWebkit        CsHtmlWebkit;
typedef struct _CsHtmlWebkitClass   CsHtmlWebkitClass;

struct _CsHtmlWebkit {
        GtkFrame      frame;
};

struct _CsHtmlWebkitClass {
        GtkFrameClass parent_class;

        /* Signals */
        void     (* title_changed)    (CsHtmlWebkit *html, const gchar *title);
        void     (* location_changed) (CsHtmlWebkit *html, const gchar *location);
        gboolean (* open_uri)         (CsHtmlWebkit *html, const gchar *uri);
        void     (* context_normal)   (CsHtmlWebkit *html);
        void     (* context_link)     (CsHtmlWebkit *html, const gchar *link);
        void     (* open_new_tab)     (CsHtmlWebkit *html, const gchar *uri);
        void     (* link_message)     (CsHtmlWebkit *html, const gchar *link);
};

GType        cs_html_webkit_get_type(void);
GtkWidget   *cs_html_webkit_new(void);
void         cs_html_webkit_load_url(CsHtmlWebkit *, const gchar *);
void         cs_html_webkit_reload(CsHtmlWebkit *);

gboolean     cs_html_webkit_can_go_forward(CsHtmlWebkit *);
gboolean     cs_html_webkit_can_go_back(CsHtmlWebkit *);
void         cs_html_webkit_go_forward(CsHtmlWebkit *);
void         cs_html_webkit_go_back(CsHtmlWebkit *);
gchar       *cs_html_webkit_get_title(CsHtmlWebkit *);
gchar       *cs_html_webkit_get_location(CsHtmlWebkit *);
gboolean     cs_html_webkit_can_copy_selection(CsHtmlWebkit *);
void         cs_html_webkit_copy_selection(CsHtmlWebkit *);
void         cs_html_webkit_select_all(CsHtmlWebkit *);
void         cs_html_webkit_increase_size(CsHtmlWebkit *);
void         cs_html_webkit_reset_size(CsHtmlWebkit *);
void         cs_html_webkit_decrease_size(CsHtmlWebkit *);
gboolean     cs_html_webkit_find(CsHtmlWebkit *, const gchar *, gboolean, gboolean);

gboolean     cs_html_webkit_init_system(void);
void         cs_html_webkit_shutdown_system(void);

void         cs_html_webkit_set_variable_font(CsHtmlWebkit *,const gchar *);
void         cs_html_webkit_set_fixed_font(CsHtmlWebkit *,const gchar *);

void         cs_html_webkit_set_charset(CsHtmlWebkit *, const gchar *);

G_END_DECLS

#endif /* !__CS_HTML_WEBKIT_H__ */
