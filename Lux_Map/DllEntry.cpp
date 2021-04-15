/***************************************************************************
 * Copyright 2019-2020 by author Omid Ghotbi "TAO" omidt.gh@gmail.com      *
 *                                                                         *
 *   This file is part of LuxCoreRender.                                   *
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

#include "Lux_Map.h"

extern ClassDesc2* GetLux_CheckerDesc();
extern ClassDesc2* GetLux_Checker3dDesc();
extern ClassDesc2* GetLux_AddDesc();
extern ClassDesc2* GetLux_SubtractDesc();
extern ClassDesc2* GetLux_ScaleDesc();
extern ClassDesc2* GetLux_MixDesc();
extern ClassDesc2* GetLux_PowerDesc();
extern ClassDesc2* GetLux_DivideDesc();
extern ClassDesc2* GetLux_DotProductDesc();
extern ClassDesc2* GetLux_LessThanDesc();
extern ClassDesc2* GetLux_GreaterThanDesc();
extern ClassDesc2* GetLux_AbsDesc();
extern ClassDesc2* GetLux_ClampDesc();
extern ClassDesc2* GetLux_RoundDesc();
extern ClassDesc2* GetLux_ModuloDesc();
extern ClassDesc2* GetLux_NormalDesc();
extern ClassDesc2* GetLux_Volume_ClearDesc();
extern ClassDesc2* GetLux_Volume_HomoDesc();
extern ClassDesc2* GetLux_Volume_HetroDesc();

HINSTANCE hInstance;
int controlsInit = FALSE;

#define MAX_MAP   19
ClassDesc *classDescArray[MAX_MAP];
int classDescCount = 0;

static BOOL InitMtlDLL(void)
{
	if (!classDescCount)
	{
		classDescArray[classDescCount++] = GetLux_CheckerDesc();
		classDescArray[classDescCount++] = GetLux_Checker3dDesc();
		classDescArray[classDescCount++] = GetLux_AddDesc();
		classDescArray[classDescCount++] = GetLux_SubtractDesc();
		classDescArray[classDescCount++] = GetLux_ScaleDesc();
		classDescArray[classDescCount++] = GetLux_MixDesc();
		classDescArray[classDescCount++] = GetLux_PowerDesc();
		classDescArray[classDescCount++] = GetLux_DivideDesc();
		classDescArray[classDescCount++] = GetLux_DotProductDesc();
		classDescArray[classDescCount++] = GetLux_LessThanDesc();
		classDescArray[classDescCount++] = GetLux_GreaterThanDesc();
		classDescArray[classDescCount++] = GetLux_AbsDesc();
		classDescArray[classDescCount++] = GetLux_ClampDesc();
		classDescArray[classDescCount++] = GetLux_RoundDesc();
		classDescArray[classDescCount++] = GetLux_ModuloDesc();
		classDescArray[classDescCount++] = GetLux_NormalDesc();
		classDescArray[classDescCount++] = GetLux_Volume_ClearDesc();
		classDescArray[classDescCount++] = GetLux_Volume_HomoDesc();
		classDescArray[classDescCount++] = GetLux_Volume_HetroDesc();
		//RegisterObjectAppDataReader(&patchReader);
		//RegisterObjectAppDataReader(&splineReader);
	}

	return TRUE;
}

// This function is called by Windows when the DLL is loaded.  This 
// function may also be called many times during time critical operations
// like rendering.  Therefore developers need to be careful what they
// do inside this function.  In the code below, note how after the DLL is
// loaded the first time only a few statements are executed.

BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID /*lpvReserved*/)
{
	if( fdwReason == DLL_PROCESS_ATTACH )
	{
		MaxSDK::Util::UseLanguagePackLocale();
		// Hang on to this DLL's instance handle.
		hInstance = hinstDLL;
		DisableThreadLibraryCalls(hInstance);
		// DO NOT do any initialization here. Use LibInitialize() instead.
	}
	return(TRUE);
}

// This function returns a string that describes the DLL and where the user
// could purchase the DLL if they don't have it.
__declspec( dllexport ) const TCHAR* LibDescription()
{
	return GetString(IDS_LIBDESCRIPTION);
}

// This function returns the number of plug-in classes this DLL
//TODO: Must change this number when adding a new class
__declspec( dllexport ) int LibNumberClasses()
{
	InitMtlDLL();

	return classDescCount;
}

// This function returns the number of plug-in classes this DLL
__declspec( dllexport ) ClassDesc* LibClassDesc(int i)
{
	InitMtlDLL();

	if (i < classDescCount)
		return classDescArray[i];
	else
		return NULL;
	/*switch(i) {
		case 0: return GetLux_CheckerDesc();
		default: return 0;
	}*/
}

// This function returns a pre-defined constant indicating the version of 
// the system under which it was compiled.  It is used to allow the system
// to catch obsolete DLLs.
__declspec( dllexport ) ULONG LibVersion()
{
	return VERSION_3DSMAX;
}

// This function is called once, right after your plugin has been loaded by 3ds Max. 
// Perform one-time plugin initialization in this method.
// Return TRUE if you deem your plugin successfully loaded, or FALSE otherwise. If 
// the function returns FALSE, the system will NOT load the plugin, it will then call FreeLibrary
// on your DLL, and send you a message.
__declspec( dllexport ) int LibInitialize(void)
{
	//#pragma message(TODO("Perform initialization here."))

	return InitMtlDLL();
	//return TRUE;
}

// This function is called once, just before the plugin is unloaded. 
// Perform one-time plugin un-initialization in this method."
// The system doesn't pay attention to a return value.
__declspec( dllexport ) int LibShutdown(void)
{
	#pragma message(TODO("Perform un-initialization here."))
	return TRUE;
}

TCHAR *GetString(int id)
{
	static TCHAR buf[256];

	if (hInstance)
		return LoadString(hInstance, id, buf, _countof(buf)) ? buf : NULL;
	return NULL;
}

