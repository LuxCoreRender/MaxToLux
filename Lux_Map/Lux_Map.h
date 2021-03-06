#pragma once

/***************************************************************************
 * Copyright 2019-2020 by author Omid Ghotbi "TAO" omidt.gh@gmail.com      *
 *                                                                         *
 *   This file is part of LuxCoreRender.                                   *
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

#include "3dsmaxsdk_preinclude.h"
#include "resource.h"
#include <istdplug.h>
#include <iparamb2.h>
#include <iparamm2.h>
#include <maxtypes.h>
//SIMPLE TYPE
#include <IMaterialBrowserEntryInfo.h>
#include <IMtlRender_Compatibility.h>


#include <stdmat.h>
#include <imtl.h>
#include <macrorec.h>


extern TCHAR *GetString(int id);

extern HINSTANCE hInstance;

/*class LuxMixMapBrowserEntryInfo
	: public IMaterialBrowserEntryInfo
{
public:
	const MCHAR* GetEntryName() const override;
	const MCHAR* GetEntryCategory() const override;
};*/
