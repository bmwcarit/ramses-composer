/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/*
This is a tree-based item model, based on this tutorial: https://doc.qt.io/qt-5/qtwidgets-itemviews-simpletreemodel-example.html
The basic indexing structure of this model is:
[] invisible root node (-1)
  - root node (0)
  - root node (1)
    - child node (0)
    - child node (1)
  - root node (2)
Traversal through the entire tree is guaranteed by the iterateThroughTree() method.
*/

#pragma once

#include "object_tree_view_model/ObjectTreeNode.h"

#include "components/DataChangeDispatcher.h"
#include "components/DebugInstanceCounter.h"

#include "common_widgets/RaCoClipboard.h"
#include "style/Icons.h"

#include "core/ExtrefOperations.h"

#include <QAbstractItemModel>

namespace raco::object_tree::model {

class ObjectTreeViewDefaultModel : public QAbstractItemModel {
	DEBUG_INSTANCE_COUNTER(ObjectTreeViewDefaultModel);
	Q_OBJECT
		
public:
	enum ColumnIndex {
		COLUMNINDEX_NAME,
		COLUMNINDEX_TYPE,
		// invisible column that is used for ID-based filtering in the tree views
		COLUMNINDEX_ID,
		COLUMNINDEX_PROJECT,
		COLUMNINDEX_COLUMN_COUNT
	};

	ObjectTreeViewDefaultModel(raco::core::CommandInterface* commandInterface, components::SDataChangeDispatcher dispatcher, core::ExternalProjectsStoreInterface* externalProjectStore, const std::vector<std::string> &allowedCreatableUserTypes,
		bool groupExternalReferences = false, bool groupByType = false);
	
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& index, int role) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex& child) const override;
	QStringList decodeMimeData(const QMimeData* data) const;
	bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const override;
	bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
	Qt::ItemFlags flags(const QModelIndex& index) const override;
	QMimeData* mimeData(const QModelIndexList& indices) const override;
	QStringList mimeTypes() const override;
	Qt::DropActions supportedDropActions() const override;
	bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

	virtual void buildObjectTree();

	void iterateThroughTree(std::function<void(QModelIndex&)> nodeFunc, QModelIndex& currentIndex);
	ObjectTreeNode* indexToTreeNode(const QModelIndex& index) const;
	core::SEditorObject indexToSEditorObject(const QModelIndex& index) const;
	std::vector<core::SEditorObject> indicesToSEditorObjects(const QModelIndexList& indices) const;
	QModelIndex indexFromTreeNodeID(const std::string& id) const;

	core::UserObjectFactoryInterface* objectFactory() const;
	core::Project* project() const;

	std::set<std::string> externalProjectPathsAtIndices(const QModelIndexList& indices);

	virtual Qt::TextElideMode textElideMode() const;

	// Compare function that produces the following order: First sort by hierachy level then by row in scene graph.
	static bool isIndexAboveInHierachyOrPosition(QModelIndex left, QModelIndex right);

	std::pair<std::vector<core::SEditorObject>, std::set<std::string>> getObjectsAndRootIdsFromClipboardString(const std::string& serializedObjs) const;
	
	virtual bool canCopyAtIndices(const QModelIndexList& index) const;
	virtual bool canDeleteAtIndices(const QModelIndexList& indices) const;
	virtual bool canPasteIntoIndex(const QModelIndex& index, const std::vector<core::SEditorObject>& objects, const std::set<std::string>& sourceProjectTopLevelObjectIds, bool asExtRef = false) const;
	virtual bool canDuplicateAtIndices(const QModelIndexList& indices) const;

	virtual bool canDeleteUnusedResources() const;
	virtual bool canProgramaticallyGotoObject() const;

	virtual bool isObjectAllowedIntoIndex(const QModelIndex& index, const core::SEditorObject& obj) const;
	virtual std::vector<std::string> typesAllowedIntoIndex(const QModelIndex& index) const;

	virtual std::vector<std::string> creatableTypes(const QModelIndex& index) const;

Q_SIGNALS:
	void repaintRequested();
	void meshImportFailed(const std::string &filePath, const std::string &error);
	void dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles = QVector<int>());

public Q_SLOTS:
	core::SEditorObject createNewObject(const std::string& typeName, const std::string& nodeName = "", const QModelIndex& parent = QModelIndex());
	virtual size_t deleteObjectsAtIndices(const QModelIndexList& indices);
	virtual void copyObjectsAtIndices(const QModelIndexList& indices, bool deepCopy);
	virtual void cutObjectsAtIndices(const QModelIndexList& indices, bool deepCut);
	virtual bool pasteObjectAtIndex(const QModelIndex& index, bool pasteAsExtref = false, std::string* outError = nullptr, const std::string& serializedObjects = RaCoClipboard::get());
	virtual std::vector<core::SEditorObject> duplicateObjectsAtIndices(const QModelIndexList& indices);
	void moveScenegraphChildren(const std::vector<core::SEditorObject>& objects, core::SEditorObject parent, int row = -1);
	void importMeshScenegraph(const QString& filePath, const QModelIndex& selectedIndex);
	void deleteUnusedResources();

protected:
	components::SDataChangeDispatcher dispatcher_;
	std::unique_ptr<ObjectTreeNode> invisibleRootNode_;
	QModelIndex invisibleRootIndex_;
	core::CommandInterface* commandInterface_;
	core::ExternalProjectsStoreInterface* externalProjectStore_;
	std::vector<std::string> allowedUserCreatableUserTypes_;
	std::unordered_map<std::string, QModelIndex> indexes_;
	std::unordered_map<std::string, std::vector<components::Subscription>> nodeSubscriptions_;
	std::unordered_map<std::string, std::vector<components::Subscription>> lifeCycleSubscriptions_;
	components::Subscription afterDispatchSubscription_;
	components::Subscription extProjectChangedSubscription_;
	bool groupExternalReferences_;
	bool groupByType_;

	// The dirty flag is set if the tree needs to be rebuilt. See afterDispatchSubscription_ member variable usage.
	bool dirty_ = false;

	virtual std::vector<core::SEditorObject> filterForTopLevelObjects(const std::vector<core::SEditorObject>& objects) const;
	virtual void setNodeExternalProjectInfo(ObjectTreeNode* node) const;
	void constructTreeUnderNode(ObjectTreeNode* rootNode, const std::vector<core::SEditorObject>& children, bool groupExternalReferences, bool groupByTypes);

	void resetInvisibleRootNode();
	void updateTreeIndexes();

	static inline constexpr const char* OBJECT_EDITOR_ID_MIME_TYPE = "application/editorobject.id";

	const std::map<std::string, QIcon> typeIconMap{
		{"PerspectiveCamera", raco::style::Icons::instance().typeCamera},
		{"OrthographicCamera", raco::style::Icons::instance().typeCamera},
		{"Texture", raco::style::Icons::instance().typeTexture},
		{"CubeMap", raco::style::Icons::instance().typeCubemap},
		{"LuaScript", raco::style::Icons::instance().typeLuaScript},
		{"LuaInterface", raco::style::Icons::instance().typeLuaInterface},
		{"Material", raco::style::Icons::instance().typeMaterial},
		{"Mesh", raco::style::Icons::instance().typeMesh},
		{"MeshNode", raco::style::Icons::instance().typeMesh},
		{"Node", raco::style::Icons::instance().typeNode},
		{"Prefab", raco::style::Icons::instance().typePrefabInternal},
		{"ExtrefPrefab", raco::style::Icons::instance().typePrefabExternal},
		{"PrefabInstance", raco::style::Icons::instance().typePrefabInstance},
		{"LuaScriptModule", raco::style::Icons::instance().typeLuaScriptModule},
		{"AnimationChannel", raco::style::Icons::instance().typeAnimationChannel},
		{"Animation", raco::style::Icons::instance().typeAnimation},
		{"Timer", raco::style::Icons::instance().typeTimer},
		{"AnchorPoint", raco::style::Icons::instance().typeAnchorPoint},
		{"BlitPass", raco::style::Icons::instance().typeBlitPass}
	};

	std::string getOriginPathFromMimeData(const QMimeData* data) const;
	QMimeData* generateMimeData(const QModelIndexList& indexes, const std::string& originPath) const;
	void ensureTypeParentExists(std::map<std::string, ObjectTreeNode*>& typeParentMap, const std::string& typeName, ObjectTreeNode* parentNode);
};

}
