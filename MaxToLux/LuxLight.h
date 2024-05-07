/***************************************************************************
//**************************************************************************/
// Copyright (c) 2015-2024 Luxrender.
// All rights reserved.
// 
//**************************************************************************/
// DESCRIPTION: Contains the Dll Entry stuff
// AUTHOR: Omid Ghotbi (TAO) omid.ghotbi@gmail.com www.3dfine.com
//***************************************************************************/

#pragma once
#include <algorithm>
#include <luxcore.h>
#include <iparamb2.h>

class MaxToLuxLights
{
public:
	//MaxToLuxLights();
	//~MaxToLuxLights();

	std::string exportOmni(INode* Light);
	std::string exportSkyLight(INode* Light);
	std::string exportDiright(INode* Light);
	std::string exportSpotLight(INode* Light);
	std::string exportLuxPointLight(INode* Light);
	std::string exportLuxSphereLight(INode * Light);
	std::string exportLuxLaserLight(INode* Light);
	std::string exportLuxSpotLight(INode * Light);
	std::string exportLuxSkyLight(INode * Light);
	std::string exportLuxSunLight(INode * Light);
	void setDefaultSkyLight(luxcore::Scene &scene);
	void setInfinitLightPreview(luxcore::Scene &scene);
	bool setInfinitLight(luxcore::Scene & scene, Texmap * envMap);
	void MaxToLuxLights::setLight(INode * currNode, luxcore::Scene &scene, TimeValue t);
	int MaxToLuxLights::isSupportedLight(Object * obj);
};