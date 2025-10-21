//**************************************************************************/
// Copyright (c) 2015-2019 Luxrender.
// All rights reserved.
// 
//**************************************************************************/
// DESCRIPTION: Contains the Dll Entry stuff
// AUTHOR: Omid Ghotbi (TAO) omid.ghotbi@gmail.com www.3dfine.com
//***************************************************************************/


//Import luxcore into separate namespace to avoid conflict with max SDK includes.
namespace luxcore
{
#include <luxcore.h>
}

using namespace std;
using namespace luxcore;
using namespace luxrays;

#define STANDARDMATERIAL_CLASSID		Class_ID(2,0)
#define ARCHITECTURAL_CLASSID			Class_ID(332471230,1763586103)
#define ARCHDESIGN_CLASSID				Class_ID(1890604853, 1242969684)
#define LR_INTERNAL_MATTE_CLASSID		Class_ID(0x31b54e60, 0x1de956e4)
#define LR_INTERNAL_MATTELIGHT_CLASSID	Class_ID(0x5d2f7ac1, 0x7dd93354)
#define LR_INTERNAL_MAT_TEMPLATE_CLASSID Class_ID(0x64691d17, 0x288d50d9)
#define LR_MATTE_TRANSLUCENT_CLASSID	Class_ID(0x31b26e70, 0x2de454e4)
#define LR_ROUGH_MATTE_CLASSID			Class_ID(0x34b56e70, 0x7de894e5)
#define LR_GLOSSY2_CLASSID				Class_ID(0x67b86e70, 0x7de456e1)
#define LR_ROUGHGLASS_CLASSID			Class_ID(0x56b76e70, 0x8de321e5)
#define LR_VELVET_CLASSID				Class_ID(0x59b79e53, 0x2de567e3)
#define LR_GLOSSYTRANSLUCENT_CLASSID	Class_ID(0x24b19e11, 0x1de467e3)
#define LR_Archglass_CLASS_ID			Class_ID(0x34b16e78, 0x3de467e3)
#define LR_Cloth_CLASS_ID				Class_ID(0x45b18e28, 0x2de456e3)
#define LR_Carpaint_CLASS_ID			Class_ID(0x12b48e28, 0x5de432e3)
#define LR_Texture_CLASS_ID				Class_ID(0x482c6db2, 0x9cb39d3d)
#define LR_Cloud_CLASS_ID				Class_ID(0x7e29b710, 0x5a07c68d)

// Map Class Ids
#define STANDARDBITMAP_CLASSID			Class_ID(576, 0)
#define LUX_CHECKER_CLASS_ID			Class_ID(0x7d03463, 0xd5454f5)
#define LUXCORE_CHEKER_CLASSID			Class_ID(0x34e85fea, 0x855292c)

MaxToLuxUtils * lmutil;

/*MaxToLuxMaterials::MaxToLuxMaterials()
{
}*/

/*MaxToLuxMaterials::~MaxToLuxMaterials()
{
}*/