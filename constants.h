#ifndef	__CONSTANTS_H__
#define	__CONSTANTS_H__

#define DEBUG_ERROR_VERBOSE     1

/* Colors */
#define kBlackColor     (RGBColor){0, 0, 0}
#define kWhiteColor     (RGBColor){0xffff, 0xffff, 0xffff}
#define kGrayColor      (RGBColor){0x7fff, 0x7fff, 0x7fff}
#define kDarkGrayColor  (RGBColor){0x9fff, 0x9fff, 0x9fff}
#define kRedColor       (RGBColor){0xffff, 0, 0}
#define kLightRedColor  (RGBColor){0xffff, 0x7fff, 0x7fff}
#define kDialogBGColor  (RGBColor){0xdddd, 0xdddd, 0xdddd}

/* Icons */
#define kOffscreenIconRect   0, 0, 32, 32

/* Menus */
enum {
	kAppleNum           = 128,
    kFileNum            = 129,
    kFileGetInfo        = 1
};

/* Dialogs */
enum {
    kAlertError         = 200,
    kAlertAbout         = 128,
    kMainDialog         = 129
};

/* Strings */
enum {
    kMainDlgStrList         = 130,
    kMainDlgLblAppSelect    = 1,
    kMainDlgLblAppDrag,
    kMainDlgLblVersion,
    kMainDlgLblTypeCreator,
    kMainDlgLbl68k,
    kMainDlgLblPPC,
    kMainDlgLblLanguage,
    kMainDlgLblFramework,
    kMainDlgLblEngine,
    kMainDlgLblCopyProtection
};

/* Controls
    Application Name
    Version
    68k
    PPC
    Language
    Framework
    Engine
    Copy Protection
*/
enum {
    kTETextBoxCount     = 18,
    kTEDataRows         = 8,
    kcReportButton      = 1,
    kcIcon,
    kcApplicationName,
    kcTEStatus,

    kcHeaderVersion,
    kcHeaderTypeCreator,
    kcHeader68k,
    kcHeaderPPC,
    kcHeaderLanguage,
    kcHeaderFramework,
    kcHeaderEngine,
    kcHeaderCopyProtection,
    
    kcDataVersion,
    kcDataTypeCreator,
    kcData68k,
    kcDataPPC,
    kcDataLanguage,
    kcDataFramework,
    kcDataEngine,
    kcDataCopyProtection,
    
    kcSeparator1,
    kcSeparator2,
    
    kcTEFirstIndex      = kcApplicationName,    // first index for gTEHandles
    kcHeaderFirst       = kcHeaderVersion,
    kcDataFirst         = kcDataVersion
};

#define kInitText       "\p-"

#define kInfoFontFamily "\pGeneva"
#define kInfoFontSize   9

/* Custom errors */
enum {
    kErrStringListID    = 300,
    kUndefinedErrorID   = 0,
    
    kNotAPPLErr         = 1,
    kMoreThanOneErr,
    kResourceForkErr,
    kOutputErr,
    kOutputExistsErr,
    kDataForkErr,
    kBadAliasErr,
    kAEVTErr,
    kNoBNDLIconErr,
    
    kUserCancelErr      = 99,
    kInvalidDragErr     = 99
};

/* AE errors */
enum {
    kErrStartupStringListID = 100,
    kGestaltAppleEventsErr  = 1,
    kGestaltDragMgrErr,
    kGestaltDialogMgrErr,
    kGestaltFSErr,
    kGestaltResourceMgrErr,
    kGestaltIconUtilitiesErr,
    kGestaltStandardFileErr
};

#endif