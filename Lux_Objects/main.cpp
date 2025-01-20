//**************************************************************************/
// Copyright (c) 2015-2019 Luxrender.
// All rights reserved.
// 
//**************************************************************************/
// DESCRIPTION: Contains the Dll Entry stuff
// AUTHOR: Omid Ghotbi (TAO) omid.ghotbi@gmail.com www.3dfine.com
//***************************************************************************/

#include "main.h"
#include "buildver.h"

HINSTANCE hInstance;
int controlsInit = FALSE;

#define MAX_PRIM_OBJECTS   8
ClassDesc *classDescArray[MAX_PRIM_OBJECTS];
int classDescCount = 0;

static BOOL InitObjectsDLL(void)
{
   if( !classDescCount )
   {
      classDescArray[classDescCount++] = GetLookatCamDesc();
      classDescArray[classDescCount++] = GetSimpleCamDesc();
      classDescArray[classDescCount++] = GetTargetObjDesc();
      classDescArray[classDescCount++] = GetTSpotLightDesc();
      classDescArray[classDescCount++] = GetFSpotLightDesc();
      classDescArray[classDescCount++] = GetTDirLightDesc();
      classDescArray[classDescCount++] = GetDirLightDesc();
      classDescArray[classDescCount++] = GetOmniLightDesc();
      //RegisterObjectAppDataReader(&patchReader);
      //RegisterObjectAppDataReader(&splineReader);
   }

   return TRUE;
}

/** public functions **/
BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID lpvReserved) {
   if( fdwReason == DLL_PROCESS_ATTACH )
   {
      MaxSDK::Util::UseLanguagePackLocale();

      hInstance = hinstDLL;
      DisableThreadLibraryCalls(hInstance);
   }
   return(TRUE);
   }


//------------------------------------------------------
// This is the interface to Jaguar:
//------------------------------------------------------

__declspec( dllexport ) const TCHAR *
LibDescription() { return GetString(IDS_LIBDESCRIPTION); }

/// MUST CHANGE THIS NUMBER WHEN ADD NEW CLASS
__declspec( dllexport ) int LibNumberClasses() 
{
   InitObjectsDLL();

   return classDescCount;
}

// russom - 05/07/01 - changed to use classDescArray
__declspec( dllexport ) ClassDesc*
LibClassDesc(int i) 
{
   InitObjectsDLL();

   if( i < classDescCount )
      return classDescArray[i];
   else
      return NULL;

}


// Return version so can detect obsolete DLLs
__declspec( dllexport ) ULONG 
LibVersion() { return VERSION_3DSMAX; }

__declspec( dllexport ) int
LibInitialize()
{
   return InitObjectsDLL();
}

TCHAR *GetString(int id)
   {
   static TCHAR buf[256];

   if (hInstance)
      return LoadString(hInstance, id, buf, _countof(buf)) ? buf : NULL;
   return NULL;
   }
