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
// local includes
#include "TranslatorGraphNode.h"
#include "ITranslationManager_Internal.h"
// Max sdk
#include <Noncopyable.h>
#include <RenderingAPI/Translator/ITranslationManager.h>
#include <RenderingAPI/Translator/TranslatorKey.h>
#include <RenderingAPI/Renderer/IRenderingProcess.h>
// std
#include <unordered_set>

namespace Max
{;
namespace RenderingAPI
{;

class TranslatorGraphRootNode;

class TranslationManager : 
	public ITranslationManager_Internal, 
	public MaxSDK::Util::Noncopyable
{
public:

	explicit TranslationManager(IRenderSessionContext& render_session_context);
	~TranslationManager();
	
	// -- inherited from ITranslationManager
	virtual void ReleaseSceneTranslator(const Translator& root_translator) override;
	virtual TranslationResult UpdateScene(Translator& root_translator, const TimeValue t) override;
    virtual bool DoesSceneNeedUpdate(const Translator& root_translator, const TimeValue t) override;

    // -- inherited from ITranslationManager_Internal
    virtual TranslatorGraphNode& acquire_graph_node(const TranslatorKey& key, const TimeValue t) override;

protected:

    // -- inherited from ITranslationManager
    virtual Translator* TranslateSceneInternal(const TranslatorKey& key, const TimeValue t, TranslationResult& result) override;

private:

    class MainThreadUpdateJob;
    class MainThreadValidityCheckJob;

	// Performs garbage collection: deletes any orphan/unreferenced nodes in the DAG
	void garbage_collect_dag();

    // Updates the scene attached to the given root node
    TranslationResult update_scene_internal(TranslatorGraphRootNode& root_node, const TimeValue t);

private:

    IRenderSessionContext& m_render_session_context;

    // A table that manages the nodes in a translation graph
    template<class GraphNodeType>     // type can either been root or regular node
    class NodeTable
    {
    public:

        ~NodeTable();

        // Garbage collects and forcibly clears the list of nodes, asserting if the garbage collection hasn't cleared up all the nodes.
        void delete_all_nodes();

        // Returns the node that corresponds to the given key. Adds a new node if necessary.
        GraphNodeType& acquire_node(const TranslatorKey& input_key, ITranslationManager_Internal& translation_manager, IRenderSessionContext& render_session_context, const TimeValue t);

        // Garbage collects the nodes stored in this table
        void garbage_collect_nodes();

    private:

        // An entry in the table
        class TableEntry
        {
        public:
            // Creates an entry for the sole purpose of performing a find()
            static TableEntry CreateForFind(const TranslatorKey& key);
            // Creates an entry for the sole purpose of performing an insert()
            static TableEntry CreateForInsert(const TranslatorKey& key, std::unique_ptr<GraphNodeType>&& node);
            // Move constructor
            TableEntry(TableEntry&& from);
            GraphNodeType* get_node() const;
            const TranslatorKey& get_key() const;
        private:
            TableEntry(const TranslatorKey& key, std::unique_ptr<GraphNodeType>&& node, const bool do_clone_key);
            // Disallow copy
            TableEntry(TableEntry&);
            void operator=(TableEntry&);
        private:
            // The cloned key, if applicable
            std::unique_ptr<const TranslatorKey> m_cloned_key;
            // The node being referenced, if applicable
            std::unique_ptr<GraphNodeType> m_node;
            // the key being referenced
            const TranslatorKey& m_key;
        };

        struct NodeTableHash
        {
            size_t operator()(const TableEntry& entry) const;
        };
        struct NodeTableEquality
        {
            bool operator()(const TableEntry& lhs, const TableEntry& rhs) const;
        };

        // The underlying table, used to store the set of nodes
        std::unordered_set<TableEntry, NodeTableHash, NodeTableEquality> m_node_table;
    };

    // Set of root and child nodes, that make up the graph.
    NodeTable<TranslatorGraphRootNode> m_dag_root_nodes;
    NodeTable<TranslatorGraphNode> m_dag_child_nodes;
};

// This class handles executing a scene update from the main thread
class TranslationManager::MainThreadUpdateJob : 
    public IRenderingProcess::IMainThreadJob,
    public MaxSDK::Util::Noncopyable
{
public:

    MainThreadUpdateJob(
        TranslatorGraphRootNode& graph_root_node, 
        const TimeValue t,
        TranslationResult& result,
        TranslationManager& translation_manager);
    virtual ~MainThreadUpdateJob();

    // -- inherited from IMainThreadJob
    virtual void ExecuteMainThreadJob() override;

private:

    TranslationManager& m_translation_manager;
    TranslatorGraphRootNode& m_graph_root_node;
    const TimeValue m_time;
    TranslationResult& m_result;
};

// Handles checking for scene validity, from the main thread.
class TranslationManager::MainThreadValidityCheckJob : 
    public IRenderingProcess::IMainThreadJob,
    public MaxSDK::Util::Noncopyable
{
public:

    MainThreadValidityCheckJob(TranslatorGraphRootNode& graph_root_node, const TimeValue t);
    virtual ~MainThreadValidityCheckJob();

    // -- inherited from IMainThreadJob
    virtual void ExecuteMainThreadJob() override;

private:

    TranslatorGraphRootNode& m_graph_root_node;
    const TimeValue m_time;
};

}};	// namespace
