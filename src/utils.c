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

#include "config.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

gchar *
convert_filename_to_utf8(const gchar *filename, const gchar *codeset)
{
        gchar * filename_utf8;

        g_debug("UTILS >>> Convert filename to UTF8.");

        if (g_utf8_validate(filename, -1, NULL)) {
                filename_utf8 = g_strdup(filename);
        } else {
                filename_utf8 = g_filename_to_utf8(filename, -1, NULL, NULL, NULL);

                if (filename_utf8 == NULL)
                        filename_utf8 = g_convert(filename, -1, "UTF-8",
                                                  codeset,
                                                  NULL, NULL, NULL);
        }

        return filename_utf8;
}

gchar *
convert_string_to_utf8(const gchar *string, const gchar *codeset)
{
        gchar * string_utf8;

        g_debug("UTILS >>> Convert string to UTF8.");
        if (g_utf8_validate(string, -1, NULL)) {
                g_debug("UTILS >>> string is utf8");
                string_utf8 = g_strdup(string);
        } else {
                g_debug("UTILS >>> string is not utf8");
                string_utf8 = g_convert(string, -1, "UTF-8",
                                        codeset,
                                        NULL, NULL, NULL);
        }

        return string_utf8;
}

gint
ncase_compare_utf8_string(const gchar *str1, const gchar *str2)
{
        gint result;
        gchar *ncase_str1, *ncase_str2;
        gchar *normalized_str1, *normalized_str2;

        ncase_str1 = g_utf8_casefold(str1, -1);
        ncase_str2 = g_utf8_casefold(str2, -1);

        normalized_str1 = g_utf8_normalize(ncase_str1, -1, G_NORMALIZE_DEFAULT);
        normalized_str2 = g_utf8_normalize(ncase_str2, -1, G_NORMALIZE_DEFAULT);

        result = g_utf8_collate(normalized_str1, normalized_str2);

        g_free(ncase_str1);
        g_free(ncase_str2);
        g_free(normalized_str1);
        g_free(normalized_str2);

        return result;
}

char *
uri_decode(const char *encoded)
{
        const char *at = encoded;
        int length = 0;
        char *rv;
        char *out;

        while (*at != '\0') {
                if (*at == '%') {
                        if (at[1] == '\0' || at[2] == '\0') {
                                g_warning ("malformed URL encoded string");
                                return NULL;
                        }
                        at += 3;
                        length++;
                } else {
                        at++;
                        length++;
                }
        }

        rv = g_new(char, length + 1);
        out = rv;
        at = encoded;

        while (*at != '\0') {
                if (*at == '%') {
                        char hex[3];
                        hex[0] = at[1];
                        hex[1] = at[2];
                        hex[2] = '\0';
                        if (at[1] == '\0' || at[2] == '\0')
                                return NULL;
                        at += 3;
                        *out++ = (char) strtol(hex, NULL, 16);
                } else {
                        *out++ = *at++;
                        length++;
                }
        }

        *out = '\0';
        return rv;
}

/* Remove '#', ';' fragment in uri */
gchar *
get_real_uri(const gchar *uri)
{
        gchar *real_uri;
        gchar *p = NULL;

        p = g_strrstr(uri, "#");
        real_uri = p ? g_strndup(uri, p - uri) : g_strdup(uri);

        p = g_strrstr(real_uri, ";");

        if (p != NULL) {
                g_free(real_uri);
                real_uri = g_strndup(real_uri, p - real_uri);
        }

        return real_uri;
}

void
convert_old_config_file(const gchar *path, const gchar *groupname)
{
        FILE *fd_old, *fd_new;

        gchar *dir = g_path_get_dirname(path);
        gchar *new_conf = g_build_filename(dir, "config.new", NULL);

        if ((fd_old = fopen(path, "r")) && (fd_new = fopen(new_conf, "w"))) {
                fputs(groupname, fd_new);

                gchar line[MAXLINE];
                while (fgets(line, MAXLINE, fd_old)) {
                        fputs(line, fd_new);
                }
                fclose(fd_old);
                fclose(fd_new);

                rename(new_conf, path);
        }

        g_free(dir);
        g_free(new_conf);
}

gchar *
file_exist_ncase(const gchar *path)
{
        if (g_file_test(path, G_FILE_TEST_EXISTS)) {
                g_debug("UTILS >>> file_exist_ncase found path: %s", path);
                return g_strdup(path);
        }

        gchar *old_dir = g_path_get_dirname(path);
        gchar *dirname = file_exist_ncase(old_dir);
        if (dirname == NULL) {
                g_free(old_dir);
                return NULL;
        }

        /* check new dirname with basename */
        gchar *filename = g_path_get_basename(path);
        gchar *newfile = g_strdup_printf("%s/%s", dirname, filename);
        if (g_file_test(newfile, G_FILE_TEST_EXISTS)) {
                g_debug("UTILS >>> file_exist_ncase found newfile: %s", newfile);
                g_free(old_dir);
                g_free(dirname);
                g_free(filename);
                return newfile;
        }

        gchar *found = NULL;
        GDir *dir = g_dir_open(dirname, 0, NULL);
        if (dir != NULL) {
                const gchar *entry;

                while ((entry = g_dir_read_name(dir))) {
                        if (!g_ascii_strcasecmp(filename, entry)) {
                                g_debug("UTILS >>> found case insensitive file: %s", entry);
                                found = g_strdup_printf("%s/%s", dirname, entry);
                                g_dir_close(dir);

                                break;
                        }
                }
        }

        g_free(old_dir);
        g_free(dirname);
        g_free(filename);

        g_debug("UTILS >>> return found file: %s", found);
        return found;
}
