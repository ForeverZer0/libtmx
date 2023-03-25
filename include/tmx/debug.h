#ifndef TMX_DEBUG_H
#define TMX_DEBUG_H

#ifndef TMX_DEBUG
#error Debugging tools are only available when compiled with TMX_DEBUG flag.
#endif

void tmxMemoryLeakCheck(void);

#endif /* TMX_DEBUG_H */