//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include <systemutilities.h>

#include <Windows.h>

HINSTANCE hInstance;
BOOL WINAPI
DllMain(HINSTANCE hinstDLL, ULONG fdwReason, LPVOID )
{
	static BOOL ownApplication = FALSE;
	if( fdwReason == DLL_PROCESS_ATTACH )	
    {
		MaxSDK::Util::UseLanguagePackLocale();
		hInstance = hinstDLL;
		DisableThreadLibraryCalls(hInstance);
	} else if( fdwReason == DLL_PROCESS_DETACH )	
    {
	}

	return(TRUE);
}

