/*
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

#include <stdio.h>
#include <string.h>

#include "bookmarksfile.h"
#include "utils.h"
#include "models/link.h"

static gchar *strip_string(gchar *);
static gchar *escape_parse(gchar *);
static gint parse_line(gchar *, gchar *, gchar *);
static void save_bookmark(Link *, FILE *);

static gchar *
strip_string(gchar *str)
{
        gint i,j;
        gint c1;

        if (str == NULL)
                return NULL;

        /* count how many leading chars to be whitespace */
        for (i = 0; i < strlen(str); i++) {
                if (str[i] != ' ' && str[i] != '\t' && str[i] != '\r')
                        break;
        }

        /* count how many trailing chars to be whitespace */
        for (j = strlen(str)-1; j >= 0; j--) {
                if (str[j] != ' ' && str[j] != '\t' && str[j] != '\n')
                        break;
        }

        /* string contains only whitespace? */
        if (j < i) {
                str[0] = '\0';

                return str;
        }

        /* now move the chars to the front */
        for (c1 = i; c1 <= j; c1++)
                str[c1-i] = str[c1];

        str[j+1-i] = '\0';

        return str;
}

static gchar *
escape_parse(gchar *str)
{
        gchar tmp[MAXLINE];
        gchar c;
        gint i, j;

        if (str == NULL)
                return NULL;

        j = 0;
        for(i = 0; i < strlen(str); i++) {
                c = str[i];
                if (c == '\\') {
                        i++;
                        switch (str[i]) {
                        case 'n':
                                c = '\n';
                                break;
                        case 't':
                                c = '\t';
                                break;
                        case 'b':
                                c = '\b';
                                break;
                        default:
                                c = str[i];
                        }
                }

                tmp[j] = c;
                j++;
        }

        tmp[j] = '\0';
        strcpy(str, tmp);

        return(str);
}

static gint
parse_line(gchar *iline, gchar *id, gchar *value)
{
        gchar *p,*p2;
        gchar line[1024];
        gchar tmp[1024];

        strcpy(line, iline);
        strcpy(id, "");
        p = strtok(line, "=");

        if (p != NULL) {
                strcpy(id, p); /* got id */
                strip_string(id);
        } else
                return 1;

        strcpy(tmp, "");
        p = strtok(NULL, "");

        if (p != NULL) {
                strcpy(tmp, p); /* string after = */
                strip_string(tmp);
        } else
                return 1;

        /* Now strip quotes from string */
        p  = tmp;
        p2 = *p == '\"' ? p+1 : p;

        if (p[strlen(p)-1] == '\"')
                p[strlen(p)-1] = '\0';

        strcpy(value, p2);

        /* Now reconvert escape-chars */
        escape_parse(value);

        /* All OK */
        return 0;
}

static void
save_bookmark(Link *link, FILE *fd)
{
        fprintf(fd, "%s=%s\n", link->name, link->uri);
        link_free(link);
}

/* External functions */

GList *
cs_bookmarks_file_load(const gchar *path)
{
        g_debug("CS_BOOKMARKS_FILE >>> load bookmarks file = %s", path);

        GList *links = NULL;
        FILE *fd;

        gchar line[MAXLINE];
        gchar id[MAXLINE];
        gchar value[MAXLINE];

        if ((fd = fopen(path, "r")) == NULL) {
                g_debug("CS_BOOKMARKS_FILE >>> Failed to open bookmarks file");
                return NULL;
        }

        while (fgets(line, MAXLINE, fd)) {
                /* Skip empty or hashed lines */
                strip_string(line);

                if (*line == '#' || *line == '\0')
                        continue;

                /* Parse lines */
                if (parse_line(line, id, value)) {
                        g_debug("CS_BOOKMARKS_FILE >>> Syntax error in %s bookmarks file. line: %s", path, line);
                }

                g_debug("CS_BOOKMARKS_FILE >>> add item: name = %s, value = %s", id, value);
                Link *link = link_new(LINK_TYPE_PAGE, g_strdup(id), g_strdup(value));

                links = g_list_prepend(links, link);
        }

        fclose(fd);
        return links;
}

void
cs_bookmarks_file_save(GList *links, const gchar *path)
{
        g_debug("CS_BOOKMARKS_FILE >>> save bookmarks file = %s", path);

        FILE *fd = fopen(path, "w");

        if (fd == NULL) {
                g_warning("CS_BOOKMARKS_FILE >>> Faild to save bookmarks file: %s", path);
                return;
        }

        g_list_foreach(links, (GFunc)save_bookmark, fd);
        g_list_free(links);

        fclose(fd);
}
