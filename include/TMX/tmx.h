#ifndef TMX_H
#define TMX_H

#include "TMX/typedefs.h"

struct TMXpoint
{
    int x;
    int y;
};

struct TMXsize
{
    int w;
    int h;
};

struct TMXvec2
{
    float x;
    float y;
};

struct TMXmap
{
    const char *name;
};



#define tmxUserPtr(ptr) ((TMXuserptr){ptr})
#define tmxNullUserPtr tmxUserPtr(NULL)

#endif /* TMX_H */