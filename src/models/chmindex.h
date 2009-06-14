#ifndef CHMSEE_CHMINDEX_H
#define CHMSEE_CHMINDEX_H

#include <glib.h>
#include <glib-object.h>

typedef struct _ChmIndex       ChmIndex;
typedef struct _ChmIndexClass  ChmIndexClass;
typedef struct _ChmIndexPriv   ChmIndexPriv;

#define TYPE_CHMINDEX \
        (chmindex_get_type ())
#define CHMINDEX(o) \
        (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_CHMINDEX, ChmIndex))
#define CHMINDEX_CLASS(k) \
        (G_TYPE_CHECK_CLASS_CAST ((k), TYPE_CHMINDEX, ChmIndexClass))
#define IS_CHMINDEX(o) \
        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_CHMINDEX))
#define IS_CHMINDEX_CLASS(k) \
        (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_CHMINDEX))
#define CHMINDEX_GET_CLASS(o) \
        (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_CHMINDEX, ChmIndexClass))

struct _ChmIndex
{
        GObject         parent;
        ChmIndexPriv* priv;
};

struct _ChmIndexClass
{
	GObjectClass parent_class;
};

GType chmindex_get_type(void);
/**
 * @return NULL if open or parse indexFname failed.
 */
ChmIndex *chmindex_new(const gchar* indexFname, const gchar* encoding);
/** @return GNode<Link> */
GNode* chmindex_get_data(ChmIndex* self);

#endif /* CHMINDEX_H_ */
