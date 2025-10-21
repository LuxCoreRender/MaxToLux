//**************************************************************************/
// Copyright (c) 2015-2019 Luxrender.
// All rights reserved.
// 
//**************************************************************************/
// DESCRIPTION: Contains the Dll Entry stuff
// AUTHOR: Omid Ghotbi (TAO) omid.ghotbi@gmail.com www.3dfine.com
//***************************************************************************/


#include <algorithm>
#include <string>
//using std::max;
//using std::min;

#include "LuxMaterials.h"
#include "LuxUtils.h"
#include <bitmap.h>
#include <pbbitmap.h>
//#include <map>
#include <IGame\IGameMaterial.h>
#include <stdio.h>
//#include <string>
#include <maxapi.h>
//#include "imtl.h"
//#include "imaterial.h"
#include <imaterial.h>
#include <iparamb2.h>
//#include <iparamb.h>
#include "path.h"
#include <bitmap.h>
#include "AssetManagement/iassetmanager.h"
#include "IFileResolutionManager.h"
//#include "AssetType.h"
#include "IFileResolutionManager.h"
//#include <boost/filesystem/operations.hpp>
//#include <boost/foreach.hpp>
//#include <boost/assign.hpp>
//#include <boost/format.hpp>
//#include <maxscript\maxscript.h>
#include "Classes.h"

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

std::string MaxToLuxMaterials::getFloatFromParamBlockID(int paramID, ::Mtl* mat, ::Texmap* tex)
{
	std::string stringValue = "";
	float value = 0.0f;
	IParamBlock2 *pBlock = NULL;

	if (mat != NULL)
	{
		pBlock = mat->GetParamBlock(0);
	}
	else
	{
		pBlock = tex->GetParamBlock(0);
	}

	if (pBlock != NULL)
	{
		value = pBlock->GetFloat(paramID, GetCOREInterface()->GetTime());
		stringValue = lmutil->floatToString(value);
	}
	return stringValue;
}

std::string MaxToLuxMaterials::exportTexturesInMaterial(::Mtl* mat, std::string texSlotName)
{
	std::string stringValue = "";

	renderOptions::materialStatics Options;
	Texmap *tex;
	std::string path = "";

	const wchar_t *matName = L"";

	matName = mat->GetName();
	std::string tmpMatName = lmutil->ToNarrow(matName);
	if (tmpMatName == "")
	{
		tmpMatName = "undefinedMaterial";
		matName = L"undefinedMaterial";
	}

	lmutil->removeUnwatedChars(tmpMatName);
	std::wstring replacedMaterialName = std::wstring(tmpMatName.begin(), tmpMatName.end());
	matName = replacedMaterialName.c_str();


	IParamBlock2 *pBlock = mat->GetParamBlock(0);
	for (int i = 0, count = mat->NumParamBlocks(); i < count; ++i)
	{
		IParamBlock2 *pBlock = mat->GetParamBlock(i);
		//for (int currParam = 0, currParam = pBlock->NumParams(); currParam < currParam; ++currParam)
		for (int a = 0; a < pBlock->NumParams(); a++) 
		{
			{
				tex = pBlock->GetTexmap(a, GetCOREInterface()->GetTime());

				if (tex != NULL)
				{
					if (tex->ClassID() == STANDARDBITMAP_CLASSID)
					{
						BitmapTex *bmt = (BitmapTex*)tex;

						if (bmt != NULL)
						{
							//Still need Non-Unicode string fix.
							std::string tmpTexName = tex->GetName().ToCStr();
							lmutil->removeUnwatedChars(tmpTexName);
							//path = bmt->GetMap().GetFullFilePath().ToUTF8();

							stringValue.append(Options.sceneTexture + tmpTexName + ".type = imagemap");
							stringValue.append("\n");
							stringValue.append(Options.sceneTexture + tmpTexName + Options.textureFile + "\"" + getMaterialDiffuseTexturePath(mat) + "\"");
							stringValue.append("\n");
							stringValue.append(Options.sceneMaterial + lmutil->ToNarrow(matName) + Options.materialKd + tmpTexName);
							stringValue.append("\n");
						}
					}
					else if (tex->ClassID() == LR_Texture_CLASS_ID)
					{
						std::string tmpTexName = tex->GetName().ToCStr();
						lmutil->removeUnwatedChars(tmpTexName);
						
						stringValue.append(Options.sceneTexture + tmpTexName + ".type = blender_noise");
						stringValue.append("\n");
						stringValue.append(Options.sceneTexture + tmpTexName + ".noisedepth =" + getIntFromParamBlockID(2, NULL, tex));
						stringValue.append("\n");
						stringValue.append(Options.sceneTexture + tmpTexName + ".bright = " + getFloatFromParamBlockID(3, NULL, tex));
						stringValue.append("\n");
						stringValue.append(Options.sceneTexture + tmpTexName + ".contrast = " + getFloatFromParamBlockID(4, NULL, tex));
						stringValue.append("\n");

						//Currently color diffuse is supported, we need a way to set *any* texture type.
						//We should set the texture name outside from here..
						if (texSlotName != "")
						{
							stringValue.append(Options.sceneMaterial + lmutil->ToNarrow(matName) + "." + texSlotName + "= " + tmpTexName);
						}
						
						stringValue.append("\n");
					}
					else
					{
						OutputDebugStringW(L"\nUnsupported texture map in material: ");//mprintf(L"ERROR : Unsupported texture in material: '%s' , named: '%s' , will not render texture. standard bitmap is supported.\n", mat->GetName(), tex->GetName());
						OutputDebugStringW(mat->GetName());
					}
				}
			}
		}
	}
	//tex = pBlock->GetTexmap(0, GetCOREInterface()->GetTime(), 0);
	
	
	//}
	return stringValue;
}

std::string MaxToLuxMaterials::getLightEmission(::Mtl* mat)
{
	std::string stringValue = "";
	bool enableEmission = false;

	for (int i = 0, count = mat->NumParamBlocks(); i < count; ++i)
	{
		IParamBlock2 *pBlock = mat->GetParamBlockByID(i);
		Interval      ivalid;
		for (int j = 0, count = pBlock->NumParams(); j < count; ++j)
		{
			std::string tmp = pBlock->GetLocalName(j).ToCStr();
			OutputDebugStringW(L"Checking param for 'enableemission: ");
			OutputDebugStringW(pBlock->GetLocalName(j));
			OutputDebugStringW(L"\n");
			if (::_stricmp(pBlock->GetLocalName(j).ToCStr(), "enableemission") == 0)
			{
				enableEmission = pBlock->GetInt(j, GetCOREInterface()->GetTime());
			}
		}
	}

	for (int i = 0, count = mat->NumParamBlocks(); i < count; ++i)
	{
		IParamBlock2 *pBlock = mat->GetParamBlockByID(i);
		Interval      ivalid;
		renderOptions::materialStatics Options;
	

		if (enableEmission)
		{ 
			
			const wchar_t *matName = L"";
			matName = mat->GetName();
			std::string tmpMatName = lmutil->ToNarrow(matName);

			if (tmpMatName == "")
			{
				tmpMatName = "undefinedMaterial";
				matName = L"undefinedMaterial";
			}

			lmutil->removeUnwatedChars(tmpMatName);
			std::wstring replacedMaterialName = std::wstring(tmpMatName.begin(), tmpMatName.end());
			matName = replacedMaterialName.c_str();

			for (int j = 0, count = pBlock->NumParams(); j < count; ++j)
			{
				OutputDebugStringW(L"\nMatte params\n");
				OutputDebugStringW(pBlock->GetLocalName(j));

				if (::_stricmp(pBlock->GetLocalName(j).ToCStr(), "emission") == 0)
				{
					Point3 color;
				
					pBlock->GetValue(j, GetCOREInterface()->GetTime(), color, ivalid);
					stringValue.append(Options.sceneMaterial + lmutil->ToNarrow(matName) + ".emission = " + to_string(color.x) + " " + to_string(color.y)  + " " + to_string(color.z));
					stringValue.append("\n");

				}

				if (::_stricmp(pBlock->GetLocalName(j).ToCStr(), "emission_power") == 0)
				{
					float spectrum;
				
					pBlock->GetValue(j, GetCOREInterface()->GetTime(), spectrum, ivalid);
					stringValue.append(Options.sceneMaterial + lmutil->ToNarrow(matName) + ".emission.power = " + to_string(spectrum));
					stringValue.append("\n");
				}

				if (::_stricmp(pBlock->GetLocalName(j).ToCStr(), "emission_efficency") == 0)
				{
					float efficency;

					pBlock->GetValue(j, GetCOREInterface()->GetTime(), efficency, ivalid);
					stringValue.append(Options.sceneMaterial + lmutil->ToNarrow(matName) + ".emission.efficency = " + to_string(efficency));
					stringValue.append("\n");
				}


				if (::_stricmp(pBlock->GetLocalName(j).ToCStr(), "emission_map") == 0)
				{
					if (getTexturePathFromParamBlockID(j, mat) != "")
					{
						std::string emission_mapfile_name = getTextureName(j, mat);
						stringValue.append(Options.sceneTexture + emission_mapfile_name + ".type = imagemap");
						stringValue.append("\n");
						stringValue.append(Options.sceneTexture + emission_mapfile_name + Options.textureFile + "\"" + getTexturePathFromParamBlockID(j, mat) + "\"");
						stringValue.append("\n");
						stringValue.append(Options.sceneMaterial + lmutil->ToNarrow(matName) + "emission.mapfile = " + emission_mapfile_name);
					}
				}
			
				if (::_stricmp(pBlock->GetLocalName(j).ToCStr(), "emission_gamma") == 0)
				{
					float gamma;

					pBlock->GetValue(j, GetCOREInterface()->GetTime(), gamma, ivalid);
					stringValue.append(Options.sceneMaterial + lmutil->ToNarrow(matName) + ".emission.gamma = " + to_string(gamma));
					stringValue.append("\n");
				}

				if (::_stricmp(pBlock->GetLocalName(j).ToCStr(), "emission_samples") == 0)
				{
					int samples;

					pBlock->GetValue(j, GetCOREInterface()->GetTime(), samples, ivalid);
					stringValue.append(Options.sceneMaterial + lmutil->ToNarrow(matName) + ".emission.samples = " + to_string(samples));
					stringValue.append("\n");
				}

				//emission_map_width
				if (::_stricmp(pBlock->GetLocalName(j).ToCStr(), "emission_map_width") == 0)
				{
					int map_width;

					pBlock->GetValue(j, GetCOREInterface()->GetTime(), map_width, ivalid);
					stringValue.append(Options.sceneMaterial + lmutil->ToNarrow(matName) + "emission.map.width = " + to_string(map_width));
					stringValue.append("\n");
				}

				if (::_stricmp(pBlock->GetLocalName(j).ToCStr(), "emission_map_height") == 0)
				{
					int map_height;

					pBlock->GetValue(j, GetCOREInterface()->GetTime(), map_height, ivalid);
					stringValue.append(Options.sceneMaterial + lmutil->ToNarrow(matName) + "emission.map.height = " + to_string(map_height));
					stringValue.append("\n");
				}

				if (::_stricmp(pBlock->GetLocalName(j).ToCStr(), "emission_id") == 0)
				{
					int emision_id;

					pBlock->GetValue(j, GetCOREInterface()->GetTime(), emision_id, ivalid);
					stringValue.append(Options.sceneMaterial + lmutil->ToNarrow(matName) + ".emission.id = " + to_string(emision_id));
					stringValue.append("\n");
				}

				//TODO: create a bool function that checks if a texture is defined by param 'name'..
				//use that to check, set texture or color based on that.

			}
		}
	}


	return stringValue;
}

std::string MaxToLuxMaterials::getIntFromParamBlockID(int paramID, ::Mtl* mat, ::Texmap* tex)
{

	std::string stringValue = "";
	//float value = 0.0f;
	int value = 0;
	IParamBlock2 *pBlock = NULL;

	if (mat != NULL)
	{
		pBlock = mat->GetParamBlock(0);
	}
	else
	{
		pBlock = tex->GetParamBlock(0);
	}

	if (pBlock != NULL)
	{
		value = pBlock->GetInt(paramID, GetCOREInterface()->GetTime());
		stringValue = std::to_string(value);
	}
	return stringValue;
}

std::string MaxToLuxMaterials::getTexturePathFromParamBlockID(int paramID, ::Mtl* mat)
{
	Texmap *tex;
	std::string path = "";

	IParamBlock2 *pBlock = mat->GetParamBlock(0);
	tex = pBlock->GetTexmap(paramID, GetCOREInterface()->GetTime(), 0);
	
	if (tex != NULL)
	{
		if (tex->ClassID() == STANDARDBITMAP_CLASSID)
		{
			BitmapTex *bmt = (BitmapTex*)tex;

			if (bmt != NULL)
			{
				//Non-Unicode string, we should fix this so that it does not crash with Chinese characters for example.
				// http://www.luxrender.net/mantis/view.php?id=1624#bugnotes

				path = bmt->GetMap().GetFullFilePath().ToUTF8();
			}
		}
		else if (tex->ClassID() == LUX_CHECKER_CLASS_ID)
		{
			path = "checker";
		}
		else
		{
			OutputDebugStringW(L"\nUnsupported texture map in material: ");//mprintf(L"ERROR : Unsupported texture in material: '%s' , named: '%s' , will not render texture. standard bitmap is supported.\n", mat->GetName(), tex->GetName());
			OutputDebugStringW(mat->GetName());
		}
	}
	return path;
}
