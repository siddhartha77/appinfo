#ifndef	__DSGLOBALS_H__
#define	__DSGLOBALS_H__


#ifndef __MWERKS__
#include <Types.h>
#include <Memory.h>
#include <QuickDraw.h>
#include <OSUtils.h>
#include <ToolUtils.h>
#include <Menus.h>
#include <Packages.h>
#include <Traps.h>
#include <Files.h>
#endif

#include <Aliases.h>
#include <AppleEvents.h>
#include <Gestalt.h>
#include <Processes.h>

#include "constants.h"
#include "plugin.h"
	
extern Boolean		    gDone, gOApped, gWasEvent, gInBackground, gError,
                        gHasDragManager, gIconAvailable, gInvalidDrag;
extern short            gPluginCount;
extern ParsedPluginH    **gPluginsH;
extern EventRecord	    gEvent;
extern MenuHandle	    gAppleMenu, gFileMenu;
extern DialogPtr	    gMainDialog;
extern GWorldPtr	    gOffscreenGWorld;
extern Rect             gTERects[kTETextBoxCount];
extern TEHandle         gTEHandles[kTETextBoxCount];

#endif
