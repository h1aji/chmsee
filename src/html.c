/*
 *  Copyright (c) 2006           Ji YongGang <jungle@soforge-studio.com>
 *  Copyright (c) 2009           Cjacker <jzhuang@redflag-linux.com>
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

#include "html.h"

#include <string.h>

#include "ihtml.h"
#include "marshal.h"
#include "utils/utils.h"

static void html_class_init(HtmlClass *);
static void html_init(Html *);
static void html_location_changed(Html* self);


static gboolean webkit_web_view_mouse_click_cb(WebKitWebView *, gpointer, Html *);

static void webkit_web_view_hovering_over_link_cb (WebKitWebView*, const gchar*, const gchar*, Html*);
static void webkit_web_view_load_committed_cb (WebKitWebView*, WebKitWebFrame*, Html*);
static void webkit_web_view_title_changed_cb (WebKitWebView*, WebKitWebFrame*, const gchar*, Html*);

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

/* Has the value of the URL under the mouse pointer, otherwise NULL */
static gchar *current_url = NULL;

static void chmsee_ihtml_interface_init (ChmseeIhtmlInterface *iface);

G_DEFINE_TYPE_WITH_CODE (Html, html, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (CHMSEE_TYPE_IHTML,
                                                chmsee_ihtml_interface_init));

static void
html_class_init(HtmlClass *klass)
{
        signals[TITLE_CHANGED] =
                g_signal_new ("title-changed",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              0,
                              NULL, NULL,
                              marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1, G_TYPE_STRING);

        signals[LOCATION_CHANGED] =
                g_signal_new("location-changed",
                             G_TYPE_FROM_CLASS (klass),
                             G_SIGNAL_RUN_LAST,
                             0,
                             NULL, NULL,
                             marshal_VOID__STRING,
                             G_TYPE_NONE,
                             1, G_TYPE_STRING);

        signals[OPEN_URI] =
                g_signal_new("open-uri",
                             G_TYPE_FROM_CLASS (klass),
                             G_SIGNAL_RUN_LAST,
                             0,
                             NULL, NULL,
                             marshal_BOOLEAN__STRING,
                             G_TYPE_BOOLEAN,
                             1, G_TYPE_STRING);

        signals[CONTEXT_NORMAL] =
                g_signal_new("context-normal",
                             G_TYPE_FROM_CLASS (klass),
                             G_SIGNAL_RUN_LAST,
                             0,
                             NULL, NULL,
                             marshal_VOID__VOID,
                             G_TYPE_NONE,
                             0);

        signals[CONTEXT_LINK] =
                g_signal_new("context-link",
                             G_TYPE_FROM_CLASS (klass),
                             G_SIGNAL_RUN_LAST,
                             0,
                             NULL, NULL,
                             marshal_VOID__STRING,
                             G_TYPE_NONE,
                             1, G_TYPE_STRING);

        signals[OPEN_NEW_TAB] =
                g_signal_new("open-new-tab",
                             G_TYPE_FROM_CLASS (klass),
                             G_SIGNAL_RUN_LAST,
                             0,
                             NULL, NULL,
                             marshal_VOID__STRING,
                             G_TYPE_NONE,
                             1, G_TYPE_STRING);

        signals[LINK_MESSAGE] =
                g_signal_new("link-message",
                             G_TYPE_FROM_CLASS (klass),
                             G_SIGNAL_RUN_LAST,
                             0,
                             NULL, NULL,
                             marshal_VOID__STRING,
                             G_TYPE_NONE,
                             1, G_TYPE_STRING);
}

static void
html_init(Html *html)
{
        html->webview = WEBKIT_WEB_VIEW(webkit_web_view_new());
        html->scrolled = gtk_scrolled_window_new(0,0);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (html->scrolled),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_AUTOMATIC);
        GTK_WIDGET_SET_FLAGS (html->scrolled, GTK_CAN_FOCUS);
        gtk_container_add(GTK_CONTAINER (html->scrolled), GTK_WIDGET(html->webview));

        g_signal_connect(G_OBJECT (html->webview),
                         "title-changed",
                         G_CALLBACK (webkit_web_view_title_changed_cb),
                         html);
        g_signal_connect(G_OBJECT (html->webview),
                         "button-press-event",
                         G_CALLBACK (webkit_web_view_mouse_click_cb),
                         html);
        g_signal_connect(G_OBJECT (html->webview),
                         "hovering-over-link",
                         G_CALLBACK (webkit_web_view_hovering_over_link_cb),
                         html);
        g_signal_connect(G_OBJECT (html->webview),
                         "load-committed",
                         G_CALLBACK (webkit_web_view_load_committed_cb),
                         html);
        gtk_drag_dest_unset(GTK_WIDGET(html->webview));
}

/* callbacks */

static void 
webkit_web_view_title_changed_cb (WebKitWebView*  web_view,
                                  WebKitWebFrame* web_frame,
                                  const gchar*    title,
                                  Html*     self)
{
	const gchar *uri;
	uri = webkit_web_frame_get_uri(web_frame);
	g_signal_emit(self, signals[TITLE_CHANGED], 0, g_strdup(title));
	html_location_changed(self);
}

static void
webkit_web_view_load_committed_cb (WebKitWebView*  web_view,
                                 WebKitWebFrame* web_frame,
                                 Html*     html)
{
		g_object_freeze_notify (G_OBJECT (html));
		const gchar *uri =  webkit_web_frame_get_uri(web_frame);
		g_return_if_fail (uri != NULL);
        g_signal_emit(html, signals[OPEN_URI], 0, g_strdup(uri));
        if (current_url != NULL) {
                g_free(current_url);
                current_url = NULL;
                g_signal_emit(html, signals[LINK_MESSAGE], 0, "");
        }
		g_object_thaw_notify (G_OBJECT (html));

}

#define KEYS_MODIFIER_MASK (GDK_SHIFT_MASK | GDK_CONTROL_MASK \
    | GDK_MOD1_MASK | GDK_META_MASK | GDK_SUPER_MASK | GDK_HYPER_MASK )

static gboolean
webkit_web_view_mouse_click_cb(WebKitWebView *widget, gpointer dom_event, Html *html)
{
		GdkEventButton* event = dom_event;

        gint button=event->button;
        gint mask = event->state & KEYS_MODIFIER_MASK;

        if (button == 2 || (button == 1 && mask & GDK_CONTROL_MASK)) {
                if (current_url) {
                        g_signal_emit(html, signals[OPEN_NEW_TAB], 0, current_url);

                        return TRUE;
                }
        } else if (button == 3) {
                if (current_url)
                        g_signal_emit(html, signals[CONTEXT_LINK], 0, current_url);
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
                                       Html* html)
{
	g_free(current_url);
	current_url = g_strdup(link_uri);

	if(current_url==NULL)
		return;
	
	g_signal_emit(html, signals[LINK_MESSAGE], 0, current_url);
	if (current_url[0] == '\0') {
		g_free(current_url);
		current_url = NULL;
	}
	return ;

}


/* external functions */

Html *
html_new(void)
{
        Html *html;

        html = g_object_new(TYPE_HTML, NULL);

        return html;
}

void
html_clear(Html *html)
{
        static const char *data = "<html><body bgcolor=\"white\"></body></html>";

        g_return_if_fail(IS_HTML (html));
		webkit_web_view_load_html_string(html->webview,data,"file:///");
}

void
html_open_uri(Html *self, const gchar *str_uri)
{
	gchar *full_uri;

	g_return_if_fail(IS_HTML (self));
	g_return_if_fail(str_uri != NULL);

	if (str_uri[0] == '/')
		full_uri = g_strdup_printf("file://%s", str_uri);
	else
		full_uri = g_strdup(str_uri);

	if(g_strcmp0(full_uri, html_get_location(self)) != 0) {
		g_debug("Open uri %s", full_uri);
		webkit_web_view_open (self->webview, full_uri);
	}
	g_free(full_uri);
}

GtkWidget *
html_get_widget(Html *html)
{
        g_return_val_if_fail(IS_HTML (html), NULL);

        return GTK_WIDGET (html->scrolled);
}

gboolean
html_can_go_forward(Html *html)
{
        g_return_val_if_fail(IS_HTML (html), FALSE);
        return webkit_web_view_can_go_forward(html->webview);
}

gboolean
html_can_go_back(Html *html)
{
        g_return_val_if_fail(IS_HTML (html), FALSE);
        return webkit_web_view_can_go_back(html->webview);
}

void
html_go_forward(Html* self)
{
        g_return_if_fail(IS_HTML (self));
        webkit_web_view_go_forward(self->webview);
    	html_location_changed(self);
}

void
html_go_back(Html * self)
{
	g_return_if_fail(IS_HTML (self));
	webkit_web_view_go_back(self->webview);
	html_location_changed(self);
}

gchar *
html_get_title(Html *html)
{
        g_return_val_if_fail(IS_HTML (html), NULL);
        WebKitWebFrame *web_frame;
        web_frame = webkit_web_view_get_main_frame (html->webview);
        gchar *web_view_title=NULL;
		if(web_frame)
        	web_view_title = g_strdup(webkit_web_frame_get_title (web_frame));

        return web_view_title; 
}

gchar *
html_get_location(Html *html)
{
        g_return_val_if_fail(IS_HTML (html), NULL);
 		WebKitWebFrame *web_frame;
        web_frame = webkit_web_view_get_main_frame (html->webview);
		gchar *uri = g_strdup(webkit_web_frame_get_uri(web_frame));
		return uri;
}

void
html_copy_selection(Html *html)
{
	g_return_if_fail(IS_HTML (html));
	GtkWidget *widget = GTK_WIDGET(html->webview);
	if (G_LIKELY (widget) && g_signal_lookup ("copy-clipboard", G_OBJECT_TYPE (widget)))
		g_signal_emit_by_name (widget, "copy-clipboard");
}

void
html_select_all(Html *html)
{
	g_return_if_fail(IS_HTML (html));
	GtkWidget *widget = GTK_WIDGET(html->webview);
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

void
html_increase_size(Html *html)
{
        gfloat zoom;

        g_return_if_fail(IS_HTML (html));

        zoom = webkit_web_view_get_zoom_level(html->webview);
        zoom *= 1.2;
		webkit_web_view_set_zoom_level(html->webview,zoom);	
}

void
html_reset_size(Html *html)
{
	g_return_if_fail(IS_HTML (html));
	webkit_web_view_set_zoom_level(html->webview,1.0);
}

void
html_decrease_size(Html *html)
{
        gfloat zoom;

        g_return_if_fail(IS_HTML (html));

        zoom = webkit_web_view_get_zoom_level(html->webview);
        zoom /= 1.2;
		webkit_web_view_set_zoom_level(html->webview,zoom);	
}

void html_shutdown(Html* html) {
  //Need it?
}

void html_init_system(void) {
  //Need it?
}

static const gchar *encoding_list[] = {
        "AUTO",
        "GBK",
        "BIG5",
        "EUC-JP",
        "EUC-KR",
        "KOI8-R",
        "ISO-8859-5"
};

void html_set_default_lang(Html *html, gint lang_index) {
	fprintf(stderr,"%s %s\n",__func__, encoding_list[lang_index]);
	if(lang_index==0) //auto
		g_object_set(html->webview,"custom-encoding",NULL,NULL);
	else
		g_object_set(html->webview,"custom-encoding",encoding_list[lang_index],NULL);

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

void html_set_variable_font(Html* html, const gchar* font) {

    gint size;
    gchar *name;
    split_font_string(font,&name,&size);
    WebKitWebSettings *settings;
    settings = webkit_web_view_get_settings (WEBKIT_WEB_VIEW (html->webview));
    g_object_set (settings,
            "default-font-family", name,
            "default-font-size", (guint) size,
            NULL);
}

void html_set_fixed_font(Html* html, const gchar* font) {
    gint size;
    gchar *name;
    split_font_string(font,&name,&size);
    WebKitWebSettings *settings;
    settings = webkit_web_view_get_settings (WEBKIT_WEB_VIEW (html->webview));
    g_object_set (settings,
            "monospace-font-family", name,
            "default-monospace-font-size", (guint) size,
            NULL);
}


void chmsee_ihtml_interface_init (ChmseeIhtmlInterface *iface) {
  iface->can_go_back = html_can_go_back;
  iface->can_go_forward = html_can_go_forward;
  iface->open_uri = html_open_uri;
  iface->go_back = html_go_back;
  iface->go_forward = html_go_forward;
  iface->get_widget = html_get_widget;
  iface->get_title = html_get_title;
  iface->get_location = html_get_location;
  iface->increase_size = html_increase_size;
  iface->decrease_size = html_decrease_size;
  iface->reset_size = html_reset_size;
  iface->copy_selection = html_copy_selection;
  iface->select_all = html_select_all;
  iface->set_variable_font = html_set_variable_font;
  iface->set_fixed_font = html_set_fixed_font;
  iface->set_lang = html_set_default_lang;

  iface->clear = html_clear;
  iface->shutdown = html_shutdown;
}

static void html_location_changed(Html* self) {
	gchar* uri = g_strdup(webkit_web_view_get_uri(self->webview));
	g_signal_emit(self, signals[LOCATION_CHANGED], 0, uri);
	g_free(uri);
}
