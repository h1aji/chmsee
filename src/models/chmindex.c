#include "config.h"
#include "chmindex.h"

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

