#ifndef TMX_H
#define TMX_H

#include "tmx/common.h"
#include <stddef.h>

#define TMX_GID_FLIP_HORIZONTAL 0x80000000U /** Bit-flag indicating a GID is flipped horizontally. */
#define TMX_GID_FLIP_VERTICAL   0x40000000U /** Bit-flag indicating a GID is flipped vertically. */
#define TMX_GID_FLIP_DIAGONAL   0x20000000U /** Bit-flag indicating a GID is flipped diagonally. */
#define TMX_GID_ROTATE_120      0x10000000U /** Bit-flag indicating a hexagonal GID is rotated 120 degrees. */
#define TMX_GID_TILE_MASK       0x0FFFFFFFU /** Bit-mask to clear the flip/rotate bits from a GID. */
#define TMX_GID_FLAG_MASK       0xF0000000U /** Bit-mask to clear the flip/rotate bits from a GID. */

/**
 * @brief Returns the value of the specified GID with any flip/rotate bits removed.
 * @param[in] gid The global tile ID to sanitize.
 * @return The GID with flip/rotate bits removed.
 */
#define TMX_GID_CLEAN(gid) ((gid) & TMX_GID_TILE_MASK)

#define TMX_FALSE 0 /** Boolean false value. */
#define TMX_TRUE  1 /** Boolean true value. */

/**
 * @brief Describes error codes that can be emitted by the library.
 */
typedef enum
{
    TMX_ERR_NONE              = 0, /** No error. */
    TMX_ERR_WARN              = 1, /** A non-critical error or problem that can be recovered from occurred. */
    TMX_ERR_MEMORY            = 2, /** A memory allocation failed. */
    TMX_ERR_UNSUPPORTED       = 3, /** Unsupported feature, format, or encoding. */
    TMX_ERR_FORMAT            = 4, /** Unrecognized or unknown format. */
    TMX_ERR_PARAM             = 5, /** An invalid enumeration value was specified. */
    TMX_ERR_VALUE             = 6, /** An invalid or out-of-range value was specified. */
    TMX_ERR_INVALID_OPERATION = 7, /** Attempted an operation that is invalid in the current state/context. */
    TMX_ERR_IO                = 8, /** An IO error occurred. */
    TMX_ERR_PARSE             = 9, /** A parsing error occurred. */
} TMX_ERRNO;

/**
 * @brief Describes the value type of a property.
 */
typedef enum
{
    TMX_PROPERTY_UNSPECIFIED = 0, /** Unspecified type. Defaults to string. */
    TMX_PROPERTY_STRING      = 1, /** A string value. */
    TMX_PROPERTY_INTEGER     = 2, /** An integer value. */
    TMX_PROPERTY_FLOAT       = 3, /** A floating point value. */
    TMX_PROPERTY_BOOL        = 4, /** An integral boolean value that is either TMX_TRUE or TMX_FALSE. */
    TMX_PROPERTY_COLOR       = 5, /** A color value. */
    TMX_PROPERTY_FILE        = 6, /** A string value that should be interpreted as a filesystem path. */
    TMX_PROPERTY_OBJECT      = 7, /** An integer value that corresponds to the ID of a map object. */
    TMX_PROPERTY_CLASS       = 8, /** A custom property type with child properties. */
} TMX_PROPERTY_TYPE;

/**
 * @brief Describes the orientation/perspective in which a map should be rendered.
 *
 * @sa https://www.significant-bits.com/a-laymans-guide-to-projection-in-videogames/
 */
typedef enum
{
    TMX_ORIENTATION_UNSPECIFIED = 0, /** Invalid/unspecified value. */
    TMX_ORIENTATION_ORTHOGONAL  = 1, /** Classic "top-down" view, with perpendicular 90° angle between each axis. */
    TMX_ORIENTATION_ISOMETRIC   = 2, /** Isometric projection with 120° angle between each axis. */
    TMX_ORIENTATION_STAGGERED   = 3, /** Isometric projection with a staggered axis. */
    TMX_ORIENTATION_HEXAGONAL   = 4, /** Hexagonal with staggered axis. */
} TMX_ORIENTATION;

/**
 * @brief Describes the order in which tiles should be rendered on a map.
 */
typedef enum
{
    TMX_RENDER_RIGHT_DOWN = 0, /** From left-to-right, top-to-bottom. */
    TMX_RENDER_RIGHT_UP   = 1, /** From left-to-right, bottom-to-top. */
    TMX_RENDER_LEFT_DOWN  = 2, /** From right-to-left, top-to-bottom. */
    TMX_RENDER_LEFT_UP    = 3, /** From right-to-left, bottom-to-top. */
} TMX_RENDER_ORDER;

/**
 * @brief Describes an axis in 2D space.
 */
typedef enum
{
    TMX_AXIS_Y = 0, /** The y-axis. */
    TMX_AXIS_X = 1, /** The x-axis. */
} TMX_AXIS;

/**
 * @brief Describes which indices are shifted on a staggered/hexagonal map.
 */
typedef enum
{
    TMX_INDEX_ODD  = 0, /** Indicates the odd indices are shifted. */
    TMX_INDEX_EVEN = 1, /** Indicates the even indices are shifted. */
} TMX_INDEX;

/**
 * @brief Bit-flags that can provide additional meta-information about an object. These are typically not
 * relevant for general use of the API, are more useful to the internal implementation, but they do have
 * some limited use that might be useful publicly.
 *
 * @details
 * The primary purpose of flags is for objects that inherit from templates. The flags provide a mechanism that allows
 * to differentiate values that were explicitly defined in an object's definition, and which were inherited from a
 * template.
 *
 * The other use they have that is significant for the public API is differentiating when a value has been left
 * undefined, and is simply a default, or if it was explicitly set to the default value. For example, think of
 * a "color" value that is set to all zero. Is it defined as such explicitly as such, or is this simply the "default"
 * value from not being set? With the flags, checking for the presence of the @c TMX_FLAG_COLOR will indicate which.
 */
typedef enum
{
    TMX_FLAG_NONE           = 0x00000000U, /** No additional meta-data flags. */
    TMX_FLAG_EMBEDDED       = 0x00000001U, /** Indicates the current object's definition is embedded within another object. */
    TMX_FLAG_EXTERNAL       = 0x00000002U, /** Indicates the current object's definition resides in an external file. */
    TMX_FLAG_CACHED         = 0x00000004U, /** Indicates the item is stored within a cache object that manages it memory. */
    TMX_FLAG_COLOR          = 0x00000008U, /** Context varies, but indicates that a color field is explicitly defined. */
    TMX_FLAG_PROPERTIES     = 0x00000010U, /** For map text object, indicates the `` field has been explicitly defined. */
    TMX_FLAG_NAME           = 0x00000020U, /** For map objects, indicates that the `name` field has been explicitly defined. */
    TMX_FLAG_POSITION       = 0x00000040U, /** For map objects, indicates that the `position` field has been explicitly defined. */
    TMX_FLAG_X              = 0x00000080U, /** For map objects, indicates the position has been explicitly defined for the x-axis. */
    TMX_FLAG_Y              = 0x00000100U, /** For map objects, indicates the position has been explicitly defined for the y-axis. */
    TMX_FLAG_SIZE           = 0x00000200U, /** For map objects, indicates that the `size` field has been explicitly defined. */
    TMX_FLAG_WIDTH          = 0x00000400U, /** For map objects. indicates the size has been explicitly defined for the x-axis. */
    TMX_FLAG_HEIGHT         = 0x00000800U, /** For map objects. indicates the size has been explicitly defined for the y-axis. */
    TMX_FLAG_ROTATION       = 0x00001000U, /** For map objects, indicates that the `rotation` field has been explicitly defined. */
    TMX_FLAG_GID            = 0x00002000U, /** For map objects, indicates that the `gid` field has been explicitly defined. */
    TMX_FLAG_VISIBLE        = 0x00004000U, /** For map objects, indicates that the `visible` field has been explicitly defined. */
    TMX_FLAG_POINTS         = 0x00008000U, /** For map objects, indicates that the `points` field has been explicitly defined. */
    TMX_FLAG_CLASS          = 0x00010000U, /** For map objects, indicates that the `class` field has been explicitly defined. */
    TMX_FLAG_TEXT           = 0x00020000U, /** For map objects, indicates that the `text` field has been explicitly defined. */
    TMX_FLAG_USER1          = 0x00040000U, /** Reserved. May be applied by user to set an arbitrary flag. */
    TMX_FLAG_USER2          = 0x00080000U, /** Reserved. May be applied by user to set an arbitrary flag. */
    TMX_FLAG_ALIGN          = 0x00100000U, /** Indicates that the `align` field has been explicitly modified. */
    TMX_FLAG_VALIGN         = 0x00200000U, /** Indicates that a horizontal align flag has been explicitly defined. */
    TMX_FLAG_HALIGN         = 0x00400000U, /** Indicates that a vertical align flag has been explicitly defined. */
    TMX_FLAG_FONT           = 0x00800000U, /** Indicates that the `font` field has been explicitly defined. */
    TMX_FLAG_FONT_SIZE      = 0x01000000U, /** Indicates that the `pixel_size` field has been explicitly defined. */
    TMX_FLAG_FONT_STYLE     = 0x02000000U, /** Indicates that any `font_style` flag has been explicitly defined. */
    TMX_FLAG_FONT_BOLD      = 0x04000000U, /** Indicates that the `bold` font style has been explicitly defined. */
    TMX_FLAG_FONT_ITALIC    = 0x08000000U, /** Indicates that the `italic` font style has been explicitly defined. */
    TMX_FLAG_FONT_UNDERLINE = 0x10000000U, /** Indicates that the `underline` font style has been explicitly defined. */
    TMX_FLAG_FONT_STRIKEOUT = 0x20000000U, /** Indicates that the `strikeout` font style has been explicitly defined. */
    TMX_FLAG_FONT_KERNING   = 0x40000000U, /** Indicates that the `kerning` field has been explicitly defined. */
    TMX_FLAG_WORD_WRAP      = 0x80000000U, /** Indicates that the `word_wrap` field has been explicitly defined. */
    TMX_FONT_MASK           = 0xFFF00000U, /** Mask that isolates the font-related bits. Used to quickly detect any font changes. */
} TMX_FLAG;

/**
 * @brief Describes the type of a map layer. A custom "chunk" layer type has been added in addition to those in the TMX
 * format to differentiate traditional tile layers from those used by infinite maps that store their data in chunks.
 */
typedef enum
{
    TMX_LAYER_NONE     = 0, /** An invalid/undefined type. */
    TMX_LAYER_TILE     = 1, /** A tile layer with tile data. */
    TMX_LAYER_CHUNK    = 2, /** A tile layer for an infinite map and chunked tile data. */
    TMX_LAYER_OBJGROUP = 3, /** A layer with a collection of map objects. */
    TMX_LAYER_IMAGE    = 4, /** A layer with a single image. */
    TMX_LAYER_GROUP    = 5, /** A container of child layers. Its offset, tint, opacity, and visibility recursively affect child layers.*/
} TMX_LAYER_TYPE;

/**
 * @brief Bit flags that describe an alignment.
 */
typedef enum
{
    TMX_ALIGN_NONE     = 0x00,                                      /** No alignment specified. */
    TMX_ALIGN_LEFT     = 0x01,                                      /** Left-aligned (x-axis). */
    TMX_ALIGN_RIGHT    = 0x02,                                      /** Right-aligned (x-axis).*/
    TMX_ALIGN_TOP      = 0x04,                                      /** Top-aligned (y-axis).*/
    TMX_ALIGN_BOTTOM   = 0x08,                                      /** Bottom-aligned (y-axis).*/
    TMX_ALIGN_CENTER_H = (TMX_ALIGN_LEFT | TMX_ALIGN_RIGHT),        /** Centered on x-axis. */
    TMX_ALIGN_CENTER_V = (TMX_ALIGN_TOP | TMX_ALIGN_BOTTOM),        /** Centered on y-axis. */
    TMX_ALIGN_CENTER   = (TMX_ALIGN_CENTER_H | TMX_ALIGN_CENTER_V), /** Centered on both x and y axis. */
} TMX_ALIGN;

/**
 * @brief Bit-flags that describe the style of a font.
 */
typedef enum
{
    TMX_FONT_STYLE_NONE      = 0x00, /** Indicates no font style. */
    TMX_FONT_STYLE_BOLD      = 0x01, /** Bit-flag indicating bold font style. */
    TMX_FONT_STYLE_ITALIC    = 0x02, /** Bit-flag indicating italic font style. */
    TMX_FONT_STYLE_UNDERLINE = 0x04, /** Bit-flag indicating underline font style. */
    TMX_FONT_STYLE_STRIKEOUT = 0x08, /** Bit-flag indicating strikeout font style. */
} TMX_FONT_STYLE;

/**
 * @brief Describes the type of a map object.
 */
typedef enum
{
    TMX_OBJECT_RECT     = 0,              /** Indicates the object is a rectangular shape (default). */
    TMX_OBJECT_ELLIPSE  = 1,              /** Indicates the object is ellipse. */
    TMX_OBJECT_POINT    = 2,              /** Indicates the object is single point. */
    TMX_OBJECT_POLYGON  = 3,              /** Indicates the object is an arbitrary closed shape. */
    TMX_OBJECT_POLYLINE = 4,              /** Indicates the object is an arbitrary open shape. */
    TMX_OBJECT_TEXT     = 5,              /** Indicates the object displays text. */
    TMX_OBJECT_DEFAULT  = TMX_OBJECT_RECT /** Default type when not specified. */
} TMX_OBJECT_TYPE;

/**
 * @brief Describes the order that map objects should be rendered.
 */
typedef enum
{
    TMX_DRAW_TOPDOWN = 0, /** Objects should be drawn sorted by y-axis. */
    TMX_DRAW_INDEX   = 1, /** Objects should be drawn sorted by the order in which they were added to the map. */
} TMX_DRAW_ORDER;

/**
 * @brief Describes the size that tileset tiles should be drawn.
 */
typedef enum
{
    TMX_RENDER_SIZE_TILE = 0, /** Indicates that the size of tiles defined in the tileset should be used. */
    TMX_RENDER_SIZE_GRID = 1, /** Indicates that the size of the defined grid should be used. */
} TMX_RENDER_SIZE;

/**
 * @brief Describes the technique to use when rendering tile images.
 */
typedef enum
{
    TMX_FILL_MODE_STRETCH  = 0, /** Images should be stretched to fill the defined bounds. */
    TMX_FILL_MODE_PRESERVE = 1, /** Images should preserve aspect-ratio and fit themselves to the defined bounds. */
} TMX_FILL_MODE;

/**
 * @brief Describes the format of a TMX document.
 */
typedef enum
{
    TMX_FORMAT_AUTO = 0, /** Detect by file extension and/or text contents. */
    TMX_FORMAT_XML  = 1, /** Document is in XML format. */
    TMX_FORMAT_JSON = 2, /** DOcument is in JSON format. */
} TMX_FORMAT;

/**
 * @brief Describes the compression algorithm used by data.
 */
typedef enum
{
    TMX_COMPRESSION_NONE = 0, /** No compression. */
    TMX_COMPRESSION_GZIP = 1, /** Gzip compression (i.e. DEFLATE) */
    TMX_COMPRESSION_ZLIB = 2, /** Zlib compression (i.e. DEFLATE with additional header and checksum) */
    TMX_COMPRESSION_ZSTD = 3, /** Zstandard compression. Optional compile-time algorithm created by Facebook. */
} TMX_COMPRESSION;

/**
 * @brief Describes the encoding algorithm used by data.
 */
typedef enum
{
    TMX_ENCODING_NONE   = 0, /** No encoding. */
    TMX_ENCODING_CSV    = 1, /** A string containing comma-separated values. */
    TMX_ENCODING_BASE64 = 2, /** A Base64-encoded string. */
} TMX_ENCODING;

/**
 * @brief Numeric type representing a tile ID.
 */
typedef uint32_t TMXtid;

/**
 * @brief Describes a @b global tile ID, which may also be tainted with bits indicating flip/rotation.
 *
 * @note This is simply an alias for a @ref TMXtid type to differentiate the semantics between local/global IDs in the API.
 * @sa @ref TMX_GID_CLEAN
 */
typedef TMXtid TMXgid;

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
 * @brief A structure describing a location and size of a rectangular object.
 */
typedef union TMXrect
{
    struct
    {
        int x; /** The position of the top-left corner of the rectangle on the x-axis. */
        int y; /** The position of the top-left corner of the rectangle on the y-axis. */
        int w; /** The dimensions of the rectangle on the x-axis. */
        int h; /** The dimensions of the rectangle on the y-axis. */
    };
    struct
    {
        TMXpoint position; /** The location of the top-left corner of the rectangle. */
        TMXsize size;      /** The dimensions of the rectangle. */
    };
} TMXrect;

/**
 * @brief A color represented as ARGB in a packed integer value, with component values ranging from 0 to 255.
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
 * @note Compile with the @c -DTMX_VECTOR_COLOR flag to make this the default structure used for all colors.
 */
typedef struct TMXcolorf
{
    float r; /** The value of the red component. */
    float g; /** The value of the green component. */
    float b; /** The value of the blue component. */
    float a; /** The value of the alpha component. */
} TMXcolorf;

/**
 * @brief Opaque type that maintains references to reusable TMX types including tilesets, templates, and images. This allows them to be
 * used without loading and parsing them each time they are referenced between map loads.
 */
typedef struct TMXcache TMXcache;

/**
 * @brief Opaque type that stores property values in a hashed dictionary-like structure.
 */
typedef struct TMXproperties TMXproperties;

/**
 * @brief Describes a named user-defined value.
 */
typedef struct TMXproperty
{
    const char *name;       /** The name of the property. */
    const char *class;      /** The custom type of the property. */
    TMX_PROPERTY_TYPE type; /** Indicates the type of the property, and which value field to reference. */
    union                   /** A union containing the value of the property. Use the type to determine which field to use. */
    {
        const char *string;        /** A string value, valid with TMX_PROPERTY_STRING and TMX_PROPERTY_FILE. */
        int integer;               /** An integer value, valid with TMX_PROPERTY_INTEGER, TMX_PROPERTY_OBJECT, and TMX_PROPERTY_BOOL. */
        float decimal;             /** A floating point value, valid with TMX_PROPERTY_FLOAT. */
        TMX_COLOR_T color;         /** A color value, valid with TMX_PROPERTY_COLOR. */
        TMXproperties *properties; /** Child properties, valid with TMX_PROPERTY_CLASS. */
    } value;
    struct TMXproperty *prev; /** Access the previous property in the properties list. */
    struct TMXproperty *next; /** Access the next property in the properties list.  */
    TMXuserptr user;          /** User-defined value that can be attached to this object. Will never be modified by this library. */
} TMXproperty;

// TODO: Have next/previous with properties, as they are hash-like

/**
 * @brief Describes an image that is used for a map, tileset, or object.
 */
typedef struct TMXimage
{
    TMX_FLAG flags;          /** Flags providing additional information about the image. */
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

/**
 * @brief Describes a single "chunk" of tile data in an infinite map.
 */
typedef struct TMXchunk
{
    TMXrect bounds; /** A rectangle describing the position/size of the chunk. */
    size_t count;   /** The number of global tile IDs in the `gids` array. */
    TMXgid *gids;   /** An array of global tile IDs. */
} TMXchunk;

/**
 * @brief For text objects, describes the text/font settings.
 */
struct TMXtext
{
    const char *font;     /** The font family used (defaults to “sans-serif”) when NULL. */
    int pixel_size;       /** The size of the font in pixel units. */
    TMX_BOOL wrap;        /** Indicates whether word wrapping is enabled. */
    TMX_COLOR_T color;    /** The color of the text. */
    TMX_FONT_STYLE style; /** Bit-flags describing the font style(s). */
    TMX_BOOL kerning;     /** Indicates whether kerning should be used while rendering the text. */
    TMX_ALIGN align;      /** Bit-flags describing how the text should be aligned. */
    const char *string;   /** The string contents of the text to render. */
    TMXuserptr user;      /** User-defined value that can be attached to this object. Will never be modified by this library. */
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

/**
 * @brief Structure describing a map object.
 */
typedef struct TMXobject
{
    TMX_FLAG flags;        /** Flags providing additional information about the object. Useful to indicate explicit/default fields. */
    int id;                /** The unique ID of the object. */
    TMX_OBJECT_TYPE type;  /** Enumeration value describing the object type. */
    const char *name;      /** The name of the object. An arbitrary string. */
    const char *class;     /** The class of the object. An arbitrary string. */
    TMXvec2 position;      /** The coordinate of the object in pixel units. */
    TMXvec2 size;          /** The dimensions of the object in pixel units.*/
    float rotation;        /** The rotation of the object in degrees clockwise around the object's position. */
    TMXgid gid;            /** An optional reference to a tile, or 0 when not defined. */
    TMX_BOOL visible;      /** Indicates when ther object is shown or hidden. */
    TMXtemplate *template; /** An optional reference to a template file. */
    union
    {
        struct TMXcoords poly; /** The points list. Applicable only when `type` is TMX_OBJECT_POLYGON or TMX_OBJECT_POLYLINE. */
        struct TMXtext *text;  /** The text object. Applicable only when `type` is TMX_OBJECT_TEXT. */
    };
    TMXproperties *properties; /** Named property hash/dictionary containing arbitrary values. */
    TMXuserptr user;           /** User-defined value that can be attached to this object. Will never be modified by this library. */
} TMXobject;

/**
 * @brief Describes a layer within a map.
 */
typedef struct TMXlayer
{
    TMX_FLAG flags;         /** Flags providing additional information about the layer. */
    TMX_LAYER_TYPE type;    /** Enumeration value describing the kind of layer this is and which data field is valid. */
    int id;                 /** The unique ID of the layer. */
    const char *name;       /** The name of the layer. */
    const char *class;      /** The class of the layer. */
    TMXpoint position;      /** The layer of the layer in tile units. Defaults to 0 and can not be changed in Tiled. */
    TMXsize size;           /** The size of the layer in tile units. Always the same as the map size for fixed-size maps. */
    float opacity;          /** The opacity of the layer as a value in the range of 0.0 to 1.0 inclusive. */
    TMX_BOOL visible;       /** Indicates if the layer is shown od hidden. */
    TMX_COLOR_T tint_color; /** A optional tint color that is multiplied with any tiles drawn by this layer. */
    TMXpoint offset;        /** The offset for this layer in pixel units. */
    TMXvec2 parallax;       /** The parallax factor for this layer. */
    size_t count;           /** Indicates the number of items in the data array. Not applicable when type is TMX_LAYER_IMAGE. */
    union /** Union of values relevant only for a specific kind of layer. Use the `type` value to indicate which field is applicable. */
    {
        TMXgid *tiles;           /** A contiguous array of global tile IDS. Applicable when the layer type is TMX_LAYER_TILE. */
        TMXchunk *chunks;        /** A contiguous array of chunks. Applicable when the layer type is TMX_LAYER_CHUNK. */
        TMXimage *image;         /** The layer image. Applicable when the layer type is TMX_LAYER_IMAGE. */
        TMXobject **objects;     /** A contiguous array of child object pointers. Applicable when the layer type is TMX_LAYER_OBJGROUP. */
        struct TMXlayer **group; /** A contiguous array of child layer pointers. Applicable when the the layer type is TMX_LAYER_GROUP. */
    } data;
    struct /** Indicates whether the image for this layer repeats. Applicable when the layer type is TMX_LAYER_IMAGE. */
    {
        TMX_BOOL x; /** Indicates whether the image drawn by this layer is repeated along the x-axis. */
        TMX_BOOL y; /** Indicates whether the image drawn by this layer is repeated along the y-axis. */
    } repeat;
    TMX_DRAW_ORDER
    draw_order; /** Indicates the order in which objects should be drawn. Applicable when the layer type is TMX_LAYER_OBJGROUP. */
    TMXproperties *properties; /** Named property hash/dictionary containing arbitrary values. */
    TMXuserptr user;           /** User-defined value that can be attached to this object. Will never be modified by this library. */
} TMXlayer;

/**
 * @brief Describes a single frame within an animation.
 */
typedef struct TMXframe
{
    TMXtid id;             /** The local ID of a tile within the parent tileset. */
    unsigned int duration; /** How long (in milliseconds) this frame should be displayed before advancing to the next frame. */
} TMXframe;

/**
 * @brief Container for map objects used for tile collisions.
 */
typedef struct TMXcollision
{
    size_t count;        /** The number of objects in the array. */
    TMXobject **objects; /** A contiguous array of objects pointers. */
} TMXcollision;

/**
 * @brief Container for tile animation frames.
 */
typedef struct TMXanimation
{
    size_t count;     /** The number of frames in the animation. */
    TMXframe *frames; /** A contiguous array of frames. */
} TMXanimation;

/**
 * @brief Describes a tile definition within a tileset.
 */
typedef struct TMXtile
{
    TMXtid id;                 /** The local ID of this tile within its parent tileset. */
    const char *class;         /** The class of the tile. Is inherited by tile objects. */
    TMXrect rect;              /** The sub-rectangle representing this tile within the tileset, in pixel units. */
    TMXimage *image;           /** The image associated with this tile, or NULL. */
    TMXanimation animation;    /** A collection of animation frames. */
    TMXcollision collision;    /** A collection of shapes describing the tile collision(s). */
    TMXproperties *properties; /** Named property hash/dictionary containing arbitrary values. */
    TMXuserptr user;           /** User-defined value that can be attached to this object. Will never be modified by this library. */
} TMXtile;

/**
 * @brief
 *
 * @details The following flags can be presents:
 *
 * * TMX_FLAG_CACHED - The tileset is cached.
 * * TMX_FLAG_EXTERNAL - The tileset definition resides in its own file.
 * * TMX_FLAG_EMBEDDED - The tileset definition is embedded within a map object.
 *
 */
typedef struct TMXtileset
{
    TMX_FLAG flags;
    const char *version;
    const char *tiled_version;
    TMXgid first_gid;
    const char *name;
    const char *class;
    TMX_COLOR_T background_color;
    TMXsize tile_size;
    int spacing;
    int margin;
    int columns;
    TMX_ALIGN object_align;
    TMX_RENDER_SIZE render_size;
    TMX_FILL_MODE fill_mode;
    TMXpoint offset;
    TMXimage *image;
    size_t tile_count;
    TMXtile *tiles;
    struct
    {
        TMX_ORIENTATION orientation;
        TMXsize size;
    } grid;
    TMXproperties *properties;
    TMXuserptr user;
} TMXtileset;

typedef struct TMXmaptileset
{
    TMXgid first_gid;
    TMXtileset *tileset;
} TMXmaptileset;

typedef struct TMXmap
{
    TMX_FLAG flags;                /** Meta-data flags that can provide additional information about the map. */
    const char *version;           /** The TMX format version. */
    const char *tiled_version;     /** The Tiled version used to save the file. */
    const char *class;             /** The class of this map. */
    TMX_ORIENTATION orientation;   /** The orientation of the map. */
    TMX_RENDER_ORDER render_order; /** The order in which tiles on tile layers are rendered. */
    TMXsize size;                  /** The map size, in tile units. */
    TMXsize tile_size;             /** The size of map tiles, in pixel units. */
    TMXsize pixel_size;            /** The size of the map, in pixel units. */
    int hex_side;                  /** For hexagonal maps, the width or height (depending on axis) of the tile’s edge, in pixel units. */
    struct                         /** Determines how staggered and hexagonal maps are drawn. */
    {
        TMX_AXIS axis;   /** Determines which axis is staggered. */
        TMX_INDEX index; /** Determines whether the even or odd indices along the staggered axis are shifted. */
    } stagger;
    TMXvec2 parallax_origin;      /** The parallax origin in pixel units. */
    TMX_COLOR_T background_color; /** The background color of the map. The TMX_FLAG_COLOR flag will be set when defined. */
    TMX_BOOL infinite;            /** Indicates whether map is infinite and its data is stored as chunks. */
    TMXproperties *properties;    /** Named property hash/dictionary containing arbitrary values. */
    size_t tileset_count;         /** The number of tilesets used in this map. */
    TMXmaptileset *tilesets;      /** A linked-list containing the tilesets and their first global tile ID. */
    size_t layer_count;           /** The number of layers defined in the map. */
    TMXlayer **layers;            /** A contiguous array of map layer pointers. */
    TMXuserptr user;              /** User-defined value that can be attached to this object. Will never be modified by this library. */
} TMXmap;

struct TMXtemplate
{
    TMX_FLAG flags;      /** Meta-data flags that can provide additional information about the template. */
    TMXgid first_gid;    /** When tileset is defined, indicates the first global tile ID of the tileset within the parent map, */
    TMXtileset *tileset; /** When the object is a tile, points to the parent tileset. */
    TMXobject *object;   /** The template object other objects inherit their values from. */
    TMXuserptr user;     /** User-defined value that can be attached to this object. Will never be modified by this library. */
};

/**
 * @brief Callback prototype for iterating a tile layer. The render order of the map is respected, and tiles
 * are yielded in the order defined by the parent map.
 *
 * @param[in] map The parent map.
 * @param[in] layer The parent tile layer within the @a map.
 * @param[in] tile The tile to be rendered, or can be @c NULL if specified to yield empty tiles.
 * @param[in] x The location the tile should be rendered on the x-axis, in tile units.
 * @param[in] y The location the tile should be rendered on the y-axis, in tile units.
 * @param[in] gid The raw global tile ID, with flip/rotate flags present if set.
 *
 * @return @ref TMX_TRUE to continue iteration, otherwise @ref TMX_FALSE to stop.
 */
typedef TMX_BOOL (*TMXforeachfunc)(const TMXmap *map, const TMXlayer *layer, const TMXtile *tile, int x, int y, TMXgid gid);

/**
 * @brief Prototype for error callbacks.
 *
 * @param errno An error code indicating the general type of error that occurred.
 * @param message A brief description of the error.
 * @param user The user-defined pointer that was specified when setting the callback.
 *
 */
typedef void (*TMXerrorfunc)(TMX_ERRNO errno, const char *message, TMXuserptr user);

/**
 * @brief Sets a callback function that will be invoked when errors are emitted by the library.
 *
 * @param callback The function to invoke when an error occurs.
 * @param user A user-defined pointer that will be passed to the callback function.
 */
void tmxErrorCallback(TMXerrorfunc callback, TMXuserptr user);

/**
 * @brief Retrieves a generic error message suitable for the given error type.
 * @param errno An error code indicating the general type of error that occurred.
 * @return The error string. This string must @b not be freed by the caller.
 */
const char *tmxErrorString(TMX_ERRNO errno);

/**
 * @brief Retrieves the first error (if any) that occurred since the last call to this function, then
 * resets the error state.
 *
 * @return The stored error state, or @ref TMX_ERR_NONE if no error has occurred.
 */
TMX_ERRNO tmxGetError(void);

/**
 * @brief Retrieves a property by its name.
 *
 * @param[in] properties The properties instance to query.
 * @param[in] name The name of the property to retrieve.
 * @param[out] property A pointer that will be assigned the property value, or @c NULL if just testing for the presence of
 * the property.
 *
 * @return @ref TMX_TRUE if property was found, otherwise @ref TMX_FALSE. When true, @a property will contain
 * the value, otherwise it will be assigned @c NULL.
 */
TMX_BOOL tmxTryGetProperty(const TMXproperties *properties, const char *name, TMXproperty **property);

/**
 * @brief Retrieves a property by its name.
 *
 * @param[in] properties The properties instance to query.
 * @param[in] name The name of the property to retrieve.
 *
 * @return The property with the specified @a name, or @c NULL if none was found.
 */
TMXproperty *tmxGetProperty(const TMXproperties *properties, const char *name);

/**
 * @brief Retrieves the number of property objects stored in the hash.
 *
 * @param[in] properties The properties instance to query.
 * @return The number of property objects in the hash.
 */
size_t tmxGetPropertyCount(const TMXproperties *properties);

/**
 * @brief Retrieves the first property in the properties hash.
 *
 * @param[in] properties The properties instance to query.
 * @return The first property value, or @c NULL if empty.
 */
TMXproperty *tmxGetPropertiesHead(const TMXproperties *properties);

/**
 * @brief Helper macro to iterate properties.
 *
 * @param[in] properties The properties value to enumerate.
 * @param[in,out] property A pointer that will be assigned the property value within the loop.
 */
#define tmxPropertiesForeach(properties, property)                                                                                         \
    for ((property) = tmxGetPropertiesHead(properties); (property); (property) = (property)->next)

/**
 * @brief Assigns a callback that can be invoked to provide user-loading of images as they are parsed from a document.
 *
 * @details This can be used configure an automatic load/free process for map resources, such as creating
 * required textures to render a map. The value returned by the callback is stored within the image structure
 * for convenient access to it while using the API.
 *
 * @param[in] load A callback that will be invoked when a TMX image is parsed from the document.
 * @param[in] free A callback that will be invoked when the TMX image is being freed to perform any necessary cleanup of the user-data.
 * @param[in] user An arbitrary user pointer that will be passed to the callbacks when invoked. Will never be modified by this library.
 */
TMX_PUBLIC void tmxImageCallback(TMXimageloadfunc load, TMXimagefreefunc free, TMXuserptr user);

TMX_PUBLIC void tmxTileForeach(TMXmap *map, TMXlayer *layer, TMX_BOOL includeEmpty, TMXforeachfunc foreachFunc);

/**
 * @brief Frees a previously created map and all of its child objects.
 *
 * @param[in] map The map pointer to free.
 * @note Cached child objects will not be freed.
 */
TMX_PUBLIC void tmxFreeMap(TMXmap *map);

/**
 * @brief Frees a previously created tileset.
 *
 * @param[in] tileset The tileset pointer to free.
 * @note Cached child objects will not be freed.
 * @warning This function should only ever be called on a pointer that was explicitly loaded/parsed by the caller.
 * @exception If the tileset is cached, a @ref TMX_ERR_INVALID_OPERATION error will be emitted.
 */
TMX_PUBLIC void tmxFreeTileset(TMXtileset *tileset);

/**
 * @brief Frees a previously created template.
 *
 * @param[in] template The template pointer to free.
 * @note Cached child objects will not be freed.
 * @warning This function should only ever be called on a pointer that was explicitly loaded/parsed by the caller.
 * @exception If the template is cached, a @ref TMX_ERR_INVALID_OPERATION error will be emitted.
 */
TMX_PUBLIC void tmxFreeTemplate(TMXtemplate *template);

/**
 * @brief Loads a TMX map document from the specified path.
 *
 * @param[in] filename The filesystem path containing the map definition.
 * @param[in] cache An optional cache object to store reusable/shared map components.
 * @param[in] format Flag indicating the format of the document.
 *
 * @return The map object, or @c NULL if an error occurred.
 */
TMX_PUBLIC TMXmap *tmxLoadMap(const char *filename, TMXcache *cache, TMX_FORMAT format);

/**
 * @brief Loads a TMX map document from the specifed text buffer.
 *
 * @param[in] text A buffer containing the text contents of the map definition.
 * @param[in] cache An optional cache object to store reusable/shared map components.
 * @param[in] format Flag indicating the format of the document.
 *
 * @return The map object, or @c NULL if an error occurred.
 */
TMX_PUBLIC TMXmap *tmxParseMap(const char *text, TMXcache *cache, TMX_FORMAT format);

/**
 * @brief Loads a TMX tileset document from the specified path.
 *
 * @param[in] filename The filesystem path containing the tileset definition.
 * @param[in] cache An optional cache object to store reusable/shared map components.
 * @param[in] format Flag indicating the format of the document.
 *
 * @return The tileset object, or @c NULL if an error occurred. May return a cached instance when @a cache is supplied.
 */
TMX_PUBLIC TMXtileset *tmxLoadTileset(const char *filename, TMXcache *cache, TMX_FORMAT format);

/**
 * @brief Loads a TMX tileset document from the specifed text buffer.
 *
 * @param[in] text A buffer containing the text contents of the tileset definition.
 * @param[in] cache An optional cache object to store reusable/shared map components.
 * @param[in] format Flag indicating the format of the document.
 *
 * @return The tileset object, or @c NULL if an error occurred.
 */
TMX_PUBLIC TMXtileset *tmxParseTileset(const char *text, TMXcache *cache, TMX_FORMAT format);

/**
 * @brief Loads a TMX template document from the specified path.
 *
 * @param[in] filename The filesystem path containing the template definition.
 * @param[in] cache An optional cache object to store reusable/shared map components.
 * @param[in] format Flag indicating the format of the document.
 *
 * @return The template object, or @c NULL if an error occurred. May return a cached instance when @a cache is supplied.
 */
TMX_PUBLIC TMXtemplate *tmxLoadTemplate(const char *filename, TMXcache *cache, TMX_FORMAT format);

/**
 * @brief Loads a TMX template document from the specifed text buffer.
 *
 * @param[in] text A buffer containing the text contents of the template definition.
 * @param[in] cache An optional cache object to store reusable/shared map components.
 * @param[in] format Flag indicating the format of the document.
 *
 * @return The template object, or @c NULL if an error occurred.
 */
TMX_PUBLIC TMXtemplate *tmxParseTemplate(const char *text, TMXcache *cache, TMX_FORMAT format);

#pragma region Cache

/**
 * @defgroup cache Cache
 * @brief Provides mechanisms for caching commonly used objects and/or objects that are
 * shared by multiple components. This allows them to be loaded/parsed once, and reused
 * as-needed.
 */

/**
 * @defgroup cachetypes Cache Targets
 * @brief Bit-flags describing supported TMX types that can be cached.
 *
 * @ingroup cache
 * @{
 */

typedef enum
{
    TMX_CACHE_NONE     = 0x00, /** None/invalid target. */
    TMX_CACHE_TEMPLATE = 0x01, /** Object templates. */
    TMX_CACHE_TILESET  = 0x02, /** Tilesets used by tile layers and objects. */
    TMX_CACHE_ALL      = 0xFF, /** Targets all supported cache types. */
} TMX_CACHE_TARGET;

/* @} */

/**
 * Helper macro to retrieve a tileset from the cache.
 * @param[in] cache The cache to query.
 * @param[in] key The key of the item to retrieve.
 * @param[out] result The address of a tileset pointer to store the result upon success.
 * @return @c TMX_TRUE when object was successfully retrieved, otherwise @c TMX_FALSE
 */
#define tmxCacheTryGetTileset(cache, key, result) tmxCacheTryGet((cache), (key), (void **) (result), TMX_CACHE_TILESET)

/**
 * Helper macro to retrieve a template from the cache.
 * @param[in] cache The cache to query.
 * @param[in] key The key of the item to retrieve.
 * @param[out] result The address of a template pointer to store the result upon success.
 * @return @c TMX_TRUE when object was successfully retrieved, otherwise @c TMX_FALSE
 */
#define tmxCacheTryGetTemplate(cache, key, result) tmxCacheTryGet((cache), (key), (void **) (result), TMX_CACHE_TEMPLATE)

/**
 * Helper macro to add a tileset to the cache.
 * @param[in] cache The cache to add an item to.
 * @param[in] key The key that will be used to store/retrieve the item.
 * @param[in] tileset A pointer to a tileset.
 * @return @c TMX_TRUE when object was successfully added, otherwise @c TMX_FALSE if insertion failed.
 */
#define tmxCacheAddTileset(cache, key, tileset) tmxCacheAdd(cache, key, tileset, TMX_CACHE_TILESET)

/**
 * Helper macro to add a template to the cache.
 * @param[in] cache The cache to add an item to.
 * @param[in] key The key that will be used to store/retrieve the item.
 * @param[in] template A pointer to a template.
 * @return @c TMX_TRUE when object was successfully added, otherwise @c TMX_FALSE if insertion failed.
 */
#define tmxCacheAddTemplate(cache, key, template) tmxCacheAdd(cache, key, template, TMX_CACHE_TEMPLATE)

/**
 * @brief Attempts to add an item of the specifed type to the cache. If the key already exists in the specified target, it will not be
 * inserted or overwrite it. To replace an item, it must first be removed with @ref tmxCacheRemove or @ref tmxCacheClear.
 *
 * @param[in] cache The cache to add an item to.
 * @param[in] key The key that will be used to store/retrieve the item.
 * @param[in] obj The object to add.
 * @param[in] target A single flag indicating which type of object this is.
 *
 * @return @c TMX_TRUE when object was successfully added, otherwise @c TMX_FALSE if insertion failed.
 */
TMX_PUBLIC TMX_BOOL tmxCacheAdd(TMXcache *cache, const char *key, void *obj, TMX_CACHE_TARGET target);

/**
 * @brief Attempts to retrieve an item of the specified type and key from the cache.
 *
 * @param[in] cache The cache to query.
 * @param[in] key The key of the item to retrieve.
 * @param[out] result A pointer to store the result upon success.
 * @param[in] target A single flag indicating the type of object to retrieve.
 *
 * @return Indicates if an item was successfully found. When @c TMX_TRUE, the @a outResult will contain
 * a valid object, otherwise if @c TMX_FALSE it should not be used.
 */
TMX_PUBLIC TMX_BOOL tmxCacheTryGet(TMXcache *cache, const char *key, void **result, TMX_CACHE_TARGET target);

/**
 * @brief Deletes an item of the specified type from the cache.
 *
 * @param[in] cache The cache to delete an item from.
 * @param[in] key The key of the value to remove.
 * @param[in] target A single flag indicating the type of object to retrieve.
 *
 * @return @c TMX_TRUE if item was successfully removed, otherwise @c TMX_FALSE.
 */
TMX_PUBLIC TMX_BOOL tmxCacheRemove(TMXcache *cache, const char *key, TMX_CACHE_TARGET target);

/**
 * @brief Removes and frees the items in the cache of the specified type(s).
 *
 * @param[in] cache The cache to remove items from.
 * @param[in] targets A sets of bit-flags OR'ed together determining which type of item(s) to remove.
 *
 * @return The number of items successfully removed.
 */
TMX_PUBLIC size_t tmxCacheClear(TMXcache *cache, TMX_CACHE_TARGET targets);

/**
 * @brief Retrieves the number of items in the cache.
 * @param[in] cache The cache to query.
 * @param[in] targets A sets of bit-flags OR'ed together determining which type of item(s) to count.
 *
 * @return The number of items in the cache of the specified type(s).
 */
TMX_PUBLIC size_t tmxCacheCount(TMXcache *cache, TMX_CACHE_TARGET targets);

/**
 * @brief Initializes a new instance of a cache and returns it.
 * @param[in] targets A sets of bit-flags OR'ed together determining which types will be automatically added to the cache.
 * @return The newly created object, which must be freed with @ref tmxFreeCache.
 */
TMX_PUBLIC TMXcache *tmxCacheCreate(TMX_CACHE_TARGET targets);

/**
 * @brief Frees the cache and all of the items it contains.
 * @param[in] cache The cache to free.
 */
TMX_PUBLIC void tmxFreeCache(TMXcache *cache);

/**
 * @brief Returns packed color from a normalized vector color.
 *
 * @param[in] color The vector color to convert.
 * @return The resulting packed color.
 */
TMX_PUBLIC TMXcolor tmxColor(const TMXcolorf *color);

/**
 * @brief Returns a vector color from a packed integral color.
 *
 * @param color The packed color to convert.
 * @return The resulting vector color.
 */
TMX_PUBLIC TMXcolorf tmxColorF(TMXcolor color);

#pragma endregion

#define tmxUserPtr(ptr) ((TMXuserptr){ptr})
#define tmxNullUserPtr  tmxUserPtr(NULL)

#ifndef TMX_DEBUG

/**
 * @brief Prints allocation/deallocation details to the standard output.
 */
void tmxMemoryLeakCheck(void);
#endif

#endif /* TMX_H */