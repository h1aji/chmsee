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

#include "config.h"

#include "string.h"
#include "setup.h"
#include "utils.h"

typedef struct {
        GtkComboBox *combobox;
        const gchar *charset;
} FindCharsetData;

static void on_bookshelf_clear(GtkWidget *, Chmsee *);
static void on_window_close(GtkButton *, Chmsee *);

static void variable_font_set_cb(GtkFontButton *, Chmsee *);
static void fixed_font_set_cb(GtkFontButton *, Chmsee *);
static void cmb_lang_changed_cb(GtkComboBox *, Chmsee *);

static GtkTreeModel *create_lang_model(void);
static void cell_layout_data_func(GtkCellLayout *, GtkCellRenderer *,
                                  GtkTreeModel *, GtkTreeIter *, gpointer);
static gboolean find_charset_func(GtkTreeModel *, GtkTreePath *,
                                  GtkTreeIter *, FindCharsetData *);

static void
on_bookshelf_clear(GtkWidget *widget, Chmsee *chmsee)
{
        const gchar *bookshelf = chmsee_get_bookshelf(chmsee);

        if (bookshelf && g_file_test(bookshelf, G_FILE_TEST_EXISTS)) {
                chmsee_close_book(chmsee);

                gchar *dir = strdup(bookshelf);
                char *argv[] = {"rm", "-rf", dir, NULL};

                g_spawn_async(g_get_tmp_dir(), argv, NULL,
                              G_SPAWN_SEARCH_PATH,
                              NULL, NULL, NULL,
                              NULL);
                g_free(dir);
        }
}

static void
on_window_close(GtkButton *button, Chmsee *chmsee)
{
        gtk_widget_destroy(gtk_widget_get_toplevel(GTK_WIDGET (button)));
}

static void
variable_font_set_cb(GtkFontButton *button, Chmsee *chmsee)
{
        const gchar *font_name = gtk_font_button_get_font_name(button);

        g_debug("SETUP >>> variable font set: %s", font_name);
        chmsee_set_variable_font(chmsee, font_name);
}

static void
fixed_font_set_cb(GtkFontButton *button, Chmsee *chmsee)
{
        const gchar *font_name = gtk_font_button_get_font_name(button);

        g_debug("SETUP >>> fixed font set: %s", font_name);
        chmsee_set_fixed_font(chmsee, font_name);
}

static void
cmb_lang_changed_cb(GtkComboBox *combo_box, Chmsee *chmsee)
{
        GtkTreeIter iter;
        gchar *charset;

        gtk_combo_box_get_active_iter(combo_box, &iter);

        GtkTreeModel *model = gtk_combo_box_get_model(combo_box);

        gtk_tree_model_get(model, &iter,
                           0, &charset,
                           -1);

        g_debug("SETUP >>> select charset: %s", charset);
        if (charset && strlen(charset))
                chmsee_set_charset(chmsee, charset);
        else
                chmsee_set_charset(chmsee, "Auto");
}

static void
startup_lastfile_toggled_cb(GtkWidget *widget, Chmsee *chmsee)
{
        g_debug("SETUP >>> startup_lastfile toggled");
        gboolean state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget));

        chmsee_set_startup_lastfile(chmsee, state);
}

static GtkTreeModel *
create_lang_model(void)
{
        const gchar *region[] = {
                _("Auto"),
                _("West European"),
                _("East European"),
                _("East Asian"),
                _("SE & SW Asian"),
                _("Middle Eastern"),
                _("Unicode")
        };

        const gchar *charset[][2] = {
                {"ISO-8859-1", _("Western (ISO-8859-1)")},  // index = 0
                {"ISO-8859-15", _("Western (ISO-8859-15)")},
                {"IBM850", _("Western (IBM-850)")},
                {"x-mac-roman", _("Western (MacRoman)")},
                {"windows-1252", _("Western (Windows-1252)")},
                {"ISO-8859-14", _("Celtic (ISO-8859-14)")},
                {"ISO-8859-7", _("Greek (ISO-8859-7)")},
                {"x-mac-greek", _("Greek (MacGreek)")},
                {"windows-1253", _("Greek (Windows-1253)")},
                {"x-mac-icelandic", _("Icelandic (MacIcelandic)")},
                {"ISO-8859-10", _("Nordic (ISO-8859-10)")},
                {"ISO-8859-3", _("South European (ISO-8859-3)")},
                {"ISO-8859-4", _("Baltic (ISO-8859-4)")}, // index = 12
                {"ISO-8859-13", _("Baltic (ISO-8859-13)")},
                {"windows-1257", _("Baltic (Windows-1257)")},
                {"IBM852", _("Central European (IBM-852)")},
                {"ISO-8859-2", _("Central European (ISO-8859-2)")},
                {"x-mac-ce", _("Central European (MacCE)")},
                {"windows-1250", _("Central European (Windows-1250)")},
                {"x-mac-croatian", _("Croatian (MacCroatian)")},
                {"IBM855", _("Cyrillic (IBM-855)")},
                {"ISO-8859-5", _("Cyrillic (ISO-8895-5)")},
                {"ISO-IR-111", _("Cyrillic (ISO-IR-111)")},
                {"KOI8-R", _("Cyrillic (KOI8-R)")},
                {"x-mac-cyrillic", _("Cyrillic (MacCyrillic)")},
                {"windows-1251", _("Cyrillic (Windows-1251)")},
                {"IBM866", _("Cyrillic/Russian (CP-866)")},
                {"KOI8-U", _("Cyrillic/Ukrainian (KOI8-U)")},
                {"ISO-8859-16", _("Romanian (ISO-8859-16)")},
                {"x-mac-romanian", _("Romanian (MacRomanian)")},
                {"GB2312", _("Chinese Simplified (GB2312)")}, // index = 30
                {"x-gbk", _("Chinese Simplified (GBK)")},
                {"gb18030", _("Chinese Simplified (GB18030)")},
                {"HZ-GB-2312", _("Chinese Simplified (HZ)")},
                {"ISO-2022-CN", _("Chinese Simplified (ISO-2022-CN)")},
                {"Big5", _("Chinese Traditional (Big5)")},
                {"Big5-HKSCS", _("Chinese Traditional (Big5-HKSCS)")},
                {"x-euc-tw", _("Chinese Traditional (EUC-TW)")},
                {"EUC-JP", _("Japanese (EUC-JP)")},
                {"ISO-2022-JP", _("Japanese (ISO-2022-JP)")},
                {"Shift_JIS", _("Japanese (Shift_JIS)")},
                {"EUC-KR", _("Korean (EUC-KR)")},
                {"x-windows-949", _("Korean (UHC)")},
                {"x-johab", _("Korean (JOHAB)")},
                {"ISO-2022-KR", _("Korean (ISO-2022-KR)")},
                {"armscii-8", _("Armenian (ARMSCII-8)")}, // index = 45
                {"GEOSTD8", _("Georgian (GEOSTD8)")},
                {"TIS-620", _("Thai (TIS-620)")},
                {"ISO-8859-11", _("Thai (ISO-8859-11)")},
                {"windows-874", _("Thai (Windows-874)")},
                {"IBM874", _("Thai (IBM-874)")},
                {"IBM857", _("Turkish (IBM-857)")},
                {"ISO-8859-9", _("Turkish (ISO-8859-9)")},
                {"x-mac-turkish", _("Turkish (MacTurkish)")},
                {"windows-1254", _("Turkish (Windows-1254)")},
                {"x-viet-tcvn5712", _("Vietnamese (TCVN)")},
                {"VISCII", _("Vietnamese (VISCII)")},
                {"x-viet-vps", _("Vietnamese (VPS)")},
                {"windows-1258", _("Vietnamese (Windows-1258)")},
                {"x-mac-devanagari", _("Hindi (MacDevanagari)")},
                {"x-mac-gujarati", _("Gujarati (MacGujarati)")},
                {"x-mac-gurmukh", _("Gurmukhi (MacGurmukhi)")},
                {"ISO-8859-6", _("Arabic (ISO-8859-6)")}, // index = 62
                {"windows-1256", _("Arabic (Windows-1256)")},
                {"IBM864", _("Arabic (IBM-864)")},
                {"x-mac-arabic", _("Arabic (MacArabic)")},
                {"x-mac-farsi", _("Farsi (MacFarsi)")},
                {"ISO-8859-8-I", _("Hebrew (ISO-8859-8-I)")},
                {"windows-1255", _("Hebrew (Windows-1255)")},
                {"ISO-8859-8", _("Hebrew Visual (ISO-8859-8)")},
                {"IBM862", _("Hebrew (IBM-862)")},
                {"x-mac-hebrew", _("Hebrew (MacHebrew)")},
                {"UTF-8", _("Unicode (UTF-8)")}, // index = 72
                {"UTF-16LE", _("Unicode (UTF-16LE)")},
                {"UTF-16BE", _("Unicode (UTF-16BE)")},
                {"UTF-32", _("Unicode (UTF-32)")},
                {"UTF-32LE", _("Unicode (UTF-32LE)")},
                {"UTF-32BE", _("Unicode (UTF-32BE)")} // index = 77
        };

        GtkTreeStore *store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_STRING);

        GtkTreeIter r_iter, c_iter;
        gint i, j, base, top;
        base = top = 0;

        for (i = 0; i < 7; i++) {
                gtk_tree_store_append(store, &r_iter, NULL);
                gtk_tree_store_set(store, &r_iter,
                                   0, "",
                                   1, region[i],
                                   -1);

                if (i == 1) {
                        base = 0;
                        top  = 11;
                } else if (i == 2) {
                        base = 12;
                        top  = 29;
                } else if (i == 3) {
                        base = 30;
                        top  = 44;
                } else if (i == 4) {
                        base = 45;
                        top  = 61;
                } else if (i == 5) {
                        base = 62;
                        top  = 71;
                } else if (i == 6) {
                        base = 72;
                        top  = 77;
                }

                for (j = base; i != 0 && j <= top; j++ ) {
                        gtk_tree_store_append(store, &c_iter, &r_iter);
                        gtk_tree_store_set(store, &c_iter,
                                           0, charset[j][0],
                                           1, charset[j][1],
                                           -1);
                }
        }

        return GTK_TREE_MODEL(store);
}

static void
cell_layout_data_func(GtkCellLayout *layout, GtkCellRenderer *renderer,
                      GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
        gtk_cell_renderer_set_sensitive(renderer, !gtk_tree_model_iter_has_child(model, iter));
}

static gboolean
find_charset_func(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, FindCharsetData *data)
{
        gchar *charset;

        gtk_tree_model_get(model, iter, 0, &charset, -1);

        if (!g_strcmp0(data->charset, charset)) {
                gtk_combo_box_set_active_iter(GTK_COMBO_BOX (data->combobox), iter);
                g_debug("SETUP >>> cmb_lang set active, charset = %s", charset);
                return TRUE;
        }

        return FALSE;
}

void
setup_window_new(Chmsee *chmsee)
{
        g_debug("SETUP >>> create setup window");
        /* create setup window */
        GtkBuilder *builder = gtk_builder_new();
        gtk_builder_add_from_file(builder, RESOURCE_FILE ("setup-window.ui"), NULL);

        GtkWidget *setup_window = BUILDER_WIDGET (builder, "setup_window");

        g_signal_connect_swapped((gpointer) setup_window,
                                 "destroy",
                                 G_CALLBACK (gtk_widget_destroy),
                                 GTK_OBJECT (setup_window));

        /* bookshelf directory */
        GtkWidget *bookshelf_entry = BUILDER_WIDGET (builder, "bookshelf_entry");
        gtk_entry_set_text(GTK_ENTRY(bookshelf_entry), chmsee_get_bookshelf(chmsee));

        GtkWidget *clear_button = BUILDER_WIDGET (builder, "setup_clear");
        g_signal_connect(G_OBJECT (clear_button),
                         "clicked",
                         G_CALLBACK (on_bookshelf_clear),
                         chmsee);

        /* font setting */
        GtkWidget *variable_font_button = BUILDER_WIDGET (builder, "variable_fontbtn");
        g_signal_connect(G_OBJECT (variable_font_button),
                         "font-set",
                         G_CALLBACK (variable_font_set_cb),
                         chmsee);

        GtkWidget *fixed_font_button = BUILDER_WIDGET (builder, "fixed_fontbtn");
        g_signal_connect(G_OBJECT (fixed_font_button),
                         "font-set",
                         G_CALLBACK (fixed_font_set_cb),
                         chmsee);

        /* default lang */
        GtkWidget *cmb_lang = BUILDER_WIDGET (builder, "cmb_default_lang");
        GtkTreeModel *cmb_model = create_lang_model();
        gtk_combo_box_set_model(GTK_COMBO_BOX (cmb_lang), cmb_model);

        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cmb_lang), renderer, FALSE);
        gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(cmb_lang), renderer,
                                       "text", 1,
                                       NULL);
        gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(cmb_lang), renderer,
                                           cell_layout_data_func,
                                           NULL, NULL);

        gtk_font_button_set_font_name(GTK_FONT_BUTTON (variable_font_button),
                                      chmsee_get_variable_font(chmsee));
        gtk_font_button_set_font_name(GTK_FONT_BUTTON (fixed_font_button),
                                      chmsee_get_fixed_font(chmsee));

        const gchar *charset = chmsee_get_charset(chmsee);
        g_debug("SETUP >>> chmsee_get_charset = %s", charset);
        if (charset && strlen(charset) && g_strcmp0(charset, "Auto")) {
                FindCharsetData *data = g_new(FindCharsetData, sizeof(FindCharsetData));

                data->combobox = GTK_COMBO_BOX (cmb_lang);
                data->charset = charset;

                gtk_tree_model_foreach(GTK_TREE_MODEL (cmb_model),
                                       (GtkTreeModelForeachFunc) find_charset_func,
                                       data);
                g_free(data);
        } else {
                gtk_combo_box_set_active(GTK_COMBO_BOX (cmb_lang), 0);
        }

        g_signal_connect(G_OBJECT (cmb_lang),
                         "changed",
                         G_CALLBACK (cmb_lang_changed_cb),
                         chmsee);

        /* startup load lastfile */
        GtkWidget *startup_lastfile_chkbtn = BUILDER_WIDGET (builder, "startup_lastfile_chkbtn");
        g_signal_connect(G_OBJECT (startup_lastfile_chkbtn),
                         "toggled",
                         G_CALLBACK (startup_lastfile_toggled_cb),
                         chmsee);

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (startup_lastfile_chkbtn),
                                     chmsee_get_startup_lastfile(chmsee));

        GtkWidget *close_button = BUILDER_WIDGET (builder, "setup_close");
        g_signal_connect(G_OBJECT (close_button),
                         "clicked",
                         G_CALLBACK (on_window_close),
                         chmsee);

        g_object_unref(builder);
}
