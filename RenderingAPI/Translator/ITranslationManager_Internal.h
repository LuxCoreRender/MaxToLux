//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include <RenderingAPI/Translator/ITranslationManager.h>

namespace Max
{;
namespace RenderingAPI
{;

// This interface adds internal functionality to the ITranslationManager interface. The point of this interface is to expose some of the manager's
// functionality to TranslatorGraphNode without creating a circular dependency between the two classes.
class ITranslationManager_Internal : 
	public ITranslationManager
{
public:

    // Returns an existing or newly created graph node for the given translator key.
    virtual TranslatorGraphNode& acquire_graph_node(const TranslatorKey& key, const TimeValue t) = 0;
};

}};	// namespace








