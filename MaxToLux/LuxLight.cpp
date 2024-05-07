//**************************************************************************/
// Copyright (c) 2015-2024 Luxrender.
// All rights reserved.
// 
//**************************************************************************/
// DESCRIPTION: Contains the Dll Entry stuff
// AUTHOR: Omid Ghotbi (TAO) omid.ghotbi@gmail.com www.3dfine.com
//***************************************************************************/


#include <algorithm>
#include <string>
#include "max.h"
#include "LuxLights.h"
#include <maxscript\maxscript.h>
#include "LuxMaterials.h"
#include "LuxMaps.h"
#include "LuxUtils.h"
#include "Classes.h"
#include <luxcore.h>

#define OMNI_CLASSID				Class_ID(4113, 0)
#define SPOTLIGHT_CLASSID			Class_ID(4114,0)
#define DIRLIGHT_CLASSID			Class_ID(4115, 0)
#define SKYLIGHT_CLASSID			Class_ID(2079724664, 1378764549)