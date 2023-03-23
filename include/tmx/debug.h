#ifndef TMX_DEBUG_H
#define TMX_DEBUG_H

#ifndef TMX_DEBUG
#error Debugging tools are only available when compiled with TMX_DEBUG flag.
#endif

void tmxMemoryLeakCheck(void);

TMXmap *tmxMapLoad(const char *filename);

#endif /* TMX_DEBUG_H */