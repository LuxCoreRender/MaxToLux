//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "TranslationManager.h"

// local includes
#include "../resource.h"
#include "TranslatorGraphNode.h"
#include "TranslatorGraphRootNode.h"
// max sdk
#include <RenderingAPI/Renderer/IRenderSessionContext.h>
#include <RenderingAPI/Renderer/IRenderSettingsContainer.h>
#include <assert1.h>
#include <dllutilities.h>

//==================================================================================================
// class TranslationManager
//==================================================================================================

namespace Max
{;
namespace RenderingAPI
{;

TranslationManager::TranslationManager(IRenderSessionContext& render_session_context)
	: m_render_session_context(render_session_context)
{

}

TranslationManager::~TranslationManager()
{
	// Here we free all the nodes in the DAG.
    // First we release all the root nodes - to release any references to child nodes.
    m_dag_root_nodes.delete_all_nodes();
    m_dag_child_nodes.delete_all_nodes();
}

Translator* TranslationManager::TranslateSceneInternal(const TranslatorKey& key, const TimeValue t, TranslationResult& result)
{
	// Acquire the node
	TranslatorGraphRootNode& root_node = m_dag_root_nodes.acquire_node(key, *this, m_render_session_context, t);

    // Increment the reference count of this scene root node
    root_node.increment_reference_count();

    // Translate/update the scene
    result = TranslationResult::Failure;
    if (root_node.get_translator() != nullptr)
    {
        result = UpdateScene((*root_node.get_translator()), t);
    }

	if(result == TranslationResult::Success)
	{
        return root_node.get_translator();
	}
    else
    {
        // Release our reference on the scene
        root_node.decrement_reference_count();
        // Garbage collect to get rid of this unused scene
        garbage_collect_dag();

	    return nullptr;
    }
}

TranslatorGraphNode& TranslationManager::acquire_graph_node(const TranslatorKey& key, const TimeValue t)
{
    TranslatorGraphNode& node = m_dag_child_nodes.acquire_node(key, *this, m_render_session_context, t);
    return node;
}

void TranslationManager::ReleaseSceneTranslator(const Translator& root_translator)
{
    TranslatorGraphRootNode* root_translator_node = dynamic_cast<TranslatorGraphRootNode*>(&root_translator.GetGraphNode());
    if(root_translator_node != nullptr)
    {
	    if(root_translator_node->decrement_reference_count() == 0)
	    {
		    // Perform garbage collection, now that this scene is no longer referenced
		    garbage_collect_dag();
	    }
    }
    else
    {
        DbgAssert(false);
    }
}

TranslationResult TranslationManager::update_scene_internal(TranslatorGraphRootNode& root_node, const TimeValue t)
{
    // Time translation total
    IRenderingProcess::NamedTimerGuard translation_timer_guard(MaxSDK::GetResourceStringAsMSTR(IDS_TIMING_TOTAL_TRANSLATION), m_render_session_context.GetRenderingProcess());

    // Translates, as needed, every single node in the DAG.
    const TranslationResult result = root_node.update_scene_graph(t);

    // When doing ActiveShade, we need to call RenderEnd() immediately after the scene has been translated/udpated, such that Viewport
    // rendering doesn't start using the render meshes and such.
    if(m_render_session_context.GetRenderSettings().GetRenderTargetType() == IRenderSettingsContainer::RenderTargetType::Interactive)
    {
        m_render_session_context.CallRenderEnd(t);
    }

    // Perform a garbage collection, in case some nodes were orphans in the process.
    garbage_collect_dag();

    return result;
}

TranslationResult TranslationManager::UpdateScene(Translator& root_translator, const TimeValue t)
{
    TranslatorGraphRootNode* root_translator_node = dynamic_cast<TranslatorGraphRootNode*>(&root_translator.GetGraphNode());
    if(root_translator_node != nullptr)
    {
        // Run the update job from the main thread: it's not safe to access the 3ds max scene from a thread other than the main one
        TranslationResult result = TranslationResult::Failure;
        MainThreadUpdateJob update_job(*root_translator_node, t, result, *this);

        if(!m_render_session_context.GetRenderingProcess().RunJobFromMainThread(update_job))
        {
            return TranslationResult::Aborted;
        }
        else
        {
            return result;
        }
    }
    else
    {
        DbgAssert(false);
        return TranslationResult::Failure;
    }
}

bool TranslationManager::DoesSceneNeedUpdate(const Translator& root_translator, const TimeValue t) 
{
    TranslatorGraphRootNode* root_translator_node = dynamic_cast<TranslatorGraphRootNode*>(&root_translator.GetGraphNode());
    if(DbgVerify(root_translator_node != nullptr))
    {
        // Check if the scene graph is invalid
        if(!root_translator_node->is_scene_valid(t))
        {
            return true;
        }
        // Check if the scene graph is flagged for potential deferred invalidation
        else if(root_translator_node->is_scene_maybe_invalid())
        {
            // Perform the validity check from the main thread
            TranslationResult result = TranslationResult::Failure;
            MainThreadValidityCheckJob validity_check_job(*root_translator_node, t);
            if(m_render_session_context.GetRenderingProcess().RunJobFromMainThread(validity_check_job))
            {
                const bool is_invalid = !root_translator_node->is_scene_valid(t);
                return is_invalid;
            }
            else
            {
                // Abort requested: just don't update the scene and let the render abort normally
            }
        }
    }

    return false;
}

void TranslationManager::garbage_collect_dag()
{
    // Collect root nodes first
    m_dag_root_nodes.garbage_collect_nodes();

    // Collect inner nodes
    m_dag_child_nodes.garbage_collect_nodes();
}

//==================================================================================================
// class TranslationManager::MainThreadUpdateJob
//==================================================================================================

TranslationManager::MainThreadUpdateJob::MainThreadUpdateJob(
    TranslatorGraphRootNode& graph_root_node, 
    const TimeValue t,
    TranslationResult& result,
    TranslationManager& translation_manager)
    : m_translation_manager(translation_manager),
    m_graph_root_node(graph_root_node),
    m_time(t),
    m_result(result)
{

}

TranslationManager::MainThreadUpdateJob::~MainThreadUpdateJob()
{

}

void TranslationManager::MainThreadUpdateJob::ExecuteMainThreadJob()
{
    // Update the scene
    m_result = m_translation_manager.update_scene_internal(m_graph_root_node, m_time);
}

//==================================================================================================
// class TranslationManager::MainThreadValidityCheckJob
//==================================================================================================

TranslationManager::MainThreadValidityCheckJob::MainThreadValidityCheckJob(TranslatorGraphRootNode& graph_root_node, const TimeValue t)
    : m_graph_root_node(graph_root_node),
    m_time(t)
{

}

TranslationManager::MainThreadValidityCheckJob::~MainThreadValidityCheckJob()
{

}

void TranslationManager::MainThreadValidityCheckJob::ExecuteMainThreadJob()
{
    m_graph_root_node.check_scene_validity(m_time);
}

//==================================================================================================
// class TranslationManager::NodeTable
//==================================================================================================

template<class GraphNodeType>
TranslationManager::NodeTable<GraphNodeType>::~NodeTable()
{
    DbgAssert(m_node_table.empty());
}

template<class GraphNodeType>
void TranslationManager::NodeTable<GraphNodeType>::delete_all_nodes()
{
    // Perform a garbage collection upon deleting the table. Following this collection, the table should be empty -
    // otherwise there may be a bug in the logic.
    garbage_collect_nodes();
    DbgAssert(m_node_table.empty());

    m_node_table.clear();
}

template<class GraphNodeType>
GraphNodeType& TranslationManager::NodeTable<GraphNodeType>::acquire_node(
    const TranslatorKey& input_key,         
    ITranslationManager_Internal& translation_manager, 
    IRenderSessionContext& render_session_context,
    const TimeValue t)
{
    // See if a translator, for this key, already exists
    auto node_iterator = m_node_table.find(TableEntry::CreateForFind(input_key));

    // Create a new node if necessary
    if(node_iterator == m_node_table.end())
    {
        // Create a new node to store the translator in the DAG
        auto insert_result = m_node_table.insert(
            TableEntry::CreateForInsert(
                input_key,
                std::unique_ptr<GraphNodeType>(new GraphNodeType(input_key, translation_manager, render_session_context, t))));
        node_iterator = insert_result.first;
        DbgAssert(insert_result.second);
    }

    GraphNodeType* const node = node_iterator->get_node();
    DbgAssert(node != nullptr);
    return *node;
}

template<class GraphNodeType>
void TranslationManager::NodeTable<GraphNodeType>::garbage_collect_nodes()
{
    // Collect all collectible nodes. Perform multiple passes, as a previous pass may render other nodes collectible.
    bool any_nodes_collected = false;
    do
    {
        any_nodes_collected = false;
        for(auto it = m_node_table.begin(); it != m_node_table.end(); )
        {
            GraphNodeType* const node = it->get_node();
            if(DbgVerify(node != nullptr) && node->can_be_garbage_collected())
            {
                // Remove the entry from the table
                it = m_node_table.erase(it);
                any_nodes_collected = true;
            }
            else
            {
                ++it;
            }
        }
    } while(any_nodes_collected);
}

template<class GraphNodeType>
size_t TranslationManager::NodeTable<GraphNodeType>::NodeTableHash::operator()(const TableEntry& entry) const
{
    return entry.get_key().get_hash();
}

template<class GraphNodeType>
bool TranslationManager::NodeTable<GraphNodeType>::NodeTableEquality::operator()(const TableEntry& lhs, const TableEntry& rhs) const
{
    return lhs.get_key() == rhs.get_key();
}

//==================================================================================================
// class TranslationManager::NodeTable::TableEntry
//==================================================================================================

template<class GraphNodeType>
TranslationManager::NodeTable<GraphNodeType>::TableEntry::TableEntry(const TranslatorKey& key, std::unique_ptr<GraphNodeType>&& node, const bool do_clone_key)
    : m_cloned_key(do_clone_key ? key.CreateClone() : nullptr),
    m_node(std::move(node)),      
    m_key(do_clone_key ? *m_cloned_key : key)
{
    DbgAssert((m_key == key) && (m_key.get_hash() == key.get_hash()));
}

template<class GraphNodeType>
TranslationManager::NodeTable<GraphNodeType>::TableEntry::TableEntry(TableEntry&& from)
    : m_key(from.m_key),
    m_cloned_key(std::move(from.m_cloned_key)),
    m_node(std::move(from.m_node))
{

}

template<class GraphNodeType>
typename TranslationManager::NodeTable<GraphNodeType>::TableEntry TranslationManager::NodeTable<GraphNodeType>::TableEntry::CreateForFind(const TranslatorKey& key)
{
    return TableEntry(key, nullptr, false);
}

template<class GraphNodeType>
typename TranslationManager::NodeTable<GraphNodeType>::TableEntry TranslationManager::NodeTable<GraphNodeType>::TableEntry::CreateForInsert(const TranslatorKey& key, std::unique_ptr<GraphNodeType>&& node)
{
    return TableEntry(key, std::move(node), true);
}

template<class GraphNodeType>
GraphNodeType* TranslationManager::NodeTable<GraphNodeType>::TableEntry::get_node() const
{
    return m_node.get();
}

template<class GraphNodeType>
const TranslatorKey& TranslationManager::NodeTable<GraphNodeType>::TableEntry::get_key() const
{
    return m_key;
}


}};	// namespace

