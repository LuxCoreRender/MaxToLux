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


#include <luxcore.h>
#include <utils\utils.h>
#include <utils\properties.h>
#include <utils\exportdefs.h>
#include <algorithm>
#include "LuxMesh.h"
#include "max.h"
#include <string>
#include <maxapi.h>
#include <imaterial.h>
#include <iparamb2.h>
#include <maxscript\maxscript.h>
#include "MeshDLib.h"

#include "LuxMaterials.h"
#include "LuxUtils.h"
#include "Classes.h"
#include <MeshNormalSpec.h>
#include "Stack.h"
#include <unordered_map>

#define Lux_MIX_CLASS_ID		Class_ID(0x26894a44, 0x8ce7173)

using namespace std;
using namespace luxcore;
using namespace luxrays;

MaxToLuxMaterials *lxmMaterials;
MaxToLuxUtils *lxmUtils;


void GetFaceRNormals(::Mesh& mesh, int fi, Point3* normals)
{
	Face& face = mesh.faces[fi];
	DWORD fsmg = face.getSmGroup();
	if (fsmg == 0)
	{
		normals[0] = normals[1] = normals[2] = mesh.getFaceNormal(fi);
		return;
	}
	MtlID fmtl = face.getMatID();
	DWORD* fverts = face.getAllVerts();
	for (int v = 0; v < 3; ++v)
	{
		RVertex& rvert = mesh.getRVert(fverts[v]);
		int numNormals = (int)(rvert.rFlags & NORCT_MASK);

		if (numNormals == 1)
			normals[v] = rvert.rn.getNormal();
		else
		{
			for (int n = 0; n < numNormals; ++n)
			{
				RNormal& rn = rvert.ern[n];
				if ((fsmg & rn.getSmGroup()) && fmtl == rn.getMtlIndex())
				{
					normals[v] = rn.getNormal();
					break;
				}
			}
		}
	}
}

using namespace MaxToLux;

void MaxToLuxMesh::createMesh(INode * currNode, luxcore::Scene &scene , TimeValue t, bool prev)
{
	if (currNode->IsHidden(0, true))
		return;

	Object*	obj;
	ObjectState os = currNode->EvalWorldState(t);
	obj = os.obj;
	Matrix3 nodeInitTM;
	Point4 nodeRotation;
	TriObject *p_triobj = NULL;

	BOOL fConvertedToTriObject = obj->CanConvertToType(triObjectClassID) && (p_triobj = (TriObject*)obj->ConvertToType(0, triObjectClassID)) != NULL;

	if (!fConvertedToTriObject || p_triobj->mesh.getNumFaces() < 1)
	{
		DebugPrint(_M("\nNot triangulate object : %x"), currNode->GetName());
		return;//exit;
	}

	//mprintf(L"Info: Creating mesh for object : %s\n", currNode->GetName());
	const wchar_t *objName = L"";
	std::string tmpName = lxmUtils->ToNarrow(currNode->GetName());
	// adding additional id for object with the same name
	std::string nodeHandle = std::to_string(currNode->GetHandle());
	tmpName.append(nodeHandle);
	lxmUtils->removeUnwatedChars(tmpName);
	std::wstring replacedObjName = std::wstring(tmpName.begin(), tmpName.end());
	objName = replacedObjName.c_str();

	//OutputDebugStringW(L"\nMaxToLuxMesh -> Exporting Object: ");
	//OutputDebugStringW(objName);
	DebugPrint(_M("\nMaxToLuxMesh -> Exporting Object: %x"), objName);

	::Mesh *p_trimesh = &p_triobj->mesh;
	int numfaces = p_trimesh->getNumFaces();
	unsigned short maxID = 0;
	Mesh dummyMeshCopy;
	MeshNormalSpec* normals = nullptr;

	p_trimesh->buildNormals();

	// Otherwise we will try to get the normals without modifying the mesh. We try it...
	normals = p_trimesh->GetSpecifiedNormals();
	if (normals == NULL || normals->GetNumNormals() == 0) {
		// ... and if it fails, we will have to copy the mesh, modify it by calling SpecifyNormals(), and get the normals 
		// from the mesh copy
		dummyMeshCopy = *p_trimesh;
		dummyMeshCopy.SpecifyNormals();
		normals = dummyMeshCopy.GetSpecifiedNormals();
	}
	normals->CheckNormals();
		
	//p_trimesh->checkNormals(true);
	//p_trimesh->buildNormals();

	int numUvs = p_trimesh->getNumTVerts();
	const bool hasUV = p_trimesh->tVerts != NULL;
	int vertCount = numfaces * 3;//p_trimesh->numFaces * 3;

	unsigned int* indices = new unsigned int[vertCount];

	unsigned int numTriangles = numfaces;//p_trimesh->getNumFaces();

	vector<Point> tmpMeshVerts;
	//vector<Normal> tmpMeshNorms;
	vector<Triangle> tmpMeshTris;
	vector<int> indTest;
	vector<Point> tmpNormal;
	vector<UV> tmpUvMaps;
	vector<UV> tmpUvMaps2;
	vector<UV> tmpUvMaps3;

	//Normal *n = new Normal[vertCount];
	float *n = new float[vertCount * 3];
	//int normalCount = rawverts->n.siz * 3;
	//float *n = new float[normalCount];

	UV *uv = new UV[vertCount];
	int ncounter = 0;

	//Mesh dummyMeshCopy;

	int numSubmtls = 0;
	Mtl *objmatNode = NULL;
	objmatNode = currNode->GetMtl();
	if (objmatNode)
		numSubmtls = objmatNode->NumSubMtls();

	// Swap comment to enable SECONDARY UV CHANNEL
	//int numChannels = p_trimesh->tVerts == NULL ? 0 : 1;

	const std::string nodeName = lxmUtils->ToNarrow(objName);
	std::string nodeSubName;

	Properties props;
	std::string objString;

	u_int alreadyExistDic = 0;
	std::unordered_map<u_int, u_int> alreadyExist;

	if ((numSubmtls > 1) && (!prev))
	{
		for (u_int matIndex = 0; matIndex < numSubmtls; matIndex++)
		{
			int count = 0;
			TVFace* tvfaces = p_trimesh->tvFace;
			Point3* uvverts = p_trimesh->tVerts;

			for (u_int faceIndex = 0; faceIndex < p_trimesh->getNumFaces(); ++faceIndex)
			{
				Face& face = p_trimesh->faces[faceIndex];

				auto search = alreadyExist.find(faceIndex);
				if (search != alreadyExist.end())
					continue;

				DebugPrint(_M("\nFace Index : %d"), faceIndex);
				//const int mtlId = face.getMatID() % numSubmtls;
				const u_int mtlId = face.getMatID();// % numSubmtls;
				u_int vertIndices[3];

				if (mtlId != matIndex)
					continue;

				for (int vertIndex = 0; vertIndex < 3; ++vertIndex) {
					const int vIndex = vertIndex;

					const u_int vertsIndex = count++;
					vertIndices[vIndex] = vertsIndex;
					const u_int v = face.getVert(vIndex);
					indTest.push_back(v);
					//vertIndices[vIndex] = v;
					//p[count] = Point(rawverts[v].p);
					tmpMeshVerts.push_back(Point(p_trimesh->getVert(v)));

					u_int normalIndex = normals->GetNormalIndex(faceIndex, vIndex);
					Point tempNormal = normals->Normal(normalIndex);// .Normalize();
					tmpNormal.push_back(tempNormal);

					//const int verColChanel = 0;
					//MeshMap& mapChannel = p_trimesh->maps[verColChanel + 1];

					if (hasUV)
					{
						Point3 pt = uvverts[tvfaces[faceIndex].t[vIndex]];
						tmpUvMaps.push_back(UV(pt.x, pt.y * -1));
					}
				}
				tmpMeshTris.push_back(Triangle(vertIndices[0], vertIndices[1], vertIndices[2]));

				alreadyExistDic++;
				alreadyExist[faceIndex] = alreadyExistDic;

			}

			// Allocate memory for LuxCore mesh data
			Triangle *meshTris = (Triangle *)Scene::AllocTrianglesBuffer(tmpMeshTris.size());
			copy(tmpMeshTris.begin(), tmpMeshTris.end(), meshTris);

			Point *meshVerts = (Point *)Scene::AllocVerticesBuffer(tmpMeshVerts.size());
			copy(tmpMeshVerts.begin(), tmpMeshVerts.end(), meshVerts);

			Point *meshNormal = new Point[tmpNormal.size()];
			copy(tmpNormal.begin(), tmpNormal.end(), meshNormal);

			UV *meshUV = new UV[tmpUvMaps.size()];
			copy(tmpUvMaps.begin(), tmpUvMaps.end(), meshUV);

			nodeSubName = nodeName + std::to_string(matIndex);

			//Check if we have UVs
			if (numUvs > 0)
			{
				//EnterCriticalSection(&meshcsect);
				scene.DefineMesh(nodeSubName, tmpMeshVerts.size(), tmpMeshTris.size(), (float *)meshVerts, (unsigned int *)meshTris, (float*)meshNormal, (float*)meshUV, NULL, NULL);
				//LeaveCriticalSection(&meshcsect);
			}
			else
			{
				//EnterCriticalSection(&meshcsect);
				scene.DefineMesh(nodeSubName, tmpMeshVerts.size(), tmpMeshTris.size(), (float *)meshVerts, (unsigned int *)meshTris, (float*)meshNormal, NULL, NULL, NULL);
				//LeaveCriticalSection(&meshcsect);
			}
			tmpMeshTris.clear();
			tmpMeshVerts.clear();
			tmpNormal.clear();
			tmpMeshTris.shrink_to_fit();
			tmpMeshVerts.shrink_to_fit();
			tmpNormal.shrink_to_fit();
			tmpUvMaps.clear();
			tmpUvMaps.shrink_to_fit();

		}
	}
	else
	{
		int count = 0;
		TVFace* tvfaces = p_trimesh->tvFace;
		Point3* uvverts = p_trimesh->tVerts;

		for (u_int faceIndex = 0; faceIndex < p_trimesh->getNumFaces(); ++faceIndex) {
			Face& face = p_trimesh->faces[faceIndex];
			u_int vertIndices[3];

			for (int vertIndex = 0; vertIndex < 3; ++vertIndex) {
				const int vIndex = vertIndex;

				const u_int vertsIndex = count++;
				vertIndices[vIndex] = vertsIndex;
				const u_int v = face.getVert(vIndex);
				//indTest.push_back(v);
				tmpMeshVerts.push_back(Point(p_trimesh->getVert(v)));

				u_int normalIndex = normals->GetNormalIndex(faceIndex, vIndex);
				Point tempNormal = normals->Normal(normalIndex);// .Normalize();
				tmpNormal.push_back(tempNormal);

				const int verColChanel = 0;
				MeshMap& mapChannel = p_trimesh->maps[verColChanel + 1];

				if (hasUV)
				{

					Point3 pt = uvverts[tvfaces[faceIndex].t[vIndex]];//->t[vIndex]];
					tmpUvMaps.push_back(UV(pt.x, pt.y * -1));
				}
			}
			tmpMeshTris.push_back(Triangle(vertIndices[0], vertIndices[1], vertIndices[2]));
			
		}

		// Allocate memory for LuxCore mesh data
		Triangle *meshTris = (Triangle *)Scene::AllocTrianglesBuffer(tmpMeshTris.size());
		copy(tmpMeshTris.begin(), tmpMeshTris.end(), meshTris);

		Point *meshVerts = (Point *)Scene::AllocVerticesBuffer(tmpMeshVerts.size());
		copy(tmpMeshVerts.begin(), tmpMeshVerts.end(), meshVerts);

		//Point *meshNormal = (Point *)Scene::AllocVerticesBuffer(normals->GetNumNormals());
		Point *meshNormal = new Point[tmpNormal.size()];
		copy(tmpNormal.begin(), tmpNormal.end(), meshNormal);

		UV *meshUV = new UV[tmpUvMaps.size()];
		copy(tmpUvMaps.begin(), tmpUvMaps.end(), meshUV);

		//Check if we have UVs
		if (numUvs > 0)
		{
			scene.DefineMesh(nodeName, tmpMeshVerts.size(), tmpMeshTris.size(), (float *)meshVerts, (unsigned int *)meshTris, (float *)meshNormal, (float *)meshUV, NULL, NULL);
		}
		else
		{
			scene.DefineMesh(nodeName, tmpMeshVerts.size(), tmpMeshTris.size(), (float *)meshVerts, (unsigned int *)meshTris, (float *)meshNormal, NULL, NULL, NULL);
		}
	}

	delete[] indices;
	tmpMeshVerts.clear();
	tmpMeshVerts.shrink_to_fit();
	tmpMeshTris.clear();
	tmpMeshTris.shrink_to_fit();
	tmpNormal.clear();
	tmpNormal.shrink_to_fit();
	tmpUvMaps.clear();
	tmpUvMaps.shrink_to_fit();
	uv = NULL;

	if ((numSubmtls > 1) && (!prev))
	{
		for (u_int matIndex = 0; matIndex < numSubmtls; matIndex++)
		{
			nodeSubName = nodeName + std::to_string(matIndex);
			objString = "scene.objects." + nodeSubName + ".ply = " + nodeSubName + "\n";
			props.SetFromString(objString);
			objString.empty();
		}
	}
	else
	{
		objString = "scene.objects." + nodeName + ".ply = " + nodeName + "\n";
		props.SetFromString(objString);
		objString.empty();
	}
		
	Mtl *objmat = NULL;
	objmat = currNode->GetMtl();

	if (!objmat)
	{
		DebugPrint(_M("\nMaxToLuxMesh -> Creating fallback material for object: %x"), objName);

		objString.append("scene.materials.undefined.type = matte");
		objString.append("\n");

		objString.append("scene.materials.undefined.kd = 0.5 0.5 0.5");
		objString.append("\n");

		if ((numSubmtls > 1) && (!prev))
		{
			for (u_int matIndex = 0; matIndex < numSubmtls; matIndex++)
			{
				nodeSubName = nodeName + std::to_string(matIndex);
				objString.append("scene.objects." + nodeSubName + ".material = undefined");
				objString.append("\n");
				objString.append("scene.objects." + nodeSubName + ".transformation = ");
				objString.append(lxmUtils->getMaxNodeTransform(currNode, t));
				objString.append("\n");
			}
		}
		else
		{
			objString.append("scene.objects." + nodeName + ".material = undefined");
			objString.append("\n");
			objString.append("scene.objects." + nodeName + ".transformation = ");
			objString.append(lxmUtils->getMaxNodeTransform(currNode, t));
			objString.append("\n");
		}
			
		props.SetFromString(objString);
		scene.Parse(props);
		objString.empty();
	}
	else
	{
		const wchar_t *matName = L"";
		//objmat = currNode->GetMtl();
		matName = objmat->GetName();
		std::string tmpMatName = lxmUtils->ToNarrow(matName);
		lxmUtils->removeUnwatedChars(tmpMatName);
		std::wstring replacedMaterialName = std::wstring(tmpMatName.begin(), tmpMatName.end());
		matName = replacedMaterialName.c_str();
			
		if (tmpMatName == "")
		{
			matName = L"unnamedMtl";
		}

		if ((numSubmtls > 1) && (!prev))
		{
			for (u_int matIndex = 0; matIndex < numSubmtls; matIndex++)
			{
				Mtl *m = objmat->GetSubMtl(matIndex);
				nodeSubName = nodeName + std::to_string(matIndex);
				if (lxmMaterials->isSupportedMaterial(m))
				{
					DebugPrint(_M("\nMaxToLuxMesh -> Creating supported material for object: %x"), nodeSubName);
					DebugPrint(_M("\nMaxToLuxMesh -> Material name: %x"), nodeSubName);
					OutputDebugStringW(L"\n======================================\n");

					lxmMaterials->setMaterial(m, scene);
				}
				else
				{
					DebugPrint(_M("\nMaxToLuxMesh -> Creating fallback material for object: %x"), nodeSubName);
					DebugPrint(_M("\nMaxToLuxMesh -> Material name: %x"), nodeSubName);
					OutputDebugStringW(L"\n======================================\n");

					objString.append("scene.materials." + nodeSubName + ".type = matte");
					objString.append("\n");

					objString.append("scene.materials." + nodeSubName + ".kd = 0.5 0.5 0.5");
					objString.append("\n");
				}
			}
		}
		else
		{
			if (lxmMaterials->isSupportedMaterial(objmat))
			{
				DebugPrint(_M("\nMaxToLuxMesh -> Creating supported material for object: %x"), objName);
				DebugPrint(_M("\nMaxToLuxMesh -> Material name: %x"), objName);
				OutputDebugStringW(L"\n======================================\n");

				lxmMaterials->setMaterial(objmat, scene);
			}
			else
			{
				DebugPrint(_M("\nMaxToLuxMesh -> Creating fallback material for object: %x"), objName);
				DebugPrint(_M("\nMaxToLuxMesh -> Material name: %x"), objName);
				OutputDebugStringW(L"\n======================================\n");

				objString.append("scene.materials." + lxmUtils->ToNarrow(matName) + ".type = matte");
				objString.append("\n");

				objString.append("scene.materials." + lxmUtils->ToNarrow(matName) + ".kd = 0.5 0.5 0.5");
				objString.append("\n");
			}
		}

		//objString = "";
		if ((numSubmtls > 1) && (!prev))
		{
			for (u_int matIndex = 0; matIndex < numSubmtls; matIndex++)
			{
				nodeSubName = nodeName + std::to_string(matIndex);
				Mtl *m = objmat->GetSubMtl(matIndex);
				const wchar_t *matName = L"";
				//objmat = currNode->GetMtl();
				matName = m->GetName();
				std::string tmpMatName = lxmUtils->ToNarrow(matName);
				lxmUtils->removeUnwatedChars(tmpMatName);
				std::wstring replacedMaterialName = std::wstring(tmpMatName.begin(), tmpMatName.end());
				matName = replacedMaterialName.c_str();
					
				objString.append("scene.objects." + nodeSubName + ".material = " + lxmUtils->ToNarrow(matName));
				objString.append("\n");
				objString.append("scene.objects." + nodeSubName + ".transformation = ");
				objString.append(lxmUtils->getMaxNodeTransform(currNode, t));
				objString.append("\n");
			}
		}
		else
		{
			objString.append("scene.objects." + nodeName + ".material = " + lxmUtils->ToNarrow(matName));
			objString.append("\n");
			objString.append("scene.objects." + nodeName + ".transformation = ");
			objString.append(lxmUtils->getMaxNodeTransform(currNode, t));
			objString.append("\n");
		}

		props.SetFromString(objString);
		scene.Parse(props);
		objString.empty();
	}
}

void MaxToLuxMesh::createMeshesInGroup(INode *currNode, luxcore::Scene &scene, TimeValue t)
{
	if (currNode->IsHidden(0, true))
		return;

	for (size_t i = 0; i < currNode->NumberOfChildren(); i++)
	{
		INode *groupChild;
		groupChild = currNode->GetChildNode(i);
		if (groupChild->IsGroupHead())
		{
			createMeshesInGroup(groupChild, scene,t);
		}
		else
		{
			createMesh(groupChild, scene,t);
		}
	}
}