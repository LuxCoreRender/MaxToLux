/**************************************************************************
* Copyright (c) 2015-2024 Luxrender.                                      *
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


#define CAMERA_TARGER_CLASSID	Class_ID(4128,0)
#define MAX2016_PHYSICAL_CAMERA Class_ID(1181315608, 686293133)
#define ENV_FOG_CLASS_ID		Class_ID(268435457, 0) 

#include <notify.h>
#include <limits>
#include "main.h"
#include "MaxToLux.h"
#include "maxscript/maxscript.h"

#include "LuxCamera.h"
#include "LuxUtils.h"
#include "LuxMaterials.h"
#include "LuxLights.h"
#include "LuxMesh.h"
#include "Classes.h"

#include <vector>
#include <thread>

#include <render.h>
#include <MeshNormalSpec.h>
#include <Path.h>
#include <bitmap.h>
#include <GraphicsWindow.h>
#include <IColorCorrectionMgr.h>
#include <IGame\IGame.h>
#include <VertexNormal.h>
#include <iostream>
#include <IMaterialBrowserEntryInfo.h>
#include <units.h>
#include <mutex>

#include <interactiverender.h>
#include <mesh.h>

#include <sstream>
#include <iInstanceMgr.h>
#include <unordered_map>

MaxToLuxCamera lxmCamera;
MaxToLuxLights lxmLights;
MaxToLuxMaterials lxmMaterials;
MaxToLuxUtils lxmUtils;
MaxToLuxMesh lxmMesh;

#pragma warning (push)
#pragma warning( disable:4002)
#pragma warning (pop)

namespace luxcore
{
	#include <luxcore.h>
}

using namespace std;
using namespace luxcore;
using namespace luxrays;


extern BOOL FileExists(const TCHAR *filename);
float* pixels;
bool defaultlightset = true;
bool defaultlightchk = true;
bool defaultlightauto = true;
bool deviceArray1 = true, deviceArray2, deviceArray3, deviceArray4, deviceArray5 = false;
int rendertype = 4;
int renderWidth = 0;
int renderHeight = 0;
bool renderingMaterialPreview = false;
int vfbRefreshRateInt = 1;
luxcore::Scene *materialPreviewScene;