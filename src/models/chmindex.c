#include "config.h"
#include "chmindex.h"

#include <libxml/parser.h>
#include <libxml/HTMLparser.h>

struct _ChmIndexPriv {

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

typedef struct {

} SAXContext;

/**
 * TODO
 */
static void chm_index_start_element (void *ctx,
		const xmlChar *name,
		const xmlChar **atts) {
}

/**
 * TODO
 */
static void chm_index_end_element (void *ctx,
		const xmlChar *name) {

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


/**
 * TODO:
 */
ChmIndex* chmindex_new(const char* filename, const char* encoding) {
	SAXContext context;

	if(htmlSAXParseFile(filename,
			encoding,
			&hhSAXHandler,
			&context) != NULL) {
		g_error("parse %s with encoding %s failed.", filename, encoding);
		return NULL;
	}

	ChmIndex* self = g_object_new(TYPE_CHMINDEX, NULL);
	return self;

}
