#include "config.h"
#include "chmindex.h"

#include <libxml/parser.h>
#include <libxml/HTMLparser.h>

#include "utils/utils.h"
#include "models/link.h"

#define selfp (self->priv)

struct _ChmIndexPriv {
	GList* data; /* GList<Link> */
};

G_DEFINE_TYPE(ChmIndex, chmindex, G_TYPE_OBJECT);

static GObjectClass *parent_class = NULL;

static void
chmindex_class_init(ChmIndexClass *klass)
{
	g_type_class_add_private(klass, sizeof(ChmIndexPriv));
	parent_class = g_type_class_peek_parent(klass);
}

static void
chmindex_init(ChmIndex* self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, TYPE_CHMINDEX, ChmIndexPriv);
}

enum {
	STATE_OUT,
	STATE_IN_SITEMAP
};


typedef struct {
	int state;
	GList* names; /* GList<gchar*> */
	char* local;

	GList* res; /* GList<Link> */
} SAXContext;

SAXContext* saxcontext_new() {
	return g_new0(SAXContext, 1);
}

void saxcontext_free(SAXContext* self) {
	g_list_foreach(self->names, (GFunc)g_free, NULL);
	g_list_free(self->names);

	g_free(self->local);

	g_list_foreach(self->res, (GFunc)g_free, NULL);
	g_list_free(self->res);

	g_free(self);
}


static void chm_index_start_element (void *ctx,
		const xmlChar *name_,
		const xmlChar **attrs_) {
	const char* name = (const char*) name_;
	const char** attrs = (const char**) attrs_;
	SAXContext* context = (SAXContext*) ctx;

	switch(context->state) {
	case STATE_OUT:
		if(g_ascii_strcasecmp(name, "object") == 0) {
			const char* type = get_attr(attrs, "type");
			if(g_ascii_strcasecmp(type, "text/sitemap") == 0) {
				context->state = STATE_IN_SITEMAP;
			}
		}
		break;
	case STATE_IN_SITEMAP:
		if(g_ascii_strcasecmp(name, "param") == 0) {
			const char* name = get_attr(attrs, "name");
			const char* value = get_attr(attrs, "value");

			if(name != NULL && value != NULL) {
				if(g_ascii_strcasecmp(name, "name") == 0) {
					context->names = g_list_append(context->names, g_strdup(value));
				} else if(g_ascii_strcasecmp(name, "local") == 0) {
					if(context->local != NULL) {
						g_warning("more than one local in one sitemap");
						g_free(context->local);
					}
					context->local = g_strdup(value);
				}
			} else {
				g_warning("name or value is null");
			}
		}
		break;
	default:
		g_return_if_reached();
	}

}

static void chm_index_end_element (void *ctx,
		const xmlChar *name_) {
	const char* name = (const char*) name_;
	SAXContext* context = (SAXContext*) ctx;

	switch(context->state) {
	case STATE_OUT:
		break;
	case STATE_IN_SITEMAP:
		if(g_ascii_strcasecmp(name, "object") == 0) {
			if(context->local != NULL && context->names != NULL) {
				GList* iter;
				for(iter = context->names; iter; iter = iter->next) {
					Link* link = link_new(LINK_TYPE_PAGE, iter->data, context->local);
					context->res = g_list_append(context->res, link);
				}
			}

			g_free(context->local);
			context->local = NULL;
			g_list_foreach(context->names, (GFunc)g_free, NULL);
			g_list_free(context->names);
			context->names = NULL;
		}
		break;
	default:
		g_return_if_reached();
	}
}



static htmlSAXHandler hhSAXHandler = {
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
  NULL, /* startDocument */
  NULL,/* endDocument */
  chm_index_start_element, /* startElement */
  chm_index_end_element, /* endElement */
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

static gint data_sort_func(gconstpointer lhs_, gconstpointer rhs_) {
	Link* lhs = (Link*)lhs_;
	Link* rhs = (Link*)rhs_;

	gint res = ncase_compare_utf8_string(lhs->name, rhs->name);
	if(res == 0) {
		res = ncase_compare_utf8_string(lhs->uri, rhs->uri);
	}
	return res;
}

ChmIndex* chmindex_new(const char* filename, const char* encoding) {
	SAXContext* context = saxcontext_new();

	if(htmlSAXParseFile(filename,
			encoding,
			&hhSAXHandler,
			context) != NULL) {
		g_error("parse %s with encoding %s failed.", filename, encoding);
		return NULL;
	}

	ChmIndex* self = g_object_new(TYPE_CHMINDEX, NULL);
	selfp->data = g_list_sort(context->res, data_sort_func);
	context->res = NULL;

	saxcontext_free(context);
	return self;

}

GList* chmindex_get_data(ChmIndex* self) {
	g_return_val_if_fail(IS_CHMINDEX(self), NULL);
	return selfp->data;
}

