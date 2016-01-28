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

#include <string.h>
#include <webkit/webkit.h>

#include "html-webkit.h"
#include "utils.h"

typedef struct _CsHtmlWebkitPrivate CsHtmlWebkitPrivate;

struct _CsHtmlWebkitPrivate {
        WebKitWebView *webkit;
        GtkWidget     * scrolled;
        gchar         *render_name;
        gchar         *current_url;
};

static void cs_html_webkit_class_init(CsHtmlWebkitClass *);
static void cs_html_webkit_init(CsHtmlWebkit *);
static void cs_html_webkit_finalize(GObject *);
static gboolean webkit_web_view_mouse_click_cb(WebKitWebView *, gpointer, CsHtmlWebkit *);
static void webkit_web_view_hovering_over_link_cb (WebKitWebView*, const gchar*, const gchar*, CsHtmlWebkit*);
static void webkit_web_view_load_committed_cb (WebKitWebView*, WebKitWebFrame*, CsHtmlWebkit*);
static void webkit_title_cb (WebKitWebView*, WebKitWebFrame*, const gchar*, CsHtmlWebkit*);


/* Signals */
enum {
        TITLE_CHANGED,
        LOCATION_CHANGED,
        OPEN_URI,
        CONTEXT_NORMAL,
        CONTEXT_LINK,
        OPEN_NEW_TAB,
        LINK_MESSAGE,
        LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = { 0 };

#define CS_HTML_WEBKIT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), CS_TYPE_HTML_WEBKIT, CsHtmlWebkitPrivate))

/* GObject functions */

G_DEFINE_TYPE (CsHtmlWebkit, cs_html_webkit, GTK_TYPE_FRAME);

static void
cs_html_webkit_class_init(CsHtmlWebkitClass *klass)
{
        G_OBJECT_CLASS (klass)->finalize = cs_html_webkit_finalize;
        g_type_class_add_private(klass, sizeof(CsHtmlWebkitPrivate));

        signals[TITLE_CHANGED] =
                g_signal_new ("title-changed",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              0,
                              NULL, NULL,
                              gtk_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1, G_TYPE_STRING);

        signals[LOCATION_CHANGED] =
                g_signal_new("location-changed",
                             G_TYPE_FROM_CLASS (klass),
                             G_SIGNAL_RUN_LAST,
                             0,
                             NULL, NULL,
                             gtk_marshal_VOID__STRING,
                             G_TYPE_NONE,
                             1, G_TYPE_STRING);

        signals[OPEN_URI] =
                g_signal_new("open-uri",
                             G_TYPE_FROM_CLASS (klass),
                             G_SIGNAL_RUN_LAST,
                             0,
                             NULL, NULL,
                             gtk_marshal_BOOLEAN__POINTER,
                             G_TYPE_BOOLEAN,
                             1, G_TYPE_POINTER);

        signals[CONTEXT_NORMAL] =
                g_signal_new("context-normal",
                             G_TYPE_FROM_CLASS (klass),
                             G_SIGNAL_RUN_LAST,
                             0,
                             NULL, NULL,
                             gtk_marshal_VOID__VOID,
                             G_TYPE_NONE,
                             0);

        signals[CONTEXT_LINK] =
                g_signal_new("context-link",
                             G_TYPE_FROM_CLASS (klass),
                             G_SIGNAL_RUN_LAST,
                             0,
                             NULL, NULL,
                             gtk_marshal_VOID__STRING,
                             G_TYPE_NONE,
                             1, G_TYPE_STRING);

        signals[OPEN_NEW_TAB] =
                g_signal_new("open-new-tab",
                             G_TYPE_FROM_CLASS (klass),
                             G_SIGNAL_RUN_LAST,
                             0,
                             NULL, NULL,
                             gtk_marshal_VOID__STRING,
                             G_TYPE_NONE,
                             1, G_TYPE_STRING);

        signals[LINK_MESSAGE] =
                g_signal_new("link-message",
                             G_TYPE_FROM_CLASS (klass),
                             G_SIGNAL_RUN_LAST,
                             0,
                             NULL, NULL,
                             gtk_marshal_VOID__STRING,
                             G_TYPE_NONE,
                             1, G_TYPE_STRING);
}

static void
cs_html_webkit_init(CsHtmlWebkit *html)
{
        CsHtmlWebkitPrivate *priv = CS_HTML_WEBKIT_GET_PRIVATE (html);
        priv->webkit = WEBKIT_WEB_VIEW(webkit_web_view_new());
        priv->scrolled = gtk_scrolled_window_new(0,0);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->scrolled),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_AUTOMATIC);
        GTK_WIDGET_SET_FLAGS (priv->scrolled, GTK_CAN_FOCUS);
        gtk_container_add(GTK_CONTAINER (priv->scrolled), GTK_WIDGET(priv->webkit));

        gtk_widget_show_all(GTK_WIDGET (priv->scrolled));
        gtk_widget_show_all(GTK_WIDGET (priv->webkit));
        priv->render_name = g_strdup("Webkit");
        priv->current_url = NULL;

        gtk_frame_set_shadow_type(GTK_FRAME (html), GTK_SHADOW_NONE);
        gtk_container_add(GTK_CONTAINER (html), GTK_WIDGET (priv->scrolled));

        g_signal_connect(G_OBJECT (priv->webkit),
                         "title-changed",
                         G_CALLBACK (webkit_title_cb),
                         html);
        g_signal_connect(G_OBJECT (priv->webkit),
                         "button-press-event",
                         G_CALLBACK (webkit_web_view_mouse_click_cb),
                         html);
        g_signal_connect(G_OBJECT (priv->webkit),
                         "hovering-over-link",
                         G_CALLBACK (webkit_web_view_hovering_over_link_cb),
                         html);
        g_signal_connect(G_OBJECT (priv->webkit),
                         "load-committed",
                         G_CALLBACK (webkit_web_view_load_committed_cb),
                         html);
}

static void
cs_html_webkit_finalize(GObject *object)
{
        CsHtmlWebkitPrivate *priv = CS_HTML_WEBKIT_GET_PRIVATE (CS_HTML_WEBKIT (object));

        g_free(priv->render_name);
        g_free(priv->current_url);
        G_OBJECT_CLASS (cs_html_webkit_parent_class)->finalize(object);
}

/* Callbacks */


static void 
webkit_title_cb (WebKitWebView*  web_view,
                                  WebKitWebFrame* web_frame,
                                  const gchar*    title,
                                  CsHtmlWebkit*     html)
{
   const gchar *uri;
   uri = webkit_web_frame_get_uri(web_frame);
   g_signal_emit(html, signals[TITLE_CHANGED], 0, g_strdup(title));
   g_signal_emit(html, signals[LOCATION_CHANGED], 0,g_strdup(uri) );
}

static void
webkit_web_view_load_committed_cb (WebKitWebView*  web_view,
                                 WebKitWebFrame* web_frame,
                                 CsHtmlWebkit*     html)
{
        g_object_freeze_notify (G_OBJECT (html));
        const gchar *uri =  webkit_web_frame_get_uri(web_frame);
        g_return_if_fail (uri != NULL);
        //g_signal_emit(html, signals[OPEN_URI], 0, g_strdup(uri));
        if (CS_HTML_WEBKIT_GET_PRIVATE (html)->current_url != NULL) {
                g_free(CS_HTML_WEBKIT_GET_PRIVATE (html)->current_url);
                CS_HTML_WEBKIT_GET_PRIVATE (html)->current_url = NULL;
                g_signal_emit(html, signals[LINK_MESSAGE], 0, "");
        }
        g_object_thaw_notify (G_OBJECT (html));

}

#define KEYS_MODIFIER_MASK (GDK_SHIFT_MASK | GDK_CONTROL_MASK \
    | GDK_MOD1_MASK | GDK_META_MASK | GDK_SUPER_MASK | GDK_HYPER_MASK )

static gboolean
webkit_web_view_mouse_click_cb(WebKitWebView *widget, gpointer dom_event, CsHtmlWebkit *html)
{
        GdkEventButton* event = dom_event;

        gint button=event->button;
        gint mask = event->state & KEYS_MODIFIER_MASK;

        if (button == 2 || (button == 1 && mask & GDK_CONTROL_MASK)) {
                if (CS_HTML_WEBKIT_GET_PRIVATE (html)->current_url) {
                        g_signal_emit(html, signals[OPEN_NEW_TAB], 0, CS_HTML_WEBKIT_GET_PRIVATE (html)->current_url);

                        return TRUE;
                }
        } else if (button == 3) {
                if (CS_HTML_WEBKIT_GET_PRIVATE (html)->current_url)
                        g_signal_emit(html, signals[CONTEXT_LINK], 0, CS_HTML_WEBKIT_GET_PRIVATE (html)->current_url);
                else
                        g_signal_emit(html, signals[CONTEXT_NORMAL], 0);

                return TRUE;
        }

        return FALSE;
}

static void
webkit_web_view_hovering_over_link_cb (WebKitWebView* web_view,
                                       const gchar*   tooltip,
                                       const gchar*   link_uri,
                                       CsHtmlWebkit* html)
{
    g_free(CS_HTML_WEBKIT_GET_PRIVATE (html)->current_url);
    CS_HTML_WEBKIT_GET_PRIVATE (html)->current_url = g_strdup(link_uri);

    if(CS_HTML_WEBKIT_GET_PRIVATE (html)->current_url==NULL)
        return;

    g_signal_emit(html, signals[LINK_MESSAGE], 0, CS_HTML_WEBKIT_GET_PRIVATE (html)->current_url);
    if (CS_HTML_WEBKIT_GET_PRIVATE (html)->current_url[0] == '\0') {
        g_free(CS_HTML_WEBKIT_GET_PRIVATE (html)->current_url);
        CS_HTML_WEBKIT_GET_PRIVATE (html)->current_url = NULL;
    }
    return ;

}


/* External functions */

GtkWidget *
cs_html_webkit_new(void)
{
        CsHtmlWebkit *html = g_object_new(CS_TYPE_HTML_WEBKIT, NULL);

        return GTK_WIDGET (html);
}

void
cs_html_webkit_load_url(CsHtmlWebkit *html, const gchar *url)
{
        g_return_if_fail(IS_CS_HTML_WEBKIT (html));
        g_return_if_fail(url != NULL);

        g_debug("CS_HTML_WEBKIT >>> load_url html = %p, uri = %s", html, url);
        webkit_web_view_open(CS_HTML_WEBKIT_GET_PRIVATE (html)->webkit, url);
}

void
cs_html_webkit_reload(CsHtmlWebkit *html)
{
        g_return_if_fail(IS_CS_HTML_WEBKIT (html));

        webkit_web_view_reload(CS_HTML_WEBKIT_GET_PRIVATE (html)->webkit);
}

gboolean
cs_html_webkit_can_go_forward(CsHtmlWebkit *html)
{
        g_return_val_if_fail(IS_CS_HTML_WEBKIT (html), FALSE);

        return webkit_web_view_can_go_forward(CS_HTML_WEBKIT_GET_PRIVATE (html)->webkit);
}

gboolean
cs_html_webkit_can_go_back(CsHtmlWebkit *html)
{
        g_return_val_if_fail(IS_CS_HTML_WEBKIT (html), FALSE);

        return webkit_web_view_can_go_back(CS_HTML_WEBKIT_GET_PRIVATE (html)->webkit);
}

void
cs_html_webkit_go_forward(CsHtmlWebkit *html)
{
        g_return_if_fail(IS_CS_HTML_WEBKIT (html));

        webkit_web_view_go_forward(CS_HTML_WEBKIT_GET_PRIVATE (html)->webkit);
}

void
cs_html_webkit_go_back(CsHtmlWebkit *html)
{
        g_return_if_fail(IS_CS_HTML_WEBKIT (html));

        webkit_web_view_go_back(CS_HTML_WEBKIT_GET_PRIVATE (html)->webkit);
}

gchar *
cs_html_webkit_get_title(CsHtmlWebkit *html)
{
        g_return_val_if_fail(IS_CS_HTML_WEBKIT (html), NULL);

        WebKitWebFrame *web_frame;
        web_frame = webkit_web_view_get_main_frame (CS_HTML_WEBKIT_GET_PRIVATE (html)->webkit);
        gchar *web_view_title=NULL;
        if(web_frame)
            web_view_title = g_strdup(webkit_web_frame_get_title (web_frame));

        return web_view_title;
}

gchar *
cs_html_webkit_get_location(CsHtmlWebkit *html)
{
        g_return_val_if_fail(IS_CS_HTML_WEBKIT (html), NULL);

        WebKitWebFrame *web_frame;
        web_frame = webkit_web_view_get_main_frame (CS_HTML_WEBKIT_GET_PRIVATE (html)->webkit);
        gchar *uri = g_strdup(webkit_web_frame_get_uri(web_frame));
        return uri;
}

gboolean
cs_html_webkit_can_copy_selection(CsHtmlWebkit *html)
{
        g_return_val_if_fail(IS_CS_HTML_WEBKIT (html), FALSE);

        return webkit_web_view_can_copy_clipboard(CS_HTML_WEBKIT_GET_PRIVATE (html)->webkit);
}

void
cs_html_webkit_copy_selection(CsHtmlWebkit *html)
{
        g_return_if_fail(IS_CS_HTML_WEBKIT (html));
        GtkWidget *widget = GTK_WIDGET(CS_HTML_WEBKIT_GET_PRIVATE (html)->webkit);
        if (G_LIKELY (widget) && g_signal_lookup ("copy-clipboard", G_OBJECT_TYPE (widget)))
            g_signal_emit_by_name (widget, "copy-clipboard");
}

void
cs_html_webkit_select_all(CsHtmlWebkit *html)
{
        g_return_if_fail(IS_CS_HTML_WEBKIT (html));
    GtkWidget *widget = GTK_WIDGET(CS_HTML_WEBKIT_GET_PRIVATE (html)->webkit);
    if (GTK_IS_EDITABLE (widget))
        gtk_editable_select_region (GTK_EDITABLE (widget), 0, -1);
    else if (g_signal_lookup ("select-all", G_OBJECT_TYPE (widget)))
    {
        if (GTK_IS_TEXT_VIEW (widget))
            g_signal_emit_by_name (widget, "select-all", TRUE);
        else if (GTK_IS_TREE_VIEW (widget))
        {
            gboolean dummy;
            g_signal_emit_by_name (widget, "select-all", &dummy);
        }
        else
            g_signal_emit_by_name (widget, "select-all");
    }

}

gboolean
cs_html_webkit_find(CsHtmlWebkit *html, const gchar *sstr, gboolean backward, gboolean match_case)
{
        g_return_val_if_fail(IS_CS_HTML_WEBKIT (html), FALSE);

        return webkit_web_view_search_text (CS_HTML_WEBKIT_GET_PRIVATE (html)->webkit,sstr,match_case,backward,TRUE);
}

void
cs_html_webkit_increase_size(CsHtmlWebkit *html)
{
        g_return_if_fail(IS_CS_HTML_WEBKIT (html));

        CsHtmlWebkitPrivate *priv = CS_HTML_WEBKIT_GET_PRIVATE (html);
        gfloat zoom = webkit_web_view_get_zoom_level(priv->webkit);
        zoom *= 1.2;
        webkit_web_view_set_zoom_level(priv->webkit,zoom);
}

void
cs_html_webkit_reset_size(CsHtmlWebkit *html)
{
        g_return_if_fail(IS_CS_HTML_WEBKIT (html));
        CsHtmlWebkitPrivate *priv = CS_HTML_WEBKIT_GET_PRIVATE (html);
        webkit_web_view_set_zoom_level(priv->webkit,1.0);
}

void
cs_html_webkit_decrease_size(CsHtmlWebkit *html)
{
        g_return_if_fail(IS_CS_HTML_WEBKIT (html));

        CsHtmlWebkitPrivate *priv = CS_HTML_WEBKIT_GET_PRIVATE (html);

        gfloat zoom = webkit_web_view_get_zoom_level(priv->webkit);
        zoom /= 1.2;
        webkit_web_view_set_zoom_level(priv->webkit,zoom);
}

gboolean
cs_html_webkit_init_system(void)
{
        g_message("CS_HTML_WEBKIT >>> init webkit system");
        return TRUE;
}

void
cs_html_webkit_shutdown_system()
{
        g_message("CS_HTML_WEBKIT >>> shutdown webkit system");
}

static gboolean
split_font_string(const gchar *font_name, gchar **name, gint *size)
{
    PangoFontDescription *desc;
    PangoFontMask         mask;
    gboolean              retval = FALSE;

    if (font_name == NULL) {
        return FALSE;
    }

    mask = (PangoFontMask) (PANGO_FONT_MASK_FAMILY | PANGO_FONT_MASK_SIZE);

    desc = pango_font_description_from_string(font_name);    if (!desc) {
        return FALSE;
    }

    if ((pango_font_description_get_set_fields(desc) & mask) == mask) {
        *size = PANGO_PIXELS(pango_font_description_get_size (desc));
        *name = g_strdup(pango_font_description_get_family (desc));
        retval = TRUE;
    }

    pango_font_description_free(desc);

    return retval;
}

void
cs_html_webkit_set_variable_font(CsHtmlWebkit *html,const gchar *font_name)
{
        g_return_if_fail(IS_CS_HTML_WEBKIT (html));
    g_debug("CS_HTML_WEBKIT >>> set variable font %s", font_name);
    gint size;
    gchar *name;
    split_font_string(font_name,&name,&size);

    CsHtmlWebkitPrivate *priv = CS_HTML_WEBKIT_GET_PRIVATE (html);
    WebKitWebSettings *settings;
    settings = webkit_web_view_get_settings (WEBKIT_WEB_VIEW (priv->webkit));
    g_object_set (settings,
            "default-font-family", name,
            "default-font-size", (guint) size,
            NULL);
}

void
cs_html_webkit_set_fixed_font(CsHtmlWebkit *html,const gchar *font_name)
{
        g_return_if_fail(IS_CS_HTML_WEBKIT (html));
    g_debug("CS_HTML_WEBKIT >>> set fixed font %s", font_name);
    gint size;
    gchar *name;
    split_font_string(font_name,&name,&size);
    CsHtmlWebkitPrivate *priv = CS_HTML_WEBKIT_GET_PRIVATE (html);
    WebKitWebSettings *settings;
    settings = webkit_web_view_get_settings (WEBKIT_WEB_VIEW (priv->webkit));
    g_object_set (settings,
            "monospace-font-family", name,
            "default-monospace-font-size", (guint) size,
            NULL);

}

void
cs_html_webkit_set_charset(CsHtmlWebkit *html, const gchar *charset)
{
    g_return_if_fail(IS_CS_HTML_WEBKIT (html));
    CsHtmlWebkitPrivate *priv = CS_HTML_WEBKIT_GET_PRIVATE (html);
    if(strcmp(charset,"Auto") == 0) //auto
        g_object_set(priv->webkit,"custom-encoding",NULL,NULL);
    else
        g_object_set(priv->webkit,"custom-encoding",charset,NULL);
}
