/******************************************************************************
**
**  Project Name:	DropShell
**     File Name:	DSUserProcs.c
**
**   Description:	Specific AppleEvent handlers used by the DropBox
**
*******************************************************************************
**                       A U T H O R   I D E N T I T Y
*******************************************************************************
**
**	Initials	Name
**	--------	-----------------------------------------------
**	LDR			Leonard Rosenthol
**	MTC			Marshall Clow
**	SCS			Stephan Somogyi
**
*******************************************************************************
**                      R E V I S I O N   H I S T O R Y
*******************************************************************************
**
**	  Date		Time	Author	Description
**	--------	-----	------	---------------------------------------------
**	06/23/94			LDR		Added support for ProcessItem and ProcessFolder handling
**	02/20/94			LDR		Modified Preflight & Postflight to take item count
**	01/25/92			LDR		Removed the use of const on the userDataHandle
**	12/09/91			LDR		Added the new SelectFile userProc
**								Added the new Install & DisposeUserGlobals procs
**								Modified PostFlight to only autoquit on odoc, not pdoc
**	11/24/91			LDR		Added the userProcs for pdoc handler
**								Cleaned up the placement of braces
**								Added the passing of a userDataHandle
**	10/29/91			SCS		Changes for THINK C 5
**	10/28/91			LDR		Officially renamed DropShell (from QuickShell)
**								Added a bunch of comments for clarification
**	10/06/91	00:02	MTC		Converted to MPW C
**	04/09/91	00:02	LDR		Added to Projector
**
******************************************************************************/

#include <StandardFile.h>

#include "DSGlobals.h"
#include "DSUtils.h"

#include "AppInfo.h"
#include "plugin.h"
#include "utils.h"
#include "main.h"

#include "DSUserProcs.h"

// Static Prototypes
static OSErr ProcessFolder(FSSpecPtr myFSSPtr);

/*
	This routine is called during init time.
	
	It allows you to install more AEVT Handlers beyond the standard four
*/
#pragma segment Main
pascal void InstallOtherEvents (void) {
}


/*	
	This routine is called when an OAPP event is received.
	
	Currently, all it does is set the gOApped flag, so you know that
	you were called initally with no docs, and therefore you shouldn't 
	quit when done processing any following odocs.
*/
#pragma segment Main
pascal void OpenApp (void) {
	gOApped = true;
}


/*	
	This routine is called when an QUIT event is received.
	
	We simply set the global done flag so that the main event loop can
	gracefully exit.  We DO NOT call ExitToShell for two reasons:
	1) It is a pretty ugly thing to do, but more importantly
	2) The Apple event manager will get REAL upset!
*/
#pragma segment Main
pascal void QuitApp (void) {
	gDone = true;	/*	All Done! */
}


/*	
	This routine is the first one called when an ODOC or PDOC event is received.
	
	In this routine you would place code used to setup structures, etc. 
	which would be used in a 'for all docs' situation (like "Archive all
	dropped files")

	Obviously, the opening boolean tells you whether you should be opening
	or printing these files based on the type of event recieved.
	
	NEW IN 2.0!
	The itemCount parameter is simply the number of items that were dropped on
	the application and that you will be processing.  This gives you the ability
	to do a single preflight for memory allocation needs, rather than doing it
	once for each item as in previous versions.
	
	userDataHandle is a handle that you can create & use to store your own
	data structs.  This dataHandle will be passed around to the other 
	odoc/pdoc routines so that you can get at your data without using
	globals - just like the new StandardFile.  
	
	We also return a boolean to tell the caller if you support this type
	of event.  By default, our dropboxes don't support the pdoc, so when
	opening is FALSE, we return FALSE to let the caller send back the
	proper error code to the AEManager.

	You will probably want to remove the #pragma unused (currently there to fool the compiler!)
*/
#pragma segment Main
pascal Boolean PreFlightDocs (Boolean opening, short itemCount, Handle *userDataHandle) {
#pragma unused (itemCount)
#pragma unused (userDataHandle)

	return opening;		// we support opening, but not printing - see above
}


/*	
	This routine is called for each file passed in the ODOC event.
	
	In this routine you would place code for processing each file/folder/disk that
	was dropped on top of you.
	
	You will probably want to remove the #pragma unused (currently there to fool the compiler!)
*/
#pragma segment Main
pascal void OpenDoc (FSSpecPtr myFSSPtr, Boolean opening, Handle userDataHandle) {
#pragma unused (opening)
#pragma unused (userDataHandle)
	OSErr	err = noErr;

	err = ProcessItem(myFSSPtr);
	
	switch (err) {
	    case noErr:	        
		    //QuitApp();
	        break;
	    case kUserCancelErr:
	        break;
	    case dupFNErr:
	        ErrorAlert(kErrStringListID, ERROR(kOutputExistsErr));
	        break;
	    default:
	        ErrorAlert(kErrStringListID, ERROR(kOutputErr));
	        break;
	}
}


/*	
	This routine is the last routine called as part of an ODOC event.
	
	In this routine you would place code to process any structures, etc. 
	that you setup in the PreflightDocs routine.

	NEW IN 2.0!
	The itemCount parameter was the number of items that you processed.
	It is passed here just in case you need it ;)  
	
	If you created a userDataHandle in the PreFlightDocs routines, this is
	the place to dispose of it since the Shell will NOT do it for you!
	
	You will probably want to remove the #pragma unusued (currently there to fool the compiler!)
*/
#pragma segment Main
pascal void PostFlightDocs (Boolean opening, short itemCount, Handle userDataHandle) {
#pragma unused (opening)
#pragma unused (itemCount)
#pragma unused (userDataHandle)

	//if ((opening) && (!gOApped))
		//gDone = true;	//	close everything up!

	/*
		The reason we do not auto quit is based on a recommendation in the
		Apple event Registry which specifically states that you should NOT
		quit on a 'pdoc' as the Finder will send you a 'quit' when it is 
		ready for you to do so.
	*/
}


/*
	This routine gets called for each item (which could be either a file or a folder)
	that the caller wants dropped.
*/
OSErr ProcessItem(FSSpecPtr myFSSPtr)
{
	OSErr               err = noErr;
	short               resRef, oldResRef;
	long                ppcSize;
	long                ppcFragmentCount;
	long                codeSize;   //68k CODE size
	//Str255              reportName;
	Str31               yesPStr = "\pYes";
	Str31               noPStr = "\pNo";
	Str31               version;
	Str31               codeResult;
	Str31               ppcResult;
	Str31               typeCreator;
	char                osTypeBuff[5];
	Str255              buff;
	PluginStrOutput     pluginStrOutput;
	FInfo               finderInfo;
	
	//StandardFileReply	stdReply;
	
	//short               resCount;
	short               iconID = 0;
	
    /* Only process applications */
    if (
        !(GetFileType(myFSSPtr) == kGenericApplicationIcon) &&
        !(GetFileType(myFSSPtr) == kGenericControlPanelIcon) &&
        !(GetFileType(myFSSPtr) == kGenericDeskAccessoryIcon)
    ) {
        //ErrorAlert(kErrStringListID, ERROR(kNotAPPLErr));
        
        return(err);
    }

    oldResRef = CurResFile();
    resRef = FSpOpenResFile(myFSSPtr, fsRdPerm);
    
    if (resRef == NULL) {
        ErrorAlert(kErrStringListID, ERROR(kResourceForkErr));
        return(err);
    }

    UseResFile(resRef);
    GetVersion(version);
    FSpGetFInfo(myFSSPtr, &finderInfo);
    osTypeBuff[0] = 4;
    *(OSType *)(osTypeBuff + 1) = finderInfo.fdType;
    myCopyPStr((StringPtr)osTypeBuff, typeCreator);
    myAppendPStr(typeCreator, "\p / ");
    *(OSType *)(osTypeBuff + 1) = finderInfo.fdCreator;
    myAppendPStr(typeCreator, (StringPtr)osTypeBuff);
    ppcSize = GetPPCSize(myFSSPtr);    
    ppcFragmentCount = GetPPCFragmentCount();
    codeSize = Get68kSize();
    
    if (codeSize) {
        myCopyPStr(yesPStr, codeResult);
        
        if (
            (codeSize < kMax68kCodeSizePPCAlert) &&
            ppcFragmentCount &&
            (ppcSize > kMinPPCCodeSizePPCAlert)
        ) {
            myAppendPStr(codeResult, "\p (likely \"PPC only\" alert)");
        }
    } else {
        myCopyPStr(noPStr, codeResult);
    }
    
    if (ppcFragmentCount) {
        myCopyPStr(yesPStr, ppcResult);
        NumToString(ppcFragmentCount, buff);
        myAppendPStr(ppcResult, "\p (");        
        myAppendPStr(ppcResult, buff);        
        myAppendPStr(ppcResult, "\p code fragment");
        if (ppcFragmentCount > 1) myAppendCharToPStr(ppcResult, 's');        
        myAppendCharToPStr(ppcResult, ')');
    } else {
        myCopyPStr(noPStr, ppcResult);
    }
    
    myCopyPStr(kInitText, pluginStrOutput.language);
    myCopyPStr(kInitText, pluginStrOutput.framework);
    myCopyPStr(kInitText, pluginStrOutput.engine );
    myCopyPStr(kInitText, pluginStrOutput.copyProtection);
    
    ExecPlugins(myFSSPtr, gPluginsH, gPluginCount, &pluginStrOutput);
    
    TESetText(myFSSPtr->name + 1, myFSSPtr->name[kcApplicationName - kcTEFirstIndex], gTEHandles[kcApplicationName - kcTEFirstIndex]);
    
    TESetText(version+1, *version, gTEHandles[kcDataVersion-kcTEFirstIndex]);
    TESetText(typeCreator+1, *typeCreator, gTEHandles[kcDataTypeCreator-kcTEFirstIndex]);
    TESetText(codeResult+1, *codeResult, gTEHandles[kcData68k-kcTEFirstIndex]);
    TESetText(ppcResult+1, *ppcResult, gTEHandles[kcDataPPC-kcTEFirstIndex]);
    TESetText(pluginStrOutput.language+1, pluginStrOutput.language[0], gTEHandles[kcDataLanguage-kcTEFirstIndex]);
    TESetText(pluginStrOutput.framework+1, pluginStrOutput.framework[0], gTEHandles[kcDataFramework-kcTEFirstIndex]);
    TESetText(pluginStrOutput.engine+1, pluginStrOutput.engine[0], gTEHandles[kcDataEngine-kcTEFirstIndex]);
    TESetText(pluginStrOutput.copyProtection+1, pluginStrOutput.copyProtection[0], gTEHandles[kcDataCopyProtection-kcTEFirstIndex]);
    
    if (CustomIconExists(myFSSPtr)) {
        GetAppIcon(myFSSPtr, kCustomIconResource);
    } else {
        if (GetIconIDFromBNDL(myFSSPtr, &iconID) == noErr) {
            GetAppIcon(myFSSPtr, iconID);
        } else {
            UseResFile(oldResRef);
            GetAppIcon(NULL, kGenericApplicationIconResource);
            UseResFile(resRef);      
        }
    }
         
    UseResFile(oldResRef);
    CloseResFile(resRef);
        
    gIconAvailable = true;
    gError = false;
    //SetStatus(NULL);
    DoUpdate(gMainDialog);
    
    /*myCopyPStr(myFSSPtr->name, reportName);
    myAppendPStr(reportName, kReportSuffix);
    
    StandardPutFile(NULL, reportName, &stdReply);
	
	if (stdReply.sfGood) {
    	// Now spit out data
    	err = GenerateReport(stdReply.sfFile.name, myFSSPtr, version);
    } else {
        err = kUserCancelErr;
    }*/
	
	return(err);
}

/*
	This routine is called when the user chooses "Select FileÉ" from the
	File Menu.
	
	Currently it simply calls the new StandardGetFile routine to have the
	user select a single file (any type, numTypes = -1) and then calls the
	SendODOCToSelf routine in order to process it.  
			
	The reason we send an odoc to ourselves is two fold: 1) it keeps the code
	cleaner as all file openings go through the same process, and 2) if events
	are ever recordable, the right things happen (this is called Factoring!)

	Modification of this routine to only select certain types of files, selection
	of multiple files, and/or handling of folder & disk selection is left 
	as an exercise to the reader.
*/
pascal void SelectFile (void)
{
	StandardFileReply	stdReply;
	SFTypeList			theTypeList = {
	                        kGenericApplicationIcon,
	                        kGenericControlPanelIcon,
	                        kGenericDeskAccessoryIcon,
	                        kApplicationAliasType
	                    };

	StandardGetFile(NULL, 1, theTypeList, &stdReply);
	if (stdReply.sfGood) {  // user did not cancel
		SendODOCToSelf(&stdReply.sfFile);	// so send me an event!
	}
}

/*
	This routine is called during the program's initialization and gives you
	a chance to allocate or initialize any of your own globals that your
	dropbox needs.
	
	You return a boolean value which determines if you were successful.
	Returning false will cause DropShell to exit immediately.
*/
pascal Boolean InitUserGlobals(void)
{
    gMainDialog 	    = NULL;
	gInBackground       = false;
	gError              = false;
	gInvalidDrag        = false;
	gHasDragManager     = true;
	gOffscreenGWorld    = NULL;
    gIconAvailable      = false;
    gPluginCount        = 0;
    gPluginsH           = (ParsedPluginH**)NewHandle(0);
    
	return(true);	// nothing to do, it we must be successful!
}

/*
	This routine is called during the program's cleanup and gives you
	a chance to deallocate any of your own globals that you allocated 
	in the above routine.
*/
pascal void DisposeUserGlobals(void)
{
    short   i;
    
    for (i = 0 ; i < kTETextBoxCount ; ++i) TEDispose(gTEHandles[i]);
	DisposeGWorld(gOffscreenGWorld);
	DisposeDialog(gMainDialog);
	ReleaseResource((Handle)gAppleMenu);
	ReleaseResource((Handle)gFileMenu);
}
