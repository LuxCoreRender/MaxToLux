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

#include "frWrap.h"

#include "FireRenderer.h"
#include "Common.h"
#include "ClassDescs.h"
#include "utils\Thread.h"
#include "utils/Utils.h"
#include "ParamDlg.h"
#include <ITabDialog.h>
#include <Notify.h>
#include <toneop.h>
#include <RadeonProRender.h>
#include <RprLoadStore.h>
#if MAX_PRODUCT_YEAR_NUMBER >= 2016
#   include <scene/IPhysicalCamera.h>
#endif

#pragma warning(push, 3)
#pragma warning(disable:4198)
#include <Math/mathutils.h>
#pragma warning(pop)
#include "ParamBlock.h"
#include "imenuman.h"
#include <memory>
#include <shlobj.h>
#include <stack>
#include <WinUser.h>
#include <resource.h>
#include <bitmap.h>
#include <notify.h>
#include <gamma.h> // gamma export for FRS files
#include <IPathConfigMgr.h>  // for IPathConfigMgr