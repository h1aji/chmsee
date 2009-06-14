#include "config.h"
#include "chmindex.h"

#include <libxml/parser.h>
#include <libxml/HTMLparser.h>

#include "utils/utils.h"
#include "models/link.h"
#include "models/hhc.h"

#define selfp (self->priv)

struct _ChmIndexPriv {
	GNode* data; /* GNode<Link> */
};

G_DEFINE_TYPE(ChmIndex, chmindex, G_TYPE_OBJECT);

static GObjectClass *parent_class = NULL;

static void chmindex_finalize(GObject* object);

static void
chmindex_class_init(ChmIndexClass *klass)
{
	g_type_class_add_private(klass, sizeof(ChmIndexPriv));
	parent_class = g_type_class_peek_parent(klass);

	G_OBJECT_CLASS(klass)->finalize = chmindex_finalize;
}

static void
chmindex_init(ChmIndex* self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, TYPE_CHMINDEX, ChmIndexPriv);
}

ChmIndex* chmindex_new(const char* filename, const char* encoding) {
	GNode* node = hhc_load(filename, encoding);
	if(node == NULL) {
		return NULL;
	}

	ChmIndex* self = g_object_new(TYPE_CHMINDEX, NULL);
	selfp->data = node;
	return self;
}

GNode* chmindex_get_data(ChmIndex* self) {
	g_return_val_if_fail(IS_CHMINDEX(self), NULL);
	return selfp->data;
}

void chmindex_finalize(GObject* object) {
	ChmIndex* self = CHMINDEX(object);
	hhc_free(selfp->data);
}

