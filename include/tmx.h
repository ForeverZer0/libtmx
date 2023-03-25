#ifndef TMX_H
#define TMX_H

#include "tmx/types.h"

#define TMX_GID_CLEAN(x) ((x) &TMX_GID_MASK)
#define TMX_GID_FLAGS(x) ((x) & ~TMX_GID_MASK)

/**
 * @brief Assigns callbacks that will be invoked when a TMX image is loaded and needs freed.
 *
 * This can be used configure an automatic load/free process for map resources, such as creating
 * required textures to render a map, and then freeing them when no longer needed.
 *
 * @param load A load callback that will be called when a TMX image is loaded.
 * @param free A free callback that will be called when an image is no longer needed.
 * @param user An arbitrary user pointer that will be passed to the callbacks when invoked. Will never be modified by this library.
 */
void tmxSetImageCallbacks(TMXimageloadfunc load, TMXimagefreefunc free, TMXuserptr user);

TMXmap *tmxLoadMapXml(const char *filename, TMXcache *cache, size_t bufferSize);

TMXtileset *tmxLoadTilesetXml(const char *filename, TMXcache *cache, size_t bufferSize);

TMXtemplate *tmxLoadTemplateXml(const char *filename, TMXcache *cache, size_t bufferSize);

void tmxSetPathResolveCallback(TMXpathfunc callback, TMXuserptr user);

#define tmxUserPtr(ptr) ((TMXuserptr){ptr})
#define tmxNullUserPtr  tmxUserPtr(NULL)

#endif /* TMX_H */