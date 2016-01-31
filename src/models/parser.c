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

#include "config.h"

#include <libxml/parser.h>
#include <libxml/HTMLparser.h>

#include "parser.h"
#include "utils.h"
#include "models/link.h"

static gint     depth = -1;
static gint     prev_depth = -1;
static gboolean tree_item = FALSE;
static gint     item_count = 0;
static gchar   *title[MAXLINE];
static gchar   *local[MAXLINE];
static GNode   *parent = NULL;
static GNode   *prev_node = NULL;

static const gchar *get_attr(const gchar **, const gchar *);
static void startDocumentHH(void *);
static void endDocumentHH(void *);
static void startElementHH(void *, const xmlChar *, const xmlChar **);
static void endElementHH(void *, const xmlChar *);

static xmlSAXHandler hhSAXHandlerStruct = {
        NULL, /* internalSubset */
        NULL, /* isStandalone */
        NULL, /* hasInternalSubset */
        NULL, /* hasExternalSubset */
        NULL, /* resolveEntity */
        NULL, /* getEntity */
        NULL, /* entityDecl */
        NULL, /* notationDecl */
        NULL, /* attributeDecl */
        NULL, /* elementDecl */
        NULL, /* unparsedEntityDecl */
        NULL, /* setDocumentLocator */
        startDocumentHH, /* startDocument */
        endDocumentHH, /* endDocument */
        startElementHH, /* startElement */
        endElementHH, /* endElement */
        NULL, /* reference */
        NULL, /* characters */
        NULL, /* ignorableWhitespace */
        NULL, /* processingInstruction */
        NULL, /* comment */
        NULL, /* xmlParserWarning */
        NULL, /* xmlParserError */
        NULL, /* xmlParserError */
        NULL, /* getParameterEntity */
        NULL, /* cdataBlock */
        NULL, /* externalSubset */
        1,    /* initialized */
        NULL, /* private */
        NULL, /* startElementNsSAX2Func */
        NULL, /* endElementNsSAX2Func */
        NULL  /* xmlStructuredErrorFunc */
};

static xmlSAXHandlerPtr hhSAXHandler = &hhSAXHandlerStruct;

static const gchar *
get_attr(const gchar **attrs, const gchar *key)
{
        while (*attrs) {
                if (g_ascii_strcasecmp(*attrs, key) == 0) {
                        return *(attrs+1);
                }
                attrs += 2;
        }
        return NULL;
}

static void
startDocumentHH(void *ctx)
{
        g_debug("CS_PARSER >>> SAX.startDocument()");
}

static void
endDocumentHH(void *ctx)
{
        g_debug("CS_PARSER >>> SAX.endDocument()");
}

static void
startElementHH(void *ctx, const xmlChar *name_, const xmlChar **atts_)
{
        const gchar  *name = (const gchar  *)name_;
        const gchar **atts = (const gchar **)atts_;

        /* g_debug("SAX.startElement(%s)", name); */

        if (g_ascii_strcasecmp("ul", name) == 0) {
                depth++;
        } else if (g_ascii_strcasecmp("object", name) == 0) {
                const gchar* type = get_attr(atts, "type");
                if (type && g_ascii_strcasecmp("text/sitemap", type) == 0) {
                        tree_item = TRUE;
                }
        } else if (g_ascii_strcasecmp("param", name) == 0) {
                const gchar *param_name = get_attr(atts, "name");
                const gchar *param_value = get_attr(atts, "value");

                if (param_name == NULL || param_value == NULL) {
                        return;
                }

                if (tree_item) {
                        if (g_ascii_strcasecmp("Keyword", param_name) == 0) {
                                title[item_count] = g_strdup(param_value);
                        } else if (g_ascii_strcasecmp("Name", param_name) == 0) {
                                if (!title[item_count])
                                        title[item_count] = g_strdup(param_value);
                        } else if (g_ascii_strcasecmp("Local", param_name) == 0) {
                                local[item_count] = g_strdup(param_value);
                        }

                        if (title[item_count] && local[item_count])
                                item_count++;
                }
        }
}

static void
endElementHH(void *ctx, const xmlChar *name_)
{
        const gchar *name = (const gchar *)name_;

        GNode *link_tree = (GNode *)ctx;
        GNode *node;
        Link  *link;

        /* g_debug("SAX.endElement(%s)", name); */

        if (g_ascii_strcasecmp("ul", name) == 0) {
                depth--;
        } else if (g_ascii_strcasecmp("object", name) == 0) {
                if (!tree_item)
                        return;

                /* g_debug("prev_depth = %d", prev_depth); */
                /* g_debug("depth = %d", depth); */

                if (depth == 0) {
                        parent = link_tree;
                } else {
                        if (depth > prev_depth)
                                parent = prev_node ? prev_node : link_tree;
                        else
                                for (; depth < prev_depth; prev_depth--)
                                        parent = parent->parent;
                }

                if (item_count == 0 && title[0]) /* Manually increase no link item */
                        item_count = 1;

                gint i = 0;
                for (; i < item_count; i++) {
                        if (title[i] == NULL)
                                title[i] = g_strdup( _("No Title"));
                        if (local[i] == NULL)
                                local[i] = g_strdup(CHMSEE_NO_LINK);

                        /* g_debug("title[%d] = %s", i, title[i]); */
                        /* g_debug("local[%d] = %s", i, local[i]); */

                        link = link_new(LINK_TYPE_PAGE, title[i], local[i]);
                        node = g_node_new(link);

                        g_node_append(parent, node);
                        prev_node = node;

                        g_free(title[i]);
                        g_free(local[i]);

                        title[i] = local[i] = NULL;
                }

                prev_depth = depth;
                tree_item = FALSE;
                item_count = 0;
        }
}

GNode *
cs_parse_file(const gchar *filename, const gchar *encoding)
{
        htmlDocPtr doc = NULL;
        GNode    *tree = g_node_new(NULL);

        depth = -1;
        prev_depth = -1;
        tree_item = FALSE;
        item_count = 0;
        parent = NULL;
        prev_node = NULL;

        doc = htmlSAXParseFile(filename,
                               encoding,
                               hhSAXHandler,
                               tree);

        if (doc != NULL) {
                g_warning("CS_PARSER >>> htmlSAXParseFile returned non-NULL");
                xmlFreeDoc(doc);
        }

        g_debug("CS_PARSER >>> Parsing file %s finished", filename);

        return tree;
}
