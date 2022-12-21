#ifndef	__MAIN_H__
#define	__MAIN_H__

#include "utils.h"

void			Panic(void);
void			InitToolbox(void);
Boolean			InitGlobals(void);
Error           CheckGestalts(void);
void			SetUpMenus(void);
OSErr           InitDragManager(void);
pascal OSErr    MyTrackingHandler(DragTrackingMessage theMessage, WindowPtr theWindow,
                    void *handlerRefCon, DragReference theDrag);
pascal OSErr    MyReceiveHandler(WindowPtr theWindow, void *handlerRefCon, DragReference theDrag);
void            InitTextBoxes(void);
void			ShowAbout(void);
void            SetStatus(StringPtr s);
void            ResetTextBoxes(void);
void            DoUpdate(WindowPtr theWindow);
void            DoDialogEvent(EventRecord *curEvent);
void            DoOSEvent(EventRecord *curEvent);
void            DoActivate(EventRecord *curEvent);
void            DoActivateWindow(WindowPtr whichWindow, Boolean becomingActive);
void            DrawSeparator(WindowPtr whichWindow, DialogItemIndex itemNo, Boolean becomingActive);
void			DoMenu(long retVal);
void			DoMouseDown(EventRecord *curEvent);
void            DoUpdateIcon(void);
void			DoKeyDown(EventRecord *curEvent);

#endif