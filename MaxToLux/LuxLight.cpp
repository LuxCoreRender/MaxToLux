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

float RGBc(float color) {
	return (color / 255) * 100;
}

std::string MaxToLuxLights::exportOmni(INode* Omni)
{

	::Point3 trans = Omni->GetNodeTM(GetCOREInterface11()->GetTime()).GetTrans();
	::Point3 color;

	luxrays::Properties props;
	std::string objString;

	renderOptions::lightStatics Options;

	ObjectState ostate = Omni->EvalWorldState(GetCOREInterface()->GetTime());
	LightObject *light = (LightObject*)ostate.obj;
	color = light->GetRGBColor(GetCOREInterface()->GetTime(), FOREVER);
	float intensityval = light->GetIntensity(GetCOREInterface()->GetTime(), FOREVER);
	
	objString.append(Options.sceneLight);
	objString.append(lxmUtil.removeUnwatedChars(lmutil.ToNarrow(Omni->GetName())));
	objString.append(Options.lightType + Options.lightTypes[Options.Point]);
	objString.append("\n");

	objString.append(Options.sceneLight);
	objString.append(lxmUtil.removeUnwatedChars(lmutil.ToNarrow(Omni->GetName())));
	objString.append(Options.lightPosition);
	objString.append(std::to_string(trans.x) + " " + std::to_string(trans.y) + " " + std::to_string(trans.z));
	objString.append("\n");


	return objString;
}

std::string MaxToLuxLights::exportSkyLight(INode* SkyLight)
{
	luxrays::Properties props;
	std::string objString;
	renderOptions::lightStatics Options;
	::Point3 trans = SkyLight->GetNodeTM(GetCOREInterface11()->GetTime()).GetTrans();
	::Point3 color;

	ObjectState os = SkyLight->EvalWorldState(GetCOREInterface()->GetTime());
	LightObject *light = (LightObject*)os.obj;

	color = light->GetRGBColor(GetCOREInterface()->GetTime(), FOREVER);
	float ColorIntensValue = 0.0f;
	ColorIntensValue = light->GetIntensity(GetCOREInterface()->GetTime(), FOREVER);

    return objString;
}

void MaxToLuxLights::setDefaultSkyLight(luxcore::Scene &scene)
{
	renderOptions::lightStatics Options;
	scene.Parse(
		luxrays::Property(Options.sceneLight + "sunl.type")(Options.lightTypes[Options.Sun]) <<
		luxrays::Property(Options.sceneLight + "sunl.dir")(0.166974f, -0.59908f, 0.783085f) <<
		luxrays::Property(Options.sceneLight + "sunl.turbidity")(2.2f) <<
		luxrays::Property(Options.sceneLight + "sunl.relsize")(2.0f) <<
		luxrays::Property(Options.sceneLight + "sunl.gain")(1.0f, 1.0f, 1.0f)
	);

}