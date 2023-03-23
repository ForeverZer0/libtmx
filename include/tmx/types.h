#ifndef TMX_TYPEDEFS_H
#define TMX_TYPEDEFS_H

#include <stddef.h>
#include <stdint.h>

#define TMX_FALSE       0
#define TMX_TRUE        1
#define TMX_UNSPECIFIED 0
#define TMX_NONE        0

#ifdef TMX_PACKED_COLOR
#define TMX_COLOR_T TMXcolor
#else
#define TMX_COLOR_T TMXcolorf
#endif

// TODO
#define TMX_INLINE __inline__

#define TMX_PROPERTY_STRING  1 /** A string value. */
#define TMX_PROPERTY_INTEGER 2 /** An integer value. */
#define TMX_PROPERTY_FLOAT   3 /** A floating point value. */
#define TMX_PROPERTY_BOOL    4 /** An integral boolean value that is either TMX_TRUE or TMX_FALSE. */
#define TMX_PROPERTY_COLOR   5 /** A color value. */
#define TMX_PROPERTY_FILE    6 /** A string value that should be interpreted as a filesystem path. */
#define TMX_PROPERTY_OBJECT  7 /** An integer value that corresponds to the ID of a map object. */
#define TMX_PROPERTY_CLASS   8 /** A custom property type with child properties. */

#define TMX_ORIENTATION_ORTHOGONAL 1
#define TMX_ORIENTATION_ISOMETRIC  2
#define TMX_ORIENTATION_STAGGERED  3
#define TMX_ORIENTATION_HEXAGONAL  4

#define TMX_RENDER_RIGHT_DOWN 0 /** From left-to-right, top-to-bottom. */
#define TMX_RENDER_RIGHT_UP   1 /** From left-to-right, bottom-to-top. */
#define TMX_RENDER_LEFT_DOWN  2 /** From right-to-left, top-to-bottom. */
#define TMX_RENDER_LEFT_UP    3 /** From right-to-left, bottom-to-top. */

#define TMX_STAGGER_AXIS_X 1
#define TMX_STAGGER_AXIS_Y 2

#define TMX_STAGGER_INDEX_EVEN 1
#define TMX_STAGGER_INDEX_ODD  2

#define TMX_GID_FLIP_HORIZONTAL 0x80000000U /** Bit-flag indicating a GID is flipped horizontally. */
#define TMX_GID_FLIP_VERTICAL   0x40000000U /** Bit-flag indicating a GID is flipped vertically. */
#define TMX_GID_FLIP_DIAGONAL   0x20000000U /** Bit-flag indicating a GID is flipped diagonally. */
#define TMX_GID_ROTATE_120      0x10000000U /** Bit-flag indicating a hexagonal GID is rotated 120 degrees. */
#define TMX_GID_MASK            0x0FFFFFFFU /** Bit-mask to clear the flip/rotate bits from a GID. */

#define TMX_FLAG_NONE     0x0000 /** No additional flags. */
#define TMX_FLAG_EMBEDDED 0x0001 /** Indicates the current object is embedded and not in an external file. */
#define TMX_FLAG_EXTERNAL 0x0002 /** Indicates the current object is loaded from an external file. */
#define TMX_FLAG_CACHED   0x0004 /** Indicates the item is cached within a cache object. */
#define TMX_FLAG_COLOR    0x0008 /** Context varies, but indicates that a color field is explicitly defined. (i.e. color, tint_color, etc) */
#define TMX_FLAG_NAME     0x0010 /** For map objects, indicates that the `name` field has been explicitly defined. */
#define TMX_FLAG_POSITION 0x0020 /** For map objects, indicates that the `position` field has been explicitly defined. */
#define TMX_FLAG_SIZE     0x0040 /** For map objects, indicates that the `size` field has been explicitly defined. */
#define TMX_FLAG_ROTATION 0x0080 /** For map objects, indicates that the `rotation` field has been explicitly defined. */
#define TMX_FLAG_GID      0x0100 /** For map objects, indicates that the `gid` field has been explicitly defined. */
#define TMX_FLAG_VISIBLE  0x0200 /** For map objects, indicates that the `visible` field has been explicitly defined. */
#define TMX_FLAG_POINTS   0x0400 /** For map objects, indicates that the `points` field has been explicitly defined. */
#define TMX_FLAG_TEXT     0x0800 /** For map objects, indicates that the `text` field has been explicitly defined. */

#define TMX_LAYER_TILE     1 /** A tile layer with tile data. */
#define TMX_LAYER_CHUNK    2 /** A tile layer for an infinite map and chunked tile data. */
#define TMX_LAYER_OBJGROUP 3 /** A layer with a collection of map objects. */
#define TMX_LAYER_IMAGE    4 /** A layer with a single image. */
#define TMX_LAYER_GROUP    5 /** A container of child layers. Its offset, tint, opacity, and visibility recursively affect child layers.*/

#define TMX_ALIGN_LEFT     0x01                                      /** Left-aligned (x-axis). */
#define TMX_ALIGN_RIGHT    0x02                                      /** Right-aligned (x-axis).*/
#define TMX_ALIGN_TOP      0x04                                      /** Top-aligned (y-axis).*/
#define TMX_ALIGN_BOTTOM   0x08                                      /** Bottom-aligned (y-axis).*/
#define TMX_ALIGN_CENTER_H (TMX_ALIGN_LEFT | TMX_ALIGN_RIGHT)        /** Centered on x-axis. */
#define TMX_ALIGN_CENTER_V (TMX_ALIGN_TOP | TMX_ALIGN_BOTTOM)        /** Centered on y-axis. */
#define TMX_ALIGN_CENTER   (TMX_ALIGN_CENTER_H | TMX_ALIGN_CENTER_V) /** Centered on both x and y axis. */

#define TMX_FONT_STYLE_NONE      0x00
#define TMX_FONT_STYLE_BOLD      0x01
#define TMX_FONT_STYLE_ITALIC    0x02
#define TMX_FONT_STYLE_UNDERLINE 0x04
#define TMX_FONT_STYLE_STRIKEOUT 0x08

#define TMX_OBJECT_RECT     0 /** */
#define TMX_OBJECT_ELLIPSE  1 /** */
#define TMX_OBJECT_POINT    2 /** */
#define TMX_OBJECT_POLYGON  3 /** */
#define TMX_OBJECT_POLYLINE 4 /** */
#define TMX_OBJECT_TEXT     5 /** */

#define TMX_DRAW_TOPDOWN 0
#define TMX_DRAW_INDEX   1

/**
 * @brief An integral value that represents a boolean true/false.
 */
typedef int32_t TMXbool;

/**
 * @brief An enumeration type whose value is defined as a strongly-typed constant.
 */
typedef int32_t TMXenum;

/**
 * @brief An enumeration type supporting bit-flags that can be OR'ed together.
 */
typedef uint32_t TMXflag;

/**
 * @brief Describes a global tile ID, which may also be tainted with bits indicating flip/rotation.
 *
 * @sa @ref TMX_GID_CLEAN
 * @sa @ref TMX_GID_FLAGS
 */
typedef uint32_t TMXgid;

/**
 * @brief Represents a dimension in 2D coordinate space.
 */
typedef struct TMXsize
{
    int w; /** The dimension on the horizontal plane. */
    int h; /** The dimension on the vertical plane. */
} TMXsize;

/**
 * @brief Represents an ordered pair of x and y coordinates that define a point in a two-dimensional plane.
 */
typedef struct TMXpoint
{
    int x; /** The coordinate on the horizontal plane. */
    int y; /** The coordinate on the vertical plane. */
} TMXpoint;

/**
 * @brief A structure encapsulating two single-precision floating point values.
 */
typedef struct TMXvec2
{
    float x; /** The x component of the vector. */
    float y; /** The y component of the vector. */
} TMXvec2;

/**
 * @brief A color represented as ARGB in a packed integer value, with component values ranging from 0 to 255.
 *
 * @note Compile with @c TMX_PACKED_COLOR to have this be the default color structure used by the library.
 */
typedef union TMXcolor
{
    struct
    {
        uint8_t a; /** The value of the alpha component. */
        uint8_t r; /** The value of the red component. */
        uint8_t g; /** The value of the green component. */
        uint8_t b; /** The value of the blue component. */
    };
    uint32_t value; /** The packed color value as an unsigned integer. */
} TMXcolor;

/**
 * @brief A color represented as RGBA with normalized float component values ranging from 0.0 to 1.0.
 *
 * @note The is the default type used for colors by the library. Compile with the @c TMX_PACKED_COLOR flag
 * to have integral @ref TMXcolor be the default.
 */
typedef struct TMXcolorf
{
    float r; /** The value of the red component. */
    float g; /** The value of the green component. */
    float b; /** The value of the blue component. */
    float a; /** The value of the alpha component. */
} TMXcolorf;

/**
 * @brief A pointer-sized structure containing a user-defined value.
 */
typedef union TMXuserptr
{
    void *ptr;    /** The value as a pointer. */
    int32_t id;   /** The value as an integer. */
    uint32_t uid; /** The value as an unsigned integer (e.g. an OpenGL name). */
} TMXuserptr;

/**
 * @brief Opaque type that maintains references to reusable TMX types including tilesets, templates, and images. This allows them to be
 * used without loading and parsing them each time they are referenced between map loads.
 */
typedef struct TMXcache TMXcache;

/**
 * @brief Opaque type that stores property values in a dictionary-like structure.
 */
typedef struct TMXproperties TMXproperties;

/**
 * @brief Describes a named user-defined value.
 */
typedef struct TMXproperty
{
    const char *name;        /** The name of the property. */
    const char *custom_type; /** The custom type of the property. */
    TMXenum type;            /** Indicates the type of the property, and which value field to reference. */
    union                    /** A union containing the value of the property. Use the type to determine which field to use. */
    {
        const char *string;        /** A string value, valid with TMX_PROPERTY_STRING and TMX_PROPERTY_FILE. */
        int integer;               /** An integer value, valid with TMX_PROPERTY_INTEGER, TMX_PROPERTY_OBJECT, and TMX_PROPERTY_BOOL. */
        float decimal;             /** A floating point value, valid with TMX_PROPERTY_FLOAT. */
        TMX_COLOR_T color;         /** A color value, valid with TMX_PROPERTY_COLOR. */
        TMXproperties *properties; /** Child properties, valid with TMX_PROPERTY_CLASS. */
    } value;
    struct TMXproperty *next; /** Access the next property in a linked-list fashion. */
    TMXuserptr user;          /** User-defined value that can be attached to this object. Will never be modified by this library. */
} TMXproperty;

/**
 * @brief Describes an image that is used for a map, tileset, or object.
 */
typedef struct TMXimage
{
    TMXflag flags;           /** Flags providing additional information about the image. */
    const char *format;      /** For embedded images, indicates the image type. */
    const char *source;      /** For external images, indicates the relative path to the source file. */
    TMXsize size;            /** Optional size of the image in pixel units. */
    TMX_COLOR_T transparent; /** A specific color that is treated as transparent. */
    void *data;              /** For embedded images, contains the image data. */
    TMXuserptr user_data;    /** When using the callback-driven image loader, contains the data returned from the callback. */
    TMXuserptr user;         /** User-defined value that can be attached to this object. Will never be modified by this library. */
} TMXimage;

/**
 * @brief Prototype for callbacks to load/process images as they are created.
 * @param[in] source The image instance.
 * @param[in] basePath The base path that @a source is relative to.
 * @param[in] user An arbitrary user value that was specified when setting the callback.
 * @return A user-defined pointer containing pertinent image data that will be stored with the image object.
 */
typedef TMXuserptr (*TMXimageloadfunc)(TMXimage *image, const char *basePath, TMXuserptr user);

/**
 * @brief Prototype for callbacks to free user-loaded images.
 * @param[in] data The user data that was returned from the load callback.
 * @param[in] user An arbitrary user value that was specified when setting the callback.
 */
typedef void (*TMXimagefreefunc)(TMXuserptr data, TMXuserptr user);

typedef struct TMXchunk
{
    TMXpoint position; /** The coordinate of the chunk in tile units. */
    TMXsize size;      /** The dimensions of the chunk in tile units. */
    size_t count;      /** The number of global tile IDs in the `gids` array. */
    TMXgid *gids;      /** An array of global tile IDs. */
} TMXchunk;

struct TMXtext
{
    const char *font;   /** The font family used (defaults to “sans-serif”) when NULL. */
    int pixel_size;     /** The size of the font in pixel units. */
    TMXbool wrap;       /** Indicates whether word wrapping is enabled. */
    TMX_COLOR_T color;  /** The color of the text. */
    TMXflag style;      /** Bit-flags describing the font style(s). */
    TMXbool kerning;    /** Indicates whether kerning should be used while rendering the text. */
    TMXflag align;      /** Bit-flags describing how the text should be aligned. */
    const char *string; /** The string contents of the text to render. */
    TMXuserptr user;    /** User-defined value that can be attached to this object. Will never be modified by this library. */
};

/**
 * @brief Container for the points of an arbitrary shape.
 */
struct TMXcoords
{
    size_t count;    /** The number of points in the shape. */
    TMXvec2 *points; /** An array of vectors defining the position of the each point on the x and y axis. */
};

typedef struct TMXtemplate TMXtemplate;

typedef struct TMXobject
{
    TMXflag flags;         /** Flags providing additional information about the object. Useful to indicate explicit/default fields. */
    int id;                /** The unique ID of the object. */
    TMXenum type;          /** Enumeration value describing the object type. */
    const char *name;      /** The name of the object. An arbitrary string. */
    const char *class;     /** The class of the object. An arbitrary string. */
    TMXvec2 position;      /** The coordinate of the object in pixel units. */
    TMXvec2 size;          /** The dimensions of the object in pixel units.*/
    float rotation;        /** The rotation of the object in degrees clockwise around the object's position. */
    TMXgid gid;            /** An optional reference to a tile, or 0 when not defined. */
    TMXbool visible;       /** Indicates when ther object is shown or hidden. */
    TMXtemplate *template; /** An optional reference to a template file. */
    union
    {
        struct TMXcoords points; /** The points list. Applicable only when `type` is TMX_OBJECT_POLYGON or TMX_OBJECT_POLYLINE. */
        struct TMXtext *text;    /** The text object. Applicable only when `type` is TMX_OBJECT_TEXT. */
    };
    struct TMXobject *next;    /** When object is a child of an object group, points to the next object in the linked-list. */
    TMXproperties *properties; /** Named property hash/dictionary containing arbitrary values. */
    TMXuserptr user;           /** User-defined value that can be attached to this object. Will never be modified by this library. */
} TMXobject;

/**
 * @brief Describes a layer within a map.
 */
typedef struct TMXlayer
{
    TMXflag flags;          /** Flags providing additional information about the layer. */
    TMXenum type;           /** Enumeration value describing the kind of layer this is and which data field is valid. */
    int id;                 /** The unique ID of the layer. */
    const char *name;       /** The name of the layer. */
    const char *class;      /** The class of the layer. */
    TMXpoint position;      /** The layer of the layer in tile units. Defaults to 0 and can not be changed in Tiled. */
    TMXsize size;           /** The size of the layer in tile units. Always the same as the map size for fixed-size maps. */
    float opacity;          /** The opacity of the layer as a value in the range of 0.0 to 1.0 inclusive. */
    TMXbool visible;        /** Indicates if the layer is shown od hidden. */
    TMX_COLOR_T tint_color; /** A optional tint color that is multiplied with any tiles drawn by this layer. */
    TMXpoint offset;        /** The offset for this layer in pixel units. */
    TMXvec2 parallax;       /** The parallax factor for this layer. */
    size_t count;           /** Indicates the number of items in the data array. Not applicable when type is TMX_LAYER_IMAGE. */
    union /** Union of values relevant only for a specific kind of layer. Use the `type` value to indicate which field is applicable. */
    {
        TMXgid *tiles;          /** An array of global tile IDS. Applicable when the layer type is TMX_LAYER_TILE. */
        TMXchunk *chunks;       /** Array of chunks. Applicable when the layer type is TMX_LAYER_CHUNK. */
        TMXimage *image;        /** The layer image. Applicable when the layer type is TMX_LAYER_IMAGE. */
        TMXobject *objects;     /** The layer's child objects. Applicable when the layer type is TMX_LAYER_OBJGROUP. */
        struct TMXlayer *group; /** The head of the child layers linked-list. Applicable when the the layer type is TMX_LAYER_GROUP. */
    } data;
    struct /** Indicates whether the image for this layer repeats. Applicable when the layer type is TMX_LAYER_IMAGE. */
    {
        TMXbool x; /** Indicates whether the image drawn by this layer is repeated along the x-axis. */
        TMXbool y; /** Indicates whether the image drawn by this layer is repeated along the y-axis. */
    } repeat;
    TMXenum draw_order; /** Indicates the order in which objects should be drawn. Applicable when the layer type is TMX_LAYER_OBJGROUP. */
    TMXproperties *properties; /** Named property hash/dictionary containing arbitrary values. */
    TMXuserptr user;           /** User-defined value that can be attached to this object. Will never be modified by this library. */
    struct TMXlayer *next;     /** The next layer defined in the map, or NULL if this is the topmost layer. */
} TMXlayer;

/*


*/




typedef struct TMXmap TMXmap;
typedef struct TMXtileset TMXtileset;

struct TMXtemplate
{
    TMXflag flags;
    TMXtileset *tileset;
    TMXobject *object;
};

#endif /* TMX_TYPEDEFS_H */