#ifndef TMX_TYPEDEFS_H
#define TMX_TYPEDEFS_H

#define TMX_FALSE 0
#define TMX_TRUE 1

typedef int TMXbool;
typedef int TMXenum;
typedef unsigned int TMXflag;

typedef struct TMXmap TMXmap;
typedef struct TMXcache TMXcache;
typedef struct TMXtileset TMXtileset;
typedef struct TMXtemplate TMXtemplate;
typedef struct TMXproperty TMXproperty;
typedef struct TMXproperties TMXproperties;

typedef union TMXuserptr
{
    void *ptr;
    int id;
    unsigned int uid;
} TMXuserptr;

#endif /* TMX_TYPEDEFS_H */