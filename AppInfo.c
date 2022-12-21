#include <Files.h>

#include "constants.h"
#include "utils.h"
#include "plugin.h"

#include "DSUtils.h"

#include "AppInfo.h"

OSErr GenerateReport(Str255 filename, FSSpecPtr myFSSPtr, Str255 inVersion) {
    Str255      buff;
    Str255      name;
    OSErr       err = noErr;
    short       refNum;
    long        count;
    
    myCopyPStr(myFSSPtr->name, name);
    myAppendPStr(name, "\p ");
    myAppendPStr(name, inVersion);
    
    // Add ellipses if name too long
    if (name[0] > 27) {
        name[0] = 27;
        name[27] = 'É';
    }
    
    myCopyPStr("\p", buff);
    myAppendPStr(buff, "\p#define PROGRAM_NAME \"");   
    myAppendPStr(buff, name);
    myAppendPStr(buff, "\p\"");

    err = HCreate(
        myFSSPtr->vRefNum,
        myFSSPtr->parID,
        filename,
        kReportCreator,
        kReportFileType
   );
    
    if (err)
        return err;
    
    err = HOpenDF(
        myFSSPtr->vRefNum,
        myFSSPtr->parID,
        filename,
        fsWrPerm,
        &refNum
   );
    
    if (err)
        return err;
        
    SetFPos(refNum, fsFromStart, 0);
    
    count = buff[0];

    err = FSWrite(
        refNum,
        &count,
        &buff[1]
   );

    FSClose(refNum);
    
    return err;
}

Boolean GetAppIcon(FSSpecPtr theSpec, short iconID) {
    OSErr           err = noErr;
    short           oldResRef;
    short           resRef;
    CGrafPtr        origPort;
    GDHandle        origDev;
    PixMapHandle    myGWorldPixMapH;
    Handle          iconRes;
    Rect            iconRect;
    RGBColor        foreColor;
    
    Boolean         result = false;

    // Default to current res if no spec specified
    if (theSpec) {
        if (!theSpec->vRefNum) return false;
        oldResRef = CurResFile();
        resRef = FSpOpenResFile(theSpec, fsRdPerm);
        if (!resRef) return false;
    }
    
    SetRect(&iconRect, kOffscreenIconRect);
    GetGWorld(&origPort, &origDev);
    SetGWorld(gOffscreenGWorld, NULL);
    myGWorldPixMapH = GetGWorldPixMap(gOffscreenGWorld);
    LockPixels(myGWorldPixMapH);
    EraseRect(&iconRect);
    
    SetResLoad(false);
    iconRes = Get1Resource(kLarge8BitData, iconID);
    SetResLoad(true);
    
    // Paint a rectangle the same color as the dialog box
    GetForeColor(&foreColor);
    RGBForeColor(&kDialogBGColor);    
    PaintRect(&iconRect);
    RGBForeColor(&foreColor);
    
    if (iconRes == NULL) {
        DrawBWIcon(iconID, &iconRect);
    } else {
        ReleaseResource(iconRes);
        err = PlotIconID(&iconRect, atAbsoluteCenter, kTransformNone, iconID);
    }
    
    UnlockPixels(myGWorldPixMapH);
    SetGWorld(origPort, origDev);
    
    if (theSpec) {
        UseResFile(oldResRef);
    	CloseResFile(resRef);
    }
    
	return (err == noErr);
}

Boolean CustomIconExists(FSSpecPtr theSpec) {
    short       oldResRef;
    short       resRef;
    Handle      resH;
    Boolean     result;
    
    if (!theSpec->vRefNum) return false;
    oldResRef = CurResFile();
    resRef = FSpOpenResFile(theSpec, fsRdPerm);
    if (!resRef) return false;
    
    SetResLoad(false);
    resH = Get1Resource(kLarge8BitData, kCustomIconResource);
    SetResLoad(true);
    result = (resH != NULL);
    
    UseResFile(oldResRef);
    CloseResFile(resRef);
    return result;
}

static void DrawBWIcon(short iconID, Rect *iconRect) {
    Handle      icon;
    BitMap      source, destination;
    GrafPtr     port;

    icon = Get1Resource(kLarge1BitMask, iconID);

    if (icon != NULL) {
        HLock(icon);
        // Prepare the source and destination bitmaps
        source.baseAddr = *icon + 128; // Mask address
        source.rowBytes = 4;
        SetRect(&source.bounds, 0, 0, 32, 32);
        GetPort(&port);
        destination = port->portBits;
        // Transfer the mask
        CopyBits(&source, &destination, &source.bounds, iconRect, srcBic, NULL);
        // Then the icon
        source.baseAddr = *icon;
        CopyBits(&source, &destination, &source.bounds, iconRect, srcOr, NULL);
        ReleaseResource(icon);
    }
}

OSErr GetIconIDFromBNDL(FSSpecPtr theSpec, short *iconID) {
    OSErr       err = kNoBNDLIconErr; // Default to an error
    Handle      bundleH;
    int         resRef;
    int         oldResRef;
    short       resCount;
    Size        byteCounter;
    
    oldResRef = CurResFile();
    resRef = FSpOpenResFile(theSpec, fsRdPerm);
    if (ResError()) return ResError();
    
    // The Apple official ID is 128
    bundleH = Get1Resource('BNDL', 128);
    
    // But that's not always the case, so just get the first one in the resource
    if (bundleH == NULL) {
        resCount = Count1Resources('BNDL');
        
        if (resCount > 0) bundleH = GetIndResource('BNDL', 1); // 1-based index
    }
    
    if (bundleH == NULL) {
        UseResFile(oldResRef);
        CloseResFile(resRef);
        return resNotFound;
    }
    
    HLock(bundleH);   
    
    for (byteCounter = 0 ; byteCounter < GetHandleSize(bundleH) ; byteCounter += 2) {
        // Search for the ICN# reference    
        if (*((UInt32 *)&((*bundleH)[byteCounter])) == kLarge1BitMask) {
            // Skip 8 bytes to get the first ID entry in the array
            BlockMove(&(*bundleH)[byteCounter + 8], iconID, 2);
            err = noErr;
            break;
        }
    }
    
    HUnlock(bundleH);
    UseResFile(oldResRef);
    CloseResFile(resRef);
    
    return err;
}

long Get68kSize() {
    register short  i;
    register long   resSize = 0;
    
    Handle      resH;
    short       resCount;
    
    resCount = Count1Resources('CODE');    
    SetResLoad(false);
    
    for (i = 1 ; i <= resCount ; ++i) {
        resH = Get1IndResource('CODE', i);
        HLock(resH);
        resSize += GetResourceSizeOnDisk(resH);
        HUnlock(resH);
        ReleaseResource(resH);
    }
    
    SetResLoad(true);
    
    return resSize;
}

long GetPPCSize(FSSpecPtr myFSSPtr) {
    long        dataForkSize = 0;
    unsigned    ppcSize = 0;
    short       refNum;
    
    FSpOpenDF(myFSSPtr, fsRdPerm, &refNum);
    GetEOF(refNum, &dataForkSize);    
    FSClose(refNum);
    
    return dataForkSize;
}

long GetPPCFragmentCount() {
    Handle      resH;
    short       resCount;
    long        fragmentCount = 0;
    
    resCount = Count1Resources(kCFragResourceType);
    
    if (resCount > 0) {
        resH = Get1IndResource(kCFragResourceType, 1);
        HLock(resH);
        fragmentCount = (*(CFragResourceHandle)(resH))->memberCount;
        HUnlock(resH);
        ReleaseResource(resH);
    }
    
    return fragmentCount;
}

void GetVersion(StringPtr s) {
	VersRecHndl     versH;
	UInt8           major;
	UInt8           minor;
	UInt8           bug;
	UInt8           stage;
	UInt8           nonRelRev;
	Str255          buff;
	Str15           initText = kInitText;
	char            stageChar;
	
	versH = (VersRecHndl)Get1Resource('vers', 2);
	if (versH == NULL) versH = (VersRecHndl)Get1Resource('vers', 1);
		
    if (versH == NULL) {
        myCopyPStr(initText, s);
        return;
    }

	HLock((Handle)versH);
	
	major = (*versH)->numericVersion.majorRev;	
	minor = (*versH)->numericVersion.minorAndBugRev >> 4;
	bug = (*versH)->numericVersion.minorAndBugRev & 0xf;
	stage = (*versH)->numericVersion.stage;
	nonRelRev = (*versH)->numericVersion.nonRelRev;
	
	switch (stage) {
	    case developStage:
	        stageChar = 'd';
	        break;
	    case alphaStage:
	        stageChar = 'a';
	        break;
	    case betaStage:
	        stageChar = 'b';
	        break;
	    default:
	        break;
	}
	
	NumToString(major, buff);	
	myCopyPStr(buff, s);
	myAppendCharToPStr(s, '.');
	NumToString(minor, buff);
	myAppendPStr(s, buff);
	myAppendCharToPStr(s, '.');
	NumToString(bug, buff);
	myAppendPStr(s, buff);
	
	if (stage != finalStage) {
	    myAppendCharToPStr(s, stageChar);   
	    NumToString(nonRelRev, buff);
	    myAppendPStr(s, buff);
	}
	
	HUnlock((Handle)versH);
	ReleaseResource((Handle)versH);
}

OSType GetFileType(FSSpecPtr inSpec) {
	FInfo finderInfo;
	
	FSpGetFInfo(inSpec, &finderInfo);
	return finderInfo.fdType;
}
