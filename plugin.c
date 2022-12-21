#include "constants.h"

#include "DSUtils.h"
#include "main.h" // DEBUG for SetStatus

#include "plugin.h"

/*
    pluginArrayH is a handle to an array of ParsedPluginH(andles)
    On return, count contains the number of plugins in the array
*/
Error LoadPlugins(ParsedPluginH **pluginArrayH, short *count) {
    register short  i;
    
    Error           error;
    OSErr           err = noErr;
    Handle          pluginResH;
    ParsedPluginH   parsedPluginH;
    short           resCount;
    short           theID;
    ResType         theType;
    Str255          name;
    
    *count = 0;
    resCount = Count1Resources(kPluginResourceType);
    
    SetHandleSize((Handle)pluginArrayH, sizeof(Handle) * resCount);    
    err = MemError();    
    if (err) return ERROR(err);
    
    HLock((Handle)pluginArrayH);
    
    // 1-based index
    for (i = 1 ; i <= resCount ; ++i) {
        pluginResH = Get1IndResource(kPluginResourceType, i);
        
        if (pluginResH != NULL) {
            GetResInfo(pluginResH, &theID, &theType, name);
            parsedPluginH = (ParsedPluginH)NewHandle(sizeof(ParsedPlugin) - sizeof(Str255) + name[0] + 1);
            
            if (parsedPluginH != NULL) {
                HLock(pluginResH);
                HLock((Handle)parsedPluginH);                        
                BlockMove(name, (*parsedPluginH)->name, name[0] + 1);            
                err = MemError();
                error = ERROR(err);
                if (error.err) return error;
                error = ParsePlugin(pluginResH, parsedPluginH);
                HUnlock(pluginResH);
                HUnlock((Handle)parsedPluginH);
            } else {
                err = MemError();
                error = ERROR(err);
            }
                
            ReleaseResource(pluginResH);
            
            if (error.err) {
                HUnlock((Handle)pluginArrayH);
                return error;
            }
            
            // GOTCHA: Dereference the handle to get the pointer, then get the array index
            (*pluginArrayH)[i - 1] = parsedPluginH;
            ++*count;
        } else {
            HUnlock((Handle)pluginArrayH);
            return ERROR(ResError());
        }
    }
    
    HUnlock((Handle)pluginArrayH);
    
    return error;
 }

static Error ParsePlugin(Handle pluginDataH, ParsedPluginH parsedPluginH) {
    register short              i, j;
    register long               memOffset = 0;
    
    OSErr                       err = noErr;
    ParsedPluginPtr             parsedPluginPtr;
    ParsedPluginResourcePtr     parsedPluginResourcePtr;
    Handle                      parsedPluginResourceH = NULL;
    Handle                      parsedPluginDFIdentifierH = NULL;
    PluginResourcePtr           pluginResourcePtr;
    PluginDFIdentifierPtr       pluginDFIdentifierPtr;
    short                       resourceCount = 0;
    Size                        structSize = 0;
    
    parsedPluginPtr = *parsedPluginH;
    
    parsedPluginPtr->category = *(__typeof__(parsedPluginPtr->category) *)*pluginDataH;
     
    // Skip over the PString theType
    memOffset += sizeof(parsedPluginPtr->category);
    
    parsedPluginPtr->typeCount = *(__typeof__(parsedPluginPtr->typeCount) *)(*pluginDataH + memOffset);
    memOffset += sizeof(parsedPluginPtr->typeCount);
    
    if (parsedPluginPtr->typeCount > 0) {
        parsedPluginPtr->resourceTypesH = (ParsedPluginResourceH)NewHandle(sizeof(ParsedPluginResource) * parsedPluginPtr->typeCount);
        parsedPluginResourcePtr = (ParsedPluginResourcePtr)(*parsedPluginPtr->resourceTypesH);
        
        if (parsedPluginPtr->resourceTypesH != NULL) {
            HLock((Handle)parsedPluginPtr->resourceTypesH);
            
            // Build an array of handles to resource types
            for (i = 0 ; i < parsedPluginPtr->typeCount ; ++i) {
                parsedPluginResourcePtr[i].resourceTypeH = (PluginResourceTypeH)NewHandle(sizeof(PluginResourceType));
                **parsedPluginResourcePtr[i].resourceTypeH = *(PluginResourceTypePtr)(*pluginDataH + memOffset);
                //BlockMove((PluginResourceTypePtr)(*pluginDataH + memOffset), parsedPluginResourcePtr[i].resourceType, sizeof(PluginResourceType));
                memOffset += sizeof(PluginResourceType);
                
                resourceCount = (*parsedPluginResourcePtr[i].resourceTypeH)->resourceCount;
                parsedPluginResourcePtr[i].resourcesH = (PluginResourceH**)NewHandle(sizeof(Handle) * resourceCount);
                
                if (parsedPluginResourcePtr[i].resourcesH != NULL) {
                    HLock((Handle)parsedPluginResourcePtr[i].resourcesH);
                    
                    // For each type build an array of handles to resources
                    for (j = 0 ; j < resourceCount ; ++j ) {
                        pluginResourcePtr = (PluginResourcePtr)(*pluginDataH + memOffset);
                        
                        // structSize is used to size the handle as .data is a variable length PString
                        structSize = sizeof(PluginResource) - sizeof(Str255) + pluginResourcePtr->name[0] + 1;
                                                
                        err = PtrToHand(pluginResourcePtr, &parsedPluginResourceH, structSize);                        
                        if (err) return ERROR(err);
                        
                        ((Handle *)(*parsedPluginResourcePtr[i].resourcesH))[j] = parsedPluginResourceH;
                        memOffset += structSize;
                    }
                    
                    HUnlock((Handle)parsedPluginResourcePtr[i].resourcesH);
                }                
            }
            
            HUnlock((Handle)parsedPluginPtr->resourceTypesH);
        } else {
            return ERROR(MemError());
        }
    }
    
    parsedPluginPtr->dfIdentifierCount = *(__typeof__(parsedPluginPtr->dfIdentifierCount) *)(*pluginDataH + memOffset);
    memOffset += sizeof(parsedPluginPtr->dfIdentifierCount);
    
    if (parsedPluginPtr->dfIdentifierCount > 0) {
        parsedPluginPtr->dfIndentifiersH = (PluginDFIdentifierH**)NewHandle(sizeof(Handle) * parsedPluginPtr->dfIdentifierCount);
        
        if (parsedPluginPtr->dfIndentifiersH != NULL) {
            HLock((Handle)parsedPluginPtr->dfIndentifiersH);
            
            // Build an array of handles to data fork identifiers
            for (i = 0 ; i < parsedPluginPtr->dfIdentifierCount ; ++i) {
                pluginDFIdentifierPtr = (PluginDFIdentifierPtr)(*pluginDataH + memOffset);
                
                // structSize is used to resize the handle as .data is a variable length PString
                structSize = sizeof(PluginDFIdentifier) - sizeof(Str255) + pluginDFIdentifierPtr->data[0] + 1;
                
                err = PtrToHand(pluginDFIdentifierPtr, &parsedPluginDFIdentifierH, structSize);                        
                if (err) return ERROR(err);

                *(Handle *)parsedPluginPtr->dfIndentifiersH[i] = parsedPluginDFIdentifierH;                
                memOffset += structSize;
            }
                   
            HUnlock((Handle)parsedPluginPtr->dfIndentifiersH);
        } else {
            return ERROR(MemError());
        }
    }
    
    return ERROR(err);
}

void ExecPlugins(FSSpecPtr theSpec, ParsedPluginH **pluginArrayH, short count, PluginStrOutputPtr outputStrings) {
    register short          i,j,k;
    
    OSErr                   err = noErr;
    short                   oldResFileRef, resFileRef, fileRef;
    ParsedPluginPtr         parsedPluginPtr;
    ParsedPluginResource    parsedPluginResource;
    PluginResourcePtr       pluginResourcePtr;
    PluginDFIdentifierPtr   pluginDFIdentifierPtr;
    ResType                 theType;
    Boolean                 match = false;
    Boolean                 matchLanguage = false;
    Boolean                 matchFramework = false;
    Boolean                 matchEngine = false;
    Boolean                 matchCopyProtection = false;
    long                    dataByteCount;
    unsigned char           dataBuffer[255];
    
    oldResFileRef = CurResFile();
    resFileRef = FSpOpenResFile(theSpec, fsRdPerm);
    
    // ---- PLUGIN ARRAY ----
    for (i = 0 ; i < count ; ++i) {
        match = false;
        parsedPluginPtr = *(*pluginArrayH)[i];
        
        // ---- RESOURCE TYPES ----
        for (j = 0 ; j < parsedPluginPtr->typeCount ; ++j) {
            parsedPluginResource = (*parsedPluginPtr->resourceTypesH)[j];
            theType = (*parsedPluginResource.resourceTypeH)->theType;
            
            if (Count1Resources(theType) > 0) {
                if ((*parsedPluginResource.resourceTypeH)->resourceCount == 0) {
                    // If the plugin only specifies a resource (and not a type)
                    match = true;
                } else {
                    // ---- RESOURCES ----
                    for (k = 0 ; k < (*parsedPluginResource.resourceTypeH)->resourceCount ; ++k) {
                        pluginResourcePtr = *(*parsedPluginResource.resourcesH)[k];

                        if (Get1Resource(theType, pluginResourcePtr->theID) != NULL) {
                            if (pluginResourcePtr->name[0] > 0) {
                                if (Get1NamedResource(theType, pluginResourcePtr->name) != NULL) {
                                    // If the plugin specifies a name and the app matches
                                    match = true;
                                } else {
                                    // No matching resource name
                                    match = false;
                                    break;
                                }
                            } else {
                                // If the plugin does not specify a name
                                match = true;
                            }
                        } else {
                            // No matching resource ID
                            match = false;
                            break;
                        }
                    }
                    
                    if (!match) break;
                }
            } else {
                // No matching types
                match = false;
                break;
            }
        }
        
        // ---- DATA FORK ----
        if (parsedPluginPtr->dfIdentifierCount > 0) {
            err = FSpOpenDF(theSpec, fsRdPerm, &fileRef);
            if (err) {match = false; break;}
            
            for (j = 0 ; j < parsedPluginPtr->dfIdentifierCount ; ++j) {
                pluginDFIdentifierPtr = *(*parsedPluginPtr->dfIndentifiersH)[j];
                dataByteCount = pluginDFIdentifierPtr->data[0];            
                
                err = SetFPos(fileRef, fsFromStart, pluginDFIdentifierPtr->offset);
                if (err) {match = false; break;}
                
                err = FSRead(fileRef, &dataByteCount, dataBuffer);            
                if (err) {match = false; break;}
                
                if (dataByteCount != pluginDFIdentifierPtr->data[0]) {match = false; break;}
                
                // Use data[1] to ignore the length byte
                if (myMemCompare(dataBuffer, &pluginDFIdentifierPtr->data[1], dataByteCount)) {
                    match = true;
                } else {
                    match = false;
                    break;
                }
            }
            
            FSClose(fileRef);
        }
        
        if (match) {
            switch (parsedPluginPtr->category) {
                case kPluginCatLanguage:
                    SetPluginOutputStr(outputStrings->language, parsedPluginPtr->name, matchLanguage);
                    matchLanguage = true;
                    break;
                case kPluginCatFramework:
                    SetPluginOutputStr(outputStrings->framework, parsedPluginPtr->name, matchFramework);
                    matchFramework = true;
                    break;
                case kPluginCatEngine:
                    SetPluginOutputStr(outputStrings->engine, parsedPluginPtr->name, matchEngine);
                    matchEngine = true;
                    break;
                case kPluginCatCopyProtection:
                    SetPluginOutputStr(outputStrings->copyProtection, parsedPluginPtr->name, matchCopyProtection);                
                    matchCopyProtection = true;
                    break;
                default:
                    break;
            }
        }
    }
    
    UseResFile(oldResFileRef);
    CloseResFile(resFileRef);
}

static void SetPluginOutputStr(StringPtr field, StringPtr name, Boolean existingMatch) {
    if (existingMatch) {
         myAppendPStr(field, kPluginOutputDelimiter);
         myAppendPStr(field, name);
     } else {
         myCopyPStr(name, field);
     } 
}