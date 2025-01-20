//**************************************************************************/
// Copyright (c) 2015-2019 Luxrender.
// All rights reserved.
// 
//**************************************************************************/
// DESCRIPTION: Contains the Dll Entry stuff
// AUTHOR: Omid Ghotbi (TAO) omid.ghotbi@gmail.com www.3dfine.com
//***************************************************************************/

#ifndef __PRIM__H
#define __PRIM__H

#include "Max.h"
#include "resource.h"
#include "resourceOverride.h"

TCHAR *GetString(int id);

extern ClassDesc* GetSimpleCamDesc();
extern ClassDesc* GetOmniLightDesc();
extern ClassDesc* GetDirLightDesc();
extern ClassDesc *GetTDirLightDesc();
extern ClassDesc* GetFSpotLightDesc();
extern ClassDesc* GetTSpotLightDesc();
extern ClassDesc* GetLookatCamDesc();
extern ClassDesc* GetTargetObjDesc();
extern ClassDesc* GetProtHelpDesc();
extern HINSTANCE hInstance;

//extern void MakeMeshCapTexture(Mesh &mesh, Matrix3 &itm, int fstart, int fend, BOOL usePhysUVs);
//extern void MakeMeshCapTexture(Mesh &mesh, Matrix3 &itm, BitArray& capFaces, BOOL usePhysUVs);

#endif
