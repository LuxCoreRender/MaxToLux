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

#pragma once
#include "LuxCamera.h"
#include <algorithm>
#include "max.h"
#include "LuxUtils.h"
#include <string>
#include <maxapi.h>
#include <iparamb2.h>
#include <Scene/IPhysicalCamera.h>
#include <luxcore.h>

#define CAMERAHELPER_CLASSID Class_ID(4128,0)
#define MAX2016_PHYSICAL_CAMERA Class_ID(1181315608,686293133)

using namespace MaxSDK;
using namespace luxcore;
using namespace luxrays;

bool MaxToLuxCamera::exportCamera(float lensRadius, luxcore::Scene &scene, TimeValue t)
{
	INode* camNode = GetCOREInterface9()->GetActiveViewExp().GetViewCamera();

	if (camNode == NULL)
	{
		float fieldOfView = 0, focaldistance = 0;
		Interval      ivalid = FOREVER;
		float mat[4][4]; //The transformation matrix times the projection matrix.
		Matrix3 affineTransformationMatrix, coordSysTM, invTM; //This is the inverse of the affine part of the camera transformation matrix
		int perspectiveMode; //Nonzero indicates this is a perspective view; 0 is orthogonal.
		float hither, yon; //Near clip value. //Far clip value. 

		ViewExp &viewPort = GetCOREInterface()->GetActiveViewExp();

		GraphicsWindow *gw = viewPort.getGW();
		fieldOfView = viewPort.GetFOV() * 180 / std::acos(-1);
		float aspectratio = GetCOREInterface11()->GetImageAspRatio();
		if (aspectratio < 1)
			fieldOfView = 2.0f * ((180 / std::acos(-1)) *(std::atan(std::tan((std::acos(-1) / 180)*(FOV / 2.0f)) / aspectratio)));

		focaldistance = viewPort.GetFocalDist();

		viewPort.GetAffineTM(affineTransformationMatrix);
		coordSysTM = Inverse(affineTransformationMatrix);
		Point3 viewDir = coordSysTM.GetRow(2);
		Point3 viewPos = coordSysTM.GetRow(3);

		Point3 viewTarget = viewPos - viewDir;


		scene.Parse(
			Property("scene.camera.lookat.orig")(viewPos.x, viewPos.y, viewPos.z) <<//(invTM.GetTrans().x, invTM.GetTrans().y, invTM.GetTrans().z) <<
			Property("scene.camera.lookat.target")(viewTarget.x, viewTarget.y, viewTarget.z) <<
			Property("scene.camera.fieldofview")(fieldOfView) <<
			Property("scene.camera.focaldistance")(focaldistance) <<
			Property("scene.camera.shutteropen")(0.0f) <<
			Property("scene.camera.shutterclose")(1.615f)
		);

		return true; // false;
	}
	else
	{
		CameraObject*   cameraPtr = (CameraObject *)camNode->EvalWorldState(t).obj;

		float v = 0.0f;
		float fieldOfView = 0;
		float focaldistance = 0;
		Interval      ivalid = FOREVER;
		IParamBlock2 *pBlock = camNode->GetParamBlock(0);
		IParamBlock2* pblock2;

		if (v > 0.0f)
		{
			MessageBox(0, L"LuxCam modifier selected.", L"Error!", MB_OK);
			return false;
		}

		if (cameraPtr->ClassID() == MAX2016_PHYSICAL_CAMERA)
		{
			IPhysicalCamera* physicalCamera = dynamic_cast<IPhysicalCamera*>(camNode->EvalWorldState(t).obj);

			fieldOfView = physicalCamera->GetfieldOfView(t, FOREVER) * 180 / std::acos(-1);
			focaldistance = physicalCamera->GetTDist(t, FOREVER);
		}
		else
		{
			fieldOfView = cameraPtr->GetfieldOfView(GetCOREInterface()->GetTime(), FOREVER) * 180 / std::acos(-1);
			focaldistance = cameraPtr->GetTDist(t, FOREVER);
		}

		::Point3 camTrans = camNode->GetNodeTM(t).GetTrans();
		INode* NewCam = camNode;
		::Matrix3 targetPos;
		NewCam->GetTargetTM(t, targetPos);

		float aspectratio = GetCOREInterface11()->GetImageAspRatio();
		if (aspectratio < 1)
			fieldOfView = 2.0f * ((180 / std::acos(-1)) *(std::atan(std::tan((std::acos(-1) / 180)*(FOV / 2.0f)) / aspectratio)));

		float x1 = targetPos.GetTrans().x;
		float x2 = targetPos.GetTrans().y;
		float x3 = targetPos.GetTrans().z;

		scene.Parse(
			Property("scene.camera.lookat.orig")(camTrans.x, camTrans.y, camTrans.z) <<
			Property("scene.camera.lookat.target")(targetPos.GetTrans().x, targetPos.GetTrans().y, targetPos.GetTrans().z) <<
			Property("scene.camera.fieldofview")(fieldOfView) <<
			Property("scene.camera.lensradius")(lensRadius) <<
			Property("scene.camera.focaldistance")(focaldistance) <<
			Property("scene.camera.shutteropen")(0.0f) <<
			Property("scene.camera.shutterclose")(1.615f)
			);
		return true;
	}

}

void MaxToLuxCamera::setMaterialCamera(luxcore::Scene * scene, float camDistance)
{
	scene->Parse(
		Property("scene.camera.lookat.orig")(camDistance, camDistance, camDistance) <<
		Property("scene.camera.lookat.target")(0.0f, 0.0f, 0.0f) <<
		Property("scene.camera.fieldofview")(35.0f)
	);
}
