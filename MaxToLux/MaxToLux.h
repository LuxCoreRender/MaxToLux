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

#ifndef MaxToLux__H
#define MaxToLux__H
#include "maxtextfile.h"
#include <iostream>
#include <string>
#include <luxcore.h>
#include <StopWatch.h>
#include <ITabDialog.h>
#include <interactiverender.h>
#include <ISceneEventManager.h>

#define REND_CLASS_ID Class_ID(98,0);

#if MAX_PRODUCT_YEAR_NUMBER >= 2017
#define MAX2017_OVERRIDE override
#else
#define MAX2017_OVERRIDE
#endif