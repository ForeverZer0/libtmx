#ifndef TMX_H
#define TMX_H

#include "tmx/types.h"



#define TMX_GID_CLEAN(x) ((x) & TMX_GID_MASK)
#define TMX_GID_FLAGS(x) ((x) & ~TMX_GID_MASK) 

void tmxSetImageCallbacks(TMXimageloadfunc load, TMXimagefreefunc free, TMXuserptr user);

struct TMXmap
{
    TMXflag flags;
    const char *version;       /** The TMX format version. */
    const char *tiled_version; /** The Tiled version used to save the file. */
    const char *class;         /** The class of this map. */
    TMXenum orientation;       /** The orientation of the map. */
    TMXenum render_order;      /** The order in which tiles on tile layers are rendered. */
    struct TMXsize size;       /** The map size in tile units. */
    struct TMXsize tile_size;  /** The size of map tiles in pixel units. */
    int hex_side;              /** For hexagonal maps, the width or height (depending on axis) of the tile’s edge, in pixel units. */
    struct                     /** Determines how staggered and hexagonal maps are drawn. */
    {
        TMXenum axis;  /** Determines which axis is staggered. */
        TMXenum index; /** Determines whether the even or odd indices along the staggered axis are shifted. */
    } stagger;
    struct TMXvec2 parallax_origin; /** The parallax origin in pixel units. */
    TMX_COLOR_T background_color;   /** The background color of the map. The TMX_FLAG_COLOR flag will be set when defined. */
    TMXbool infinite;               /** Indicates whether map is infinite and its data is stored as chunks. */
    TMXproperties *properties;      /** Named property hash/dictionary containing arbitrary values. */

    size_t count;
    TMXlayer *layers;
};

#define tmxUserPtr(ptr) ((TMXuserptr){ptr})
#define tmxNullUserPtr  tmxUserPtr(NULL)

#endif /* TMX_H */