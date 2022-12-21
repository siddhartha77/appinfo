#ifndef	__PLUGIN_H__
#define	__PLUGIN_H__

#include    "utils.h"

#define kPluginsPath            "\p:Plugins"
#define kPluginOutputDelimiter  "\p / "

enum {
    kPluginResourceType         = 'PLUG',
    kPluginCatLanguage          = 'LANG',
    kPluginCatFramework         = 'FMWK',
    kPluginCatEngine            = 'NGIN',
    kPluginCatCopyProtection    = 'COPY'
};

struct PluginResourceType {
    ResType         theType;
    UInt16          resourceCount;
};
typedef struct PluginResourceType PluginResourceType;
typedef PluginResourceType * PluginResourceTypePtr;
typedef PluginResourceTypePtr * PluginResourceTypeH;

/*
    Since PluginResource can change size because of Str255,
    all references should be to PluginResourceH.
*/
struct PluginResource {
    UInt16          theID;
    Str255          name; 
};
typedef struct PluginResource PluginResource;
typedef PluginResource * PluginResourcePtr;
typedef PluginResourcePtr * PluginResourceH;

/*
    Same as PluginResource above, reference using PluginDFIdentifierH.
*/
struct PluginDFIdentifier {
    UInt32          offset;
    Str255          data;
};
typedef struct PluginDFIdentifier PluginDFIdentifier;
typedef PluginDFIdentifier * PluginDFIdentifierPtr;
typedef PluginDFIdentifierPtr * PluginDFIdentifierH;

/*
    ParsedPluginResource remains a fixed size, so we don't need handles
    to handles to it like with do with PluginResource and PluginDFIdentifier.
    
    resourcesH is a handle to an array of PluginResourceH,
    which are handles.
*/
struct ParsedPluginResource {
    PluginResourceTypeH         resourceTypeH;
    PluginResourceH**           resourcesH;
};
typedef struct ParsedPluginResource ParsedPluginResource;
typedef ParsedPluginResource * ParsedPluginResourcePtr;
typedef ParsedPluginResourcePtr * ParsedPluginResourceH;

struct ParsedPlugin {
    FourCharCode                category;
    UInt16                      typeCount;
    ParsedPluginResourceH       resourceTypesH;
    UInt16                      dfIdentifierCount;
    PluginDFIdentifierH**       dfIndentifiersH;
    Str255                      name;
};
typedef struct ParsedPlugin ParsedPlugin;
typedef ParsedPlugin * ParsedPluginPtr;
typedef ParsedPluginPtr * ParsedPluginH;

struct PluginStrOutput {
    Str255  language;
    Str255  framework;
    Str255  engine;
    Str255  copyProtection;
};
typedef struct PluginStrOutput PluginStrOutput;
typedef struct PluginStrOutput * PluginStrOutputPtr;

Error           LoadPlugins(ParsedPluginH **pluginArrayH, short *count);
static Error    ParsePlugin(Handle pluginDataH, ParsedPluginH parsedPluginH);
void            ExecPlugins(FSSpecPtr theSpec, ParsedPluginH **pluginArrayH, short count, PluginStrOutputPtr outputStrings);
static void     SetPluginOutputStr(StringPtr field, StringPtr name, Boolean existingMatch);

#endif