/*
 *  Copyright (C) 2010 Ji YongGang <jungleji@gmail.com>
 *  ChmSee is free software; you may redistribute it and/or modify it
 *  under the terms of the GNU General Public License, version 2 or later.
 */

#include <string.h>
#include <webkit2/webkit2.h>

#include "html-webkit.h"

typedef struct {
        WebKitWebView *webkit;
        gchar *current_url;
        gchar *requested_uri;
        gchar *find_text;
        gboolean find_match_case;
} CsHtmlWebkitPrivate;

enum {
        TITLE_CHANGED, LOCATION_CHANGED, OPEN_URI, CONTEXT_NORMAL,
        CONTEXT_LINK, OPEN_NEW_TAB, LINK_MESSAGE, LAST_SIGNAL
};
static guint signals[LAST_SIGNAL];

G_DEFINE_TYPE_WITH_PRIVATE(CsHtmlWebkit, cs_html_webkit, GTK_TYPE_FRAME)

static CsHtmlWebkitPrivate *
get_priv(CsHtmlWebkit *html)
{
        return cs_html_webkit_get_instance_private(html);
}

static void
cs_html_webkit_finalize(GObject *object)
{
        CsHtmlWebkitPrivate *priv = get_priv(CS_HTML_WEBKIT(object));
        g_free(priv->current_url);
        g_free(priv->requested_uri);
        g_free(priv->find_text);
        G_OBJECT_CLASS(cs_html_webkit_parent_class)->finalize(object);
}

static void
cs_html_webkit_class_init(CsHtmlWebkitClass *klass)
{
        G_OBJECT_CLASS(klass)->finalize = cs_html_webkit_finalize;
#define STRING_SIGNAL(name) g_signal_new(name, G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING)
        signals[TITLE_CHANGED] = STRING_SIGNAL("title-changed");
        signals[LOCATION_CHANGED] = STRING_SIGNAL("location-changed");
        signals[CONTEXT_LINK] = STRING_SIGNAL("context-link");
        signals[OPEN_NEW_TAB] = STRING_SIGNAL("open-new-tab");
        signals[LINK_MESSAGE] = STRING_SIGNAL("link-message");
#undef STRING_SIGNAL
        signals[OPEN_URI] = g_signal_new("open-uri", G_TYPE_FROM_CLASS(klass),
                G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_BOOLEAN, 1, G_TYPE_STRING);
        signals[CONTEXT_NORMAL] = g_signal_new("context-normal", G_TYPE_FROM_CLASS(klass),
                G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void
view_title_changed(WebKitWebView *view, GParamSpec *pspec, CsHtmlWebkit *html)
{
        g_signal_emit(html, signals[TITLE_CHANGED], 0, webkit_web_view_get_title(view));
}

static void
view_uri_changed(WebKitWebView *view, GParamSpec *pspec, CsHtmlWebkit *html)
{
        const gchar *uri = webkit_web_view_get_uri(view);
        if (uri)
                g_signal_emit(html, signals[LOCATION_CHANGED], 0, uri);
        g_clear_pointer(&get_priv(html)->current_url, g_free);
        g_signal_emit(html, signals[LINK_MESSAGE], 0, "");
}

static void
view_mouse_target_changed(WebKitWebView *view, WebKitHitTestResult *hit,
                          guint modifiers, CsHtmlWebkit *html)
{
        CsHtmlWebkitPrivate *priv = get_priv(html);
        const gchar *uri = webkit_hit_test_result_context_is_link(hit) ?
                webkit_hit_test_result_get_link_uri(hit) : NULL;
        if (g_strcmp0(uri, priv->current_url) == 0)
                return;
        g_free(priv->current_url);
        priv->current_url = g_strdup(uri);
        g_signal_emit(html, signals[LINK_MESSAGE], 0, uri ? uri : "");
}

static gboolean
view_context_menu(WebKitWebView *view, WebKitContextMenu *menu,
                  GdkEvent *event, WebKitHitTestResult *hit, CsHtmlWebkit *html)
{
        if (webkit_hit_test_result_context_is_link(hit)) {
                const gchar *uri = webkit_hit_test_result_get_link_uri(hit);
                if (uri)
                        g_signal_emit(html, signals[CONTEXT_LINK], 0, uri);
        } else {
                g_signal_emit(html, signals[CONTEXT_NORMAL], 0);
        }
        return TRUE; /* The application's GTK menu replaces WebKit's menu. */
}

static gboolean
view_decide_policy(WebKitWebView *view, WebKitPolicyDecision *decision,
                   WebKitPolicyDecisionType type, CsHtmlWebkit *html)
{
        CsHtmlWebkitPrivate *priv = get_priv(html);
        WebKitNavigationAction *action;
        const gchar *uri;
        gboolean handled = FALSE;

        if (type != WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION &&
            type != WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION)
                return FALSE;

        action = webkit_navigation_policy_decision_get_navigation_action(
                WEBKIT_NAVIGATION_POLICY_DECISION(decision));
        uri = webkit_uri_request_get_uri(webkit_navigation_action_get_request(action));

        /* load_uri() also produces policy decisions, asynchronously. Allow our own
         * request once instead of recursively routing it back through the book. */
        if (priv->requested_uri && g_strcmp0(uri, priv->requested_uri) == 0) {
                g_clear_pointer(&priv->requested_uri, g_free);
                webkit_policy_decision_use(decision);
                return TRUE;
        }

        if (webkit_navigation_action_get_navigation_type(action) == WEBKIT_NAVIGATION_TYPE_BACK_FORWARD) {
                webkit_policy_decision_use(decision);
                return TRUE;
        }

        if (type == WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION ||
            (webkit_navigation_action_get_navigation_type(action) == WEBKIT_NAVIGATION_TYPE_LINK_CLICKED &&
             (webkit_navigation_action_get_mouse_button(action) == 2 ||
              (webkit_navigation_action_get_modifiers(action) & GDK_CONTROL_MASK)))) {
                g_signal_emit(html, signals[OPEN_NEW_TAB], 0, uri);
                webkit_policy_decision_ignore(decision);
                return TRUE;
        }

        g_signal_emit(html, signals[OPEN_URI], 0, uri, &handled);
        if (handled)
                webkit_policy_decision_ignore(decision);
        else
                webkit_policy_decision_use(decision);
        return TRUE;
}

static void
cs_html_webkit_init(CsHtmlWebkit *html)
{
        CsHtmlWebkitPrivate *priv = get_priv(html);
        priv->webkit = WEBKIT_WEB_VIEW(webkit_web_view_new());
        gtk_frame_set_shadow_type(GTK_FRAME(html), GTK_SHADOW_NONE);
        /* WebKit2's web view is scrollable itself; do not nest it in a scroller. */
        gtk_container_add(GTK_CONTAINER(html), GTK_WIDGET(priv->webkit));
        gtk_widget_show(GTK_WIDGET(priv->webkit));
        g_signal_connect(priv->webkit, "notify::title", G_CALLBACK(view_title_changed), html);
        g_signal_connect(priv->webkit, "notify::uri", G_CALLBACK(view_uri_changed), html);
        g_signal_connect(priv->webkit, "mouse-target-changed", G_CALLBACK(view_mouse_target_changed), html);
        g_signal_connect(priv->webkit, "context-menu", G_CALLBACK(view_context_menu), html);
        g_signal_connect(priv->webkit, "decide-policy", G_CALLBACK(view_decide_policy), html);
}

GtkWidget *cs_html_webkit_new(void)
{
        return g_object_new(CS_TYPE_HTML_WEBKIT, NULL);
}

void cs_html_webkit_load_url(CsHtmlWebkit *html, const gchar *url)
{
        g_return_if_fail(IS_CS_HTML_WEBKIT(html) && url != NULL);
        CsHtmlWebkitPrivate *priv = get_priv(html);
        g_free(priv->requested_uri);
        priv->requested_uri = g_strdup(url);
        webkit_web_view_load_uri(priv->webkit, url);
}

void cs_html_webkit_reload(CsHtmlWebkit *html)
{
        g_return_if_fail(IS_CS_HTML_WEBKIT(html));
        webkit_web_view_reload(get_priv(html)->webkit);
}

gboolean cs_html_webkit_can_go_forward(CsHtmlWebkit *html)
{
        g_return_val_if_fail(IS_CS_HTML_WEBKIT(html), FALSE);
        return webkit_web_view_can_go_forward(get_priv(html)->webkit);
}

gboolean cs_html_webkit_can_go_back(CsHtmlWebkit *html)
{
        g_return_val_if_fail(IS_CS_HTML_WEBKIT(html), FALSE);
        return webkit_web_view_can_go_back(get_priv(html)->webkit);
}

void cs_html_webkit_go_forward(CsHtmlWebkit *html)
{
        g_return_if_fail(IS_CS_HTML_WEBKIT(html));
        webkit_web_view_go_forward(get_priv(html)->webkit);
}

void cs_html_webkit_go_back(CsHtmlWebkit *html)
{
        g_return_if_fail(IS_CS_HTML_WEBKIT(html));
        webkit_web_view_go_back(get_priv(html)->webkit);
}

gchar *cs_html_webkit_get_title(CsHtmlWebkit *html)
{
        g_return_val_if_fail(IS_CS_HTML_WEBKIT(html), NULL);
        return g_strdup(webkit_web_view_get_title(get_priv(html)->webkit));
}

gchar *cs_html_webkit_get_location(CsHtmlWebkit *html)
{
        g_return_val_if_fail(IS_CS_HTML_WEBKIT(html), NULL);
        return g_strdup(webkit_web_view_get_uri(get_priv(html)->webkit));
}

gboolean cs_html_webkit_can_copy_selection(CsHtmlWebkit *html)
{
        g_return_val_if_fail(IS_CS_HTML_WEBKIT(html), FALSE);
        /* WebKit2 only exposes this query asynchronously. Keep Copy in the
         * context menu; the editing command is harmless without a selection. */
        return TRUE;
}

void cs_html_webkit_copy_selection(CsHtmlWebkit *html)
{
        g_return_if_fail(IS_CS_HTML_WEBKIT(html));
        webkit_web_view_execute_editing_command(get_priv(html)->webkit, WEBKIT_EDITING_COMMAND_COPY);
}

void cs_html_webkit_select_all(CsHtmlWebkit *html)
{
        g_return_if_fail(IS_CS_HTML_WEBKIT(html));
        webkit_web_view_execute_editing_command(get_priv(html)->webkit, WEBKIT_EDITING_COMMAND_SELECT_ALL);
}

gboolean cs_html_webkit_find(CsHtmlWebkit *html, const gchar *text,
                             gboolean backward, gboolean match_case)
{
        g_return_val_if_fail(IS_CS_HTML_WEBKIT(html), FALSE);
        CsHtmlWebkitPrivate *priv = get_priv(html);
        WebKitFindController *find = webkit_web_view_get_find_controller(priv->webkit);
        if (!text || !*text) {
                webkit_find_controller_search_finish(find);
                g_clear_pointer(&priv->find_text, g_free);
                return FALSE;
        }
        if (g_strcmp0(priv->find_text, text) == 0 &&
            priv->find_match_case == match_case) {
                if (backward)
                        webkit_find_controller_search_previous(find);
                else
                        webkit_find_controller_search_next(find);
                return TRUE;
        }
        g_free(priv->find_text);
        priv->find_text = g_strdup(text);
        priv->find_match_case = match_case;
        WebKitFindOptions options = WEBKIT_FIND_OPTIONS_WRAP_AROUND;
        if (backward) options |= WEBKIT_FIND_OPTIONS_BACKWARDS;
        if (!match_case) options |= WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE;
        webkit_find_controller_search(find, text, options, G_MAXUINT);
        return TRUE; /* WebKit2 reports matches asynchronously. */
}

void cs_html_webkit_increase_size(CsHtmlWebkit *html)
{
        g_return_if_fail(IS_CS_HTML_WEBKIT(html));
        WebKitWebView *view = get_priv(html)->webkit;
        webkit_web_view_set_zoom_level(view, webkit_web_view_get_zoom_level(view) * 1.2);
}

void cs_html_webkit_reset_size(CsHtmlWebkit *html)
{
        g_return_if_fail(IS_CS_HTML_WEBKIT(html));
        webkit_web_view_set_zoom_level(get_priv(html)->webkit, 1.0);
}

void cs_html_webkit_decrease_size(CsHtmlWebkit *html)
{
        g_return_if_fail(IS_CS_HTML_WEBKIT(html));
        WebKitWebView *view = get_priv(html)->webkit;
        webkit_web_view_set_zoom_level(view, webkit_web_view_get_zoom_level(view) / 1.2);
}

gboolean cs_html_webkit_init_system(void) { return TRUE; }
void cs_html_webkit_shutdown_system(void) { }

static void
set_font(CsHtmlWebkit *html, const gchar *font, gboolean monospace)
{
        if (!font || !*font) return;
        PangoFontDescription *desc = pango_font_description_from_string(font);
        const gchar *family = pango_font_description_get_family(desc);
        WebKitSettings *settings = webkit_web_view_get_settings(get_priv(html)->webkit);
        if (family) {
                if (monospace)
                        webkit_settings_set_monospace_font_family(settings, family);
                else
                        webkit_settings_set_default_font_family(settings, family);
        }
        if (pango_font_description_get_set_fields(desc) & PANGO_FONT_MASK_SIZE) {
                gint size = PANGO_PIXELS(pango_font_description_get_size(desc));
                if (size > 0) {
                        if (monospace)
                                webkit_settings_set_default_monospace_font_size(settings, size);
                        else
                                webkit_settings_set_default_font_size(settings, size);
                }
        }
        pango_font_description_free(desc);
}

void cs_html_webkit_set_variable_font(CsHtmlWebkit *html, const gchar *font)
{
        g_return_if_fail(IS_CS_HTML_WEBKIT(html));
        set_font(html, font, FALSE);
}

void cs_html_webkit_set_fixed_font(CsHtmlWebkit *html, const gchar *font)
{
        g_return_if_fail(IS_CS_HTML_WEBKIT(html));
        set_font(html, font, TRUE);
}

void cs_html_webkit_set_charset(CsHtmlWebkit *html, const gchar *charset)
{
        g_return_if_fail(IS_CS_HTML_WEBKIT(html));
        webkit_web_view_set_custom_charset(get_priv(html)->webkit,
                charset && strcmp(charset, "Auto") != 0 ? charset : NULL);
}
