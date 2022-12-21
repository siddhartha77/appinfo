#ifndef	__APPINFO_H__
#define	__APPINFO_H__

#define kReportCreator          'ttxt'
#define kReportFileType         'TEXT'
#define kReportSuffix           "\p.nfo"

/*
    Some applications have 68k code to let
    the user know the application is PPC only
*/
enum {
    kMax68kCodeSizePPCAlert     = 4000,
    kMinPPCCodeSizePPCAlert     = 5000
};

OSErr       GenerateReport(Str255 filename, FSSpecPtr myFSSPtr, Str255 inVersion);
OSType      GetFileType(FSSpecPtr inSpec);
Boolean     CustomIconExists(FSSpecPtr theSpec);
Boolean     GetAppIcon(FSSpecPtr theSpec, short iconID);
static void DrawBWIcon(short iconID, Rect *iconRect);
void        GetVersion(StringPtr outString);
long        Get68kSize();
long        GetPPCSize(FSSpecPtr myFSSpec);
long        GetPPCFragmentCount();
OSErr       GetIconIDFromBNDL(FSSpecPtr theSpec, short *iconID);

#endif