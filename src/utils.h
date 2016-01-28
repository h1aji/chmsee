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

#ifndef __CS_UTILS_H__
#define __CS_UTILS_H__

#include <stdio.h>
#include <glib.h>

G_BEGIN_DECLS

#ifdef ENABLE_NLS
#  include <libintl.h>
#  undef _
#  define _(String) dgettext (PACKAGE, String)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
#  define textdomain(String) (String)
#  define gettext(String) (String)
#  define dgettext(Domain,Message) (Message)
#  define dcgettext(Domain,Message,Type) (Message)
#  define bindtextdomain(Domain,Directory) (Domain)
#  define _(String) (String)
#  define N_(String) (String)
#endif

#define RESOURCE_FILE(file) (CHMSEE_DATA_DIR G_DIR_SEPARATOR_S file)
#define BUILDER_WIDGET(builder, widget) (GTK_WIDGET (gtk_builder_get_object(builder, widget)))

gchar *convert_filename_to_utf8(const gchar *, const gchar *);
gchar *convert_string_to_utf8(const gchar *, const gchar *);
gchar *get_real_uri(const gchar *);
char  *uri_decode(const char*);
gint   ncase_compare_utf8_string(const gchar *, const gchar *);
void   convert_old_config_file(const gchar *, const gchar *);
gchar *file_exist_ncase(const gchar *);

G_END_DECLS

#endif /* !__CS_UTILS_H__ */
