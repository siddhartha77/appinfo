#include <Controls.h>
#include <Devices.h>
#include <Dialogs.h>
#include <Drag.h>
#include <Errors.h>
#include <Events.h>
#include <Files.h>
#include <Fonts.h>
#include <Memory.h>
#include <Menus.h>
#include <MixedMode.h>
#include <StandardFile.h>
#include <QuickDraw.h>
#include <TextEdit.h>
#include <Types.h>
#include <Windows.h>

#include "constants.h"
#include "utils.h"
#include "plugin.h"

#include "DSGlobals.h"
#include "DSUserProcs.h"
#include "DSAppleEvents.h"

#include "main.h"

Boolean		    gDone, gOApped, gWasEvent, gInBackground, gError,
                gInvalidDrag, gHasDragManager, gIconAvailable;
short           gPluginCount;
ParsedPluginH   **gPluginsH;
EventRecord	    gEvent;
GWorldPtr       gOffscreenGWorld;
MenuHandle	    gAppleMenu, gFileMenu;
DialogPtr	    gMainDialog;
TEHandle        gTEHandles[kTETextBoxCount];
Rect            gTERects[kTETextBoxCount];
HFSFlavor       gDragHFSData;

#pragma segment Initialize
void InitToolbox (void) {

#ifdef MPW
	UnloadSeg ((Ptr) _DataInit);
#endif

    MaxApplZone();

	MoreMasters();
	MoreMasters();
	MoreMasters();
	MoreMasters();
	
	InitGraf (&qd.thePort);
	InitFonts ();
	InitWindows ();
	InitMenus ();
	TEInit ();
	InitDialogs (NULL);		// use of ResumeProcs no longer approved by Apple
	InitCursor ();
	FlushEvents (everyEvent, 0);
}

/*
	Let's setup those global variables that the DropShell uses.
	
	If you add any globals for your own use,
	init them in the InitUserGlobals routine in DSUserProcs.c
*/
#pragma segment Initialize
Boolean InitGlobals (void) {
	gDone			    = false;
	gOApped			    = false;	// probably not since users are supposed to DROP things!	

	return(InitUserGlobals());	// call the user proc
}

#pragma segment Initialize
Error CheckGestalts() {
    long    response;
    
	if (Gestalt(gestaltAppleEventsAttr, &response)) return ERROR(kGestaltAppleEventsErr);
	//if (Gestalt(gestaltDialogMgrAttr, &response)) return ERROR(kGestaltDialogMgrErr);
	if (Gestalt(gestaltFSAttr, &response) || !(response &  (1 << gestaltHasFSSpecCalls))) return ERROR(kGestaltFSErr);
	if (Gestalt(gestaltResourceMgrAttr, &response)) return ERROR(kGestaltResourceMgrErr);
	//if (Gestalt(gestaltIconUtilitiesAttr, &response)) return ERROR(kGestaltIconUtilitiesErr);
	if (Gestalt(gestaltStandardFileAttr, &response) || !(response &  (1 << gestaltStandardFile58))) return ERROR(kGestaltStandardFileErr);
	if (Gestalt(gestaltDragMgrAttr, &response)) gHasDragManager = false;
	
	return ERROR(noErr);
}

/*
	Again, nothing fancy.  Just setting up the menus.
	
	If you add any menus to your DropBox - insert them here!
*/
#pragma segment Initialize
void SetUpMenus (void) {

	gAppleMenu = GetMenu (kAppleNum);
	AppendResMenu (gAppleMenu, 'DRVR');
	InsertMenu (gAppleMenu, 0);

	gFileMenu = GetMenu (kFileNum);
	InsertMenu (gFileMenu, 0);
	DrawMenuBar ();
}

/*	--------------- Standard Event Handling routines ---------------------- */
#pragma segment Main
void ShowAbout () {
    CenterAlert (kAlertAbout);
	(void) Alert (kAlertAbout, NULL);
}

#pragma segment Main
void SetStatus(StringPtr s) {
    if (s == NULL) {
        TESetText(" ", 1, gTEHandles[kcTEStatus - kcTEFirstIndex]);
    } else {
        TESetText(s + 1, *s, gTEHandles[kcTEStatus - kcTEFirstIndex]);
    }
}

#pragma segment Main
void ResetTextBoxes() {

    Str15   initText = kInitText;
    Str255  strApplicationHeader;
    short   i;
    
    // Application Name
    if (gHasDragManager) {
        GetIndString(strApplicationHeader, kMainDlgStrList, kMainDlgLblAppDrag);
    } else {
        GetIndString(strApplicationHeader, kMainDlgStrList, kMainDlgLblAppSelect);
    }
    
    TESetText(strApplicationHeader + 1, *strApplicationHeader, gTEHandles[kcApplicationName - kcTEFirstIndex]);
    
    for (i = kcDataFirst - kcTEFirstIndex ; i < (kcDataFirst - kcTEFirstIndex) + kTEDataRows ; ++i) {
        TESetText(initText + 1, *initText, gTEHandles[i]);
    }
}

#pragma segment Main
void DoUpdate (WindowPtr theWindow) {
    GrafPtr	tmpPort;
    
    GetPort(&tmpPort);
	SetPort(theWindow);        
    UpdateGWorld(&gOffscreenGWorld, 0, &theWindow->portRect, NULL, NULL, NULL);
    //BeginUpdate(theWindow);    
    //UpdateDialog(theWindow, theWindow->visRgn);
    //EndUpdate(theWindow);     
    DoActivateWindow(theWindow, !gInBackground);   
    SetPort(tmpPort);
}

#pragma segment Main
void DoUpdateIcon() {
    Rect            iconRect;
    GrafPtr         savedPort;
    CGrafPtr        origPort;
    GDHandle        origDev;
    PixMapHandle    myGWorldPixMapH;
    Rect            sourceRect;
    RGBColor        backColor, foreColor;

    GetPort(&savedPort);
    SetPort(gMainDialog);
    GetDialogItem(gMainDialog, kcIcon, NULL, NULL, &iconRect);
    EraseRect(&iconRect);
    
    // Leave icon blank on error
    if (!gError) {
        GetGWorld(&origPort, &origDev);
        SetGWorld((CGrafPtr)gOffscreenGWorld, NULL);        
        SetRect(&sourceRect, kOffscreenIconRect);        
        myGWorldPixMapH = GetGWorldPixMap(gOffscreenGWorld);
        LockPixels(myGWorldPixMapH);    
        SetGWorld((CGrafPtr)gMainDialog, NULL);
        
        // Defaults to eliminate unwanted coloring
        GetBackColor(&backColor);
        GetForeColor(&foreColor);
        RGBBackColor(&kWhiteColor);
        RGBForeColor(&kBlackColor);
        
        // Copy over the icon from the offscreen world
        CopyBits(
            (BitMap *)*myGWorldPixMapH,
            &((GrafPtr)gMainDialog)->portBits,
            &sourceRect,
            &iconRect,
            srcCopy,
            gMainDialog->visRgn
        );
        
        RGBBackColor(&backColor);
        RGBForeColor(&foreColor);
        
        UnlockPixels(myGWorldPixMapH);
        SetGWorld(origPort, origDev);
    }
    
    SetPort(savedPort);
}

#pragma segment Main
void DoDialogEvent(EventRecord *curEvent) {
    short               itemHit;
    DialogPtr           dialog;
    StandardFileReply	stdReply;
    
    switch (curEvent->what) {
        case keyDown:
        case autoKey:
            DoKeyDown(curEvent);
            break;
        case updateEvt:
            DoUpdate((WindowPtr)curEvent->message);
            break;
        case osEvt:
			DoOSEvent(curEvent);
			HiliteMenu(0);
			break;
		default:
            break;
    }
    
     if (DialogSelect(curEvent, &dialog, &itemHit)) {
        switch (itemHit) {
            case kcReportButton:
                DoActivateWindow(FrontWindow(), false);
                StandardPutFile(NULL, "\ptest.txt", &stdReply);
                DoActivateWindow(FrontWindow(), true);
                break;
            default:
                break;
        }
    }
}

#pragma segment Main
void DoOSEvent(EventRecord *curEvent) {
    switch((curEvent->message >> 24) & 0x000000FF) {
		case suspendResumeMessage:
			gInBackground = (curEvent->message & resumeFlag) == 0;
			DoUpdate(FrontWindow());
			break;
				
		case mouseMovedMessage:
			break;
	}
}

#pragma segment Main
void DoActivate(EventRecord *curEvent) {
    WindowPtr   whichWindow;
    
    FindWindow (curEvent->where, &whichWindow);
    DoActivateWindow(whichWindow, curEvent->modifiers & activeFlag);
}

#pragma segment Main
void DoActivateWindow(WindowPtr whichWindow, Boolean becomingActive) {
    DialogItemType  itemType;
    Handle          itemHandle;
    Rect            itemRect;
    short           hilite;
    int             i;
    
    hilite = becomingActive ? 0 : 255;

    GetDialogItem(whichWindow, kcReportButton, &itemType, &itemHandle, &itemRect);
    HiliteControl((ControlHandle)itemHandle, hilite);
    RGBForeColor(&kBlackColor); 
    if (gIconAvailable) DoUpdateIcon();
    if (!becomingActive) RGBForeColor(&kGrayColor);
    for (i = 0 ; i < kTETextBoxCount ; ++i) TEUpdate(&gTERects[i], gTEHandles[i]);
    
    // Error status text
    if (gError) {
        if (becomingActive) {
            // Red
            RGBForeColor(&kRedColor);
        } else {
            // Dimmed red
            RGBForeColor(&kLightRedColor);
        }
    }
    
    TEUpdate(&gTERects[kcTEStatus - kcTEFirstIndex], gTEHandles[kcTEStatus - kcTEFirstIndex]);
    
    DrawSeparator(whichWindow, kcSeparator1, becomingActive);
    DrawSeparator(whichWindow, kcSeparator2, becomingActive);
    
    // Reset to black default
    RGBForeColor(&kBlackColor);
}

#pragma segment Main
void DrawSeparator(WindowPtr whichWindow, DialogItemIndex itemNo, Boolean becomingActive) {
    DialogItemType  itemType;
    Handle          itemHandle;
    Rect            itemRect;
    RGBColor        savedColor;
    
    GetDialogItem(whichWindow, itemNo, &itemType, &itemHandle, &itemRect);    
    GetForeColor(&savedColor);    
    RGBForeColor(&kDarkGrayColor);
    MoveTo(itemRect.left, itemRect.top);
    LineTo(itemRect.right,itemRect.bottom);
    
    if (becomingActive) {
        RGBForeColor(&kWhiteColor);
    } else {
        // Erase the highlight line
        RGBForeColor(&kDialogBGColor);
    }
    
    MoveTo(itemRect.left + 1, itemRect.top + 1);
    LineTo(itemRect.right + 1, itemRect.bottom + 1);        
    RGBForeColor(&savedColor);
}

#pragma segment Main
void DoMenu (long retVal) {
	short	menuID, itemID;
	Str255	itemStr;

	menuID = HiWord (retVal);
	itemID = LoWord (retVal);
	
	switch (menuID) {
		case kAppleNum:
			if (itemID == 1)
				ShowAbout ();	//	Show the about box
			else
			{
				GetMenuItemText(GetMenuHandle(kAppleNum), itemID, itemStr);
				OpenDeskAcc(itemStr);
			}
			break;
			
		case kFileNum:
			if (itemID == kFileGetInfo)
				SelectFile();		// call file selection userProc
			else
				SendQuitToSelf();	// send self a 'quit' event
			break;
		
		default:
			break;
			
		}
	HiliteMenu(0);		// turn it off!
	}


#pragma segment Main
void DoMouseDown (EventRecord *curEvent) {
	WindowPtr	whichWindow;
	short		whichPart;

	whichPart = FindWindow (curEvent->where, &whichWindow);
	
	switch (whichPart) {
		case inMenuBar:
			DoMenu (MenuSelect (curEvent->where));
			break;
		
		case inSysWindow:
			SystemClick (curEvent, whichWindow);
			break;
		
		case inDrag:
		    {
    			Rect	boundsRect = (*GetGrayRgn())->rgnBBox;
    			DragWindow (whichWindow, curEvent->where, &boundsRect);
    			break;
    		}
		
		default:
			break;
	}
}

#pragma segment Main
void DoKeyDown (EventRecord *curEvent) {
	if (curEvent->modifiers & cmdKey) {
		DoMenu (MenuKey ((char) curEvent->message & charCodeMask));
	}
}

#pragma segment Main
void InitTextBoxes() {
    Str255      strVersion, strTypeCreator, str68k, strPPC, strLanguage, strFramework, strEngine, strCopyProtection;
    short       fontNumber = 0;
    Handle      itemHandle;
    short       itemType;
    int         i;
    GrafPtr     port;
    
    GetDialogItem(gMainDialog, kcApplicationName, &itemType, &itemHandle, &gTERects[kcApplicationName - kcTEFirstIndex]);
    gTEHandles[kcApplicationName - kcTEFirstIndex] = TENew(&gTERects[kcApplicationName - kcTEFirstIndex], &gTERects[kcApplicationName - kcTEFirstIndex]);
    
    // Status Text
    GetDialogItem(gMainDialog, kcTEStatus, &itemType, &itemHandle, &gTERects[kcTEStatus - kcTEFirstIndex]);
    gTEHandles[kcTEStatus - kcTEFirstIndex] = TENew(&gTERects[kcTEStatus - kcTEFirstIndex], &gTERects[kcTEStatus - kcTEFirstIndex]);
  
    // Setup small text
    port = (GrafPtr)gMainDialog;
    GetFNum(kInfoFontFamily, &fontNumber);    
    if (fontNumber) TextFont(fontNumber);
    TextFace(bold);
    TextSize(kInfoFontSize);
    
    // Headers
    for (i = kcHeaderFirst - kcTEFirstIndex ; i < (kcHeaderFirst + kTEDataRows) - kcTEFirstIndex ; ++i) {    
        GetDialogItem(gMainDialog, kcTEFirstIndex + i, &itemType, &itemHandle, &gTERects[i]);
        gTEHandles[i] = TENew(&gTERects[i], &gTERects[i]);
        TESetAlignment(teFlushRight, gTEHandles[i]);
    }
    
    GetIndString(strVersion, kMainDlgStrList, kMainDlgLblVersion);
    GetIndString(strTypeCreator, kMainDlgStrList, kMainDlgLblTypeCreator);
    GetIndString(str68k, kMainDlgStrList, kMainDlgLbl68k);
    GetIndString(strPPC, kMainDlgStrList, kMainDlgLblPPC);
    GetIndString(strLanguage, kMainDlgStrList, kMainDlgLblLanguage);
    GetIndString(strFramework, kMainDlgStrList, kMainDlgLblFramework);
    GetIndString(strEngine, kMainDlgStrList, kMainDlgLblEngine);
    GetIndString(strCopyProtection, kMainDlgStrList, kMainDlgLblCopyProtection);
    
    TESetText(strVersion + 1, *strVersion, gTEHandles[kcHeaderVersion - kcTEFirstIndex]);
    TESetText(strTypeCreator + 1, *strTypeCreator, gTEHandles[kcHeaderTypeCreator - kcTEFirstIndex]);
    TESetText(str68k + 1, *str68k, gTEHandles[kcHeader68k - kcTEFirstIndex]);
    TESetText(strPPC + 1, *strPPC, gTEHandles[kcHeaderPPC - kcTEFirstIndex]);
    TESetText(strLanguage + 1, *strLanguage, gTEHandles[kcHeaderLanguage - kcTEFirstIndex]);
    TESetText(strFramework + 1, *strFramework, gTEHandles[kcHeaderFramework - kcTEFirstIndex]);
    TESetText(strEngine + 1, *strEngine, gTEHandles[kcHeaderEngine - kcTEFirstIndex]);
    TESetText(strCopyProtection + 1, *strCopyProtection, gTEHandles[kcHeaderCopyProtection - kcTEFirstIndex]);
    
    port = (GrafPtr)gMainDialog;
    if (fontNumber) TextFont(fontNumber);
    TextFace(normal);
    
    // Data  
    for (i = kcDataFirst -  kcTEFirstIndex; i < (kcDataFirst + kTEDataRows) - kcTEFirstIndex ; ++i) {    
        GetDialogItem(gMainDialog, kcTEFirstIndex + i, &itemType, &itemHandle, &gTERects[i]);
        gTEHandles[i] = TENew(&gTERects[i], &gTERects[i]);
    }
}

#pragma segment Main
OSErr InitDragManager() {
    OSErr       result = noErr;

#if TARGET_CPU_PPC
    #define uppTrackingHandlerProcInfo kPascalStackBased \
    | RESULT_SIZE(SIZE_CODE(sizeof(OSErr))) \
    | STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(DragTrackingMessage))) \
    | STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(WindowPtr))) \
    | STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(void*))) \
    | STACK_ROUTINE_PARAMETER(4, SIZE_CODE(sizeof(DragReference)))
    
    #define uppReceiveHandlerProcInfo kPascalStackBased \
    | RESULT_SIZE(SIZE_CODE(sizeof(OSErr))) \
    | STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(WindowPtr))) \
    | STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(void*))) \
    | STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(DragReference)))
    
    static RoutineDescriptor   rdTracking = BUILD_ROUTINE_DESCRIPTOR(uppTrackingHandlerProcInfo, MyTrackingHandler);
    static RoutineDescriptor   rdReceive = BUILD_ROUTINE_DESCRIPTOR(uppReceiveHandlerProcInfo, MyReceiveHandler);
    
    result = InstallTrackingHandler(&rdTracking, 0L, NULL);
    if (result != noErr) return result;
    result = InstallReceiveHandler(&rdReceive, 0L, NULL);
#elif TARGET_CPU_68K
    result = InstallTrackingHandler(&MyTrackingHandler, 0L, NULL);
    if (result != noErr) return result;
    result = InstallReceiveHandler(&MyReceiveHandler, 0L, NULL);
#endif

    return result;
}

#pragma segment Main
pascal OSErr MyTrackingHandler(DragTrackingMessage theMessage, WindowPtr theWindow, void *handlerRefCon, DragReference theDrag) {
    #pragma unused (handlerRefCon)
    RgnHandle   hiliteRgn;
    
    switch(theMessage) {
        case kDragTrackingEnterHandler:
            break;
        case kDragTrackingEnterWindow:
            RectRgn(hiliteRgn = NewRgn(), &theWindow->portRect);
            ShowDragHilite(theDrag, hiliteRgn, true);
            DisposeRgn(hiliteRgn);
           
            break;
        case kDragTrackingInWindow:            
            break;
        case kDragTrackingLeaveWindow:
            HideDragHilite(theDrag);
            
            break;
        case kDragTrackingLeaveHandler:
            break;
    }
    return noErr;
}

#pragma segment Main
pascal OSErr MyReceiveHandler(WindowPtr theWindow, void *handlerRefCon, DragReference theDrag) {
    #pragma unused (handlerRefCon)
    OSErr           error = noErr;
    ItemReference   theItem;
    FlavorType      theFlavor;
    Size            dataSize;
    Boolean         targetIsFolder;
    Boolean         wasAliased;
    Str255          errorString;
    
    GetDragItemReferenceNumber(theDrag, 1, &theItem);
    GetFlavorType(theDrag, theItem, 1, &theFlavor);

    if (theFlavor == kDragFlavorTypeHFS) {
        GetFlavorDataSize(theDrag, theItem, kDragFlavorTypeHFS, &dataSize);
        GetFlavorData(theDrag, theItem, kDragFlavorTypeHFS, &gDragHFSData, &dataSize, 0L);
        
        if (
            (gDragHFSData.fileType == kGenericApplicationIcon) ||
            (gDragHFSData.fileType == kGenericControlPanelIcon) ||
            (gDragHFSData.fileType == kGenericDeskAccessoryIcon) ||
            (gDragHFSData.fileType == kApplicationAliasType)
        ) {
            if (gDragHFSData.fileType == kApplicationAliasType) {
                error = ResolveAliasFile(&gDragHFSData.fileSpec, true, &targetIsFolder, &wasAliased);
            }
            
            if (error == fnfErr) {
                gInvalidDrag = true;
                gError = true;
                HideDragHilite(theDrag);
                GetIndString(errorString, kErrStringListID, kBadAliasErr);
                SetStatus(errorString);
                ResetTextBoxes();       
            } else {
                gError = false;
                SetStatus("\p ");
                gInvalidDrag = false;
                ProcessItem(&gDragHFSData.fileSpec);
            }
            
            // Update the dialog and maintain dimming
            DoUpdate(theWindow);
        } else {
            gInvalidDrag = true;
            error = kInvalidDragErr;
        }
    } else {
        gInvalidDrag = true;
        error = kInvalidDragErr;
    }

    return error;
}

#pragma segment Main
void main () {
    Error       error;
    
	InitToolbox ();
	if (InitGlobals ()) {	// if we succeeding in initting self
	    error = CheckGestalts();
		if (error.err)
			ErrorAlert(kErrStartupStringListID, error);
		else {
			InitAEVTStuff();
			SetUpMenus();
			if (gHasDragManager) InitDragManager();
			gMainDialog = GetNewDialog(kMainDialog, NULL, (DialogPtr)-1L);			
			SetGrafPortOfDialog(gMainDialog);
	        if (gMainDialog) ShowWindow(gMainDialog);
	        DrawDialog(gMainDialog);
	        // OffscreenGWorld is used for icons
	        NewGWorld(&gOffscreenGWorld, 0, &gMainDialog->portRect, NULL, NULL, 0);
	        InitTextBoxes();
	        ResetTextBoxes();
	        error = LoadPlugins(gPluginsH, &gPluginCount);
	        
	        if (error.err) ErrorAlert(NULL, error);

			while (!gDone) {
				gWasEvent = WaitNextEvent(everyEvent, &gEvent, 0, NULL);
				if (gWasEvent) {
                    if (IsDialogEvent(&gEvent) ) {
                        DoDialogEvent(&gEvent);
                    } else {
    					switch (gEvent.what) {
    						case kHighLevelEvent:
    							DoHighLevelEvent (&gEvent);
    							break;
    							
    						case mouseDown:
    							DoMouseDown (&gEvent);
    							break;
    							
    						case keyDown:
    						case autoKey:
    							DoKeyDown (&gEvent);
    							break;
    					    case updateEvt:
    					        DoUpdate((WindowPtr)gEvent.message);    					       
    					        break;
    					        
    						case activateEvt:
    					        DoActivate(&gEvent);
    					        break;
    					        
    						default:
    							break;
    					}
    				}
				}
			}
		}

		DisposeUserGlobals();	// call the userproc to clean itself up		
		ExitToShell();
	}
}
