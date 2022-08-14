/**************************************************************************
* Copyright (c) 2015-2022 Luxrender.                                      *
* All rights reserved.                                                    *
*                                                                         *
* DESCRIPTION: Contains the Dll Entry stuff                               *
* AUTHOR: Omid Ghotbi (TAO) omid.ghotbi@gmail.com                         *
*                                                                         *
*   This file is part of LuxRender.                                       *
*                                                                         *
* Licensed under the Apache License, Version 2.0 (the "License");         *
* you may not use this file except in compliance with the License.        *
* You may obtain a copy of the License at                                 *
*                                                                         *
*     http://www.apache.org/licenses/LICENSE-2.0                          *
*                                                                         *
* Unless required by applicable law or agreed to in writing, software     *
* distributed under the License is distributed on an "AS IS" BASIS,       *
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
* See the License for the specific language governing permissions and     *
* limitations under the License.                                          *
***************************************************************************/


#include "main.h"
#include <string>

extern ClassDesc2* GetRendDesc();
extern ClassDesc2* GetLR_NullDesc();

HINSTANCE hInstance;
HINSTANCE hInstanceAdvance;
int controlsInit = FALSE;

#define MAX_PLUGIN   2
ClassDesc *classDescArray[MAX_PLUGIN];
int classDescCount = 0;

static BOOL InitMtlDLL(void)
{
	if (!classDescCount)
	{
		classDescArray[classDescCount++] = GetRendDesc();
		classDescArray[classDescCount++] = GetLR_NullDesc();
	}

	return TRUE;
}

/** public functions **/
BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID /*lpvReserved*/) {
   if( fdwReason == DLL_PROCESS_ATTACH )
   {
      MaxSDK::Util::UseLanguagePackLocale();
      hInstance = hinstDLL;
	  hInstanceAdvance = hinstDLL;
      DisableThreadLibraryCalls(hinstDLL);
   }
	return(TRUE);
}


//------------------------------------------------------
// This is the interface to Max:
//------------------------------------------------------

// This function returns a string that describes the DLL and where the user
// could purchase the DLL if they don't have it.
__declspec(dllexport) const TCHAR* LibDescription()
{
	return GetString(IDS_LIBDESCRIPTION);
}

/// MUST CHANGE THIS NUMBER WHEN ADD NEW CLASS
// This function returns the number of plug-in classes this DLL
//TODO: Must change this number when adding a new class
__declspec(dllexport) int LibNumberClasses()
{
	InitMtlDLL();

	return classDescCount;
}

// This function returns the number of plug-in classes this DLL
__declspec(dllexport) ClassDesc* LibClassDesc(int i)
{
	InitMtlDLL();

	if (i < classDescCount)
		return classDescArray[i];
	else
		return nullptr;
}

// Return version so can detect obsolete DLLs
// This function returns a pre-defined constant indicating the version of 
// the system under which it was compiled.  It is used to allow the system
// to catch obsolete DLLs.
__declspec(dllexport) ULONG LibVersion()
{
	return VERSION_3DSMAX;
}

__declspec(dllexport) int LibInitialize(void)
{
	//#pragma message(TODO("Perform initialization here."))
	
	typedef int(*LibInitializeFunc)();
	auto RealLibInitialize =
		reinterpret_cast<LibInitializeFunc>(GetProcAddress(hInstance, "LibInitialize"));
	return RealLibInitialize ? RealLibInitialize() : InitMtlDLL();

	//return TRUE;
}

// This function is called once, just before the plugin is unloaded. 
// Perform one-time plugin un-initialization in this method."
// The system doesn't pay attention to a return value.
__declspec(dllexport) int LibShutdown(void)
{
	typedef int(*LibShutdownFunc)();
	auto RealLibShutdown =
		reinterpret_cast<LibShutdownFunc>(GetProcAddress(hInstance, "LibShutdown"));
	return RealLibShutdown ? RealLibShutdown() : TRUE;
	//#pragma message(TODO("Perform un-initialization here."))
	//return TRUE;
}

__declspec(dllexport) ULONG CanAutoDefer()
{
	return FALSE;
}

TCHAR *GetString(int id)
{
	static TCHAR buf[256];

	if(hInstance)
		return LoadString(hInstance, id, buf, _countof(buf)) ? buf : NULL;
	return NULL;
}