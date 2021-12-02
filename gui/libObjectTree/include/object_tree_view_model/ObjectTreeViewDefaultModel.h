/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
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

using ObjectFilterFunc = std::function<std::vector<core::SEditorObject>(const std::vector<core::SEditorObject>&)>;
using ObjectTreeBuildFunc = std::function<void(ObjectTreeNode*, const std::vector<core::SEditorObject>&)>;

class ObjectTreeViewDefaultModel : public QAbstractItemModel {
	DEBUG_INSTANCE_COUNTER(ObjectTreeViewDefaultModel);
	Q_OBJECT

public:
	enum ColumnIndex {
		COLUMNINDEX_NAME,
		COLUMNINDEX_TYPE,
		COLUMNINDEX_PROJECT,
		COLUMNINDEX_COLUMN_COUNT
	};

	ObjectTreeViewDefaultModel(raco::core::CommandInterface* commandInterface, components::SDataChangeDispatcher dispatcher, core::ExternalProjectsStoreInterface* externalProjectStore, const std::vector<std::string> &allowedCreatableUserTypes = {});
	
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
	QMimeData* mimeData(const QModelIndexList& indexes) const override;
	QStringList mimeTypes() const override;
	Qt::DropActions supportedDropActions() const override;
	
	virtual void buildObjectTree();
	virtual void setUpTreeModificationFunctions();

	void iterateThroughTree(std::function<void(QModelIndex&)> nodeFunc, QModelIndex& currentIndex);
	QModelIndex getInvisibleRootIndex() const;
	ObjectTreeNode* indexToTreeNode(const QModelIndex& index) const;
	core::SEditorObject indexToSEditorObject(const QModelIndex& index) const;
	QModelIndex indexFromObjectID(const std::string& id) const;

	void setProjectObjectFilterFunction(const ObjectFilterFunc& func);
	void setTreeBuildingFunction(const ObjectTreeBuildFunc& func);

	core::UserObjectFactoryInterface* objectFactory();
	core::Project* project() const;

	virtual Qt::TextElideMode textElideMode() const;

	virtual std::vector<std::string> allowedCreatableUserTypes(const QModelIndexList& selectedIndexes) const;
	
	virtual bool canCopy(const QModelIndex& index) const;
	virtual bool canDelete(const QModelIndex& index) const;
	virtual bool canInsertMeshAssets(const QModelIndex& index) const;
	virtual bool canPasteInto(const QModelIndex& index, const std::string& serializedObjs = RaCoClipboard::get(), bool asExtRef = false) const;
	virtual bool canDeleteUnusedResources() const;
	virtual bool objectsAreAllowedInModel(const std::vector<core::SEditorObject>& objs, const QModelIndex& parentIndex) const;

Q_SIGNALS:
	void repaintRequested();
	void meshImportFailed(const std::string &filePath, const std::string &error);

public Q_SLOTS:
	core::SEditorObject createNewObject(const core::EditorObject::TypeDescriptor &typeDesc, const std::string &nodeName = "", const QModelIndex &parent = QModelIndex());
	virtual size_t deleteObjectAtIndex(const QModelIndex& index);
	virtual void copyObjectAtIndex(const QModelIndex& index, bool deepCopy);
	virtual void cutObjectAtIndex(const QModelIndex& index, bool deepCut);
	virtual bool pasteObjectAtIndex(const QModelIndex& index, bool pasteAsExtref = false, std::string* outError = nullptr, const std::string& serializedObjects = RaCoClipboard::get());
	void moveScenegraphChild(core::SEditorObject child, core::SEditorObject parent, int row = -1);
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

	// The dirty flag is set if the tree needs to be rebuilt. See afterDispatchSubscription_ member variable usage.
	bool dirty_ = false;

	ObjectFilterFunc objectFilterFunc_;
	ObjectTreeBuildFunc treeBuildFunc_;

	void resetInvisibleRootNode();
	void updateTreeIndexes();

	static inline constexpr const char* OBJECT_EDITOR_ID_MIME_TYPE = "application/editorobject.id";

	using Pixmap = ::raco::style::Pixmap;

	static inline const std::map<std::string, Pixmap> typeIconMap{
		{std::string("PerspectiveCamera"), Pixmap::typeCamera},
		{std::string("OrthographicCamera"), Pixmap::typeCamera},
		{std::string("Texture"), Pixmap::typeTexture},
		{std::string("CubeMap"), Pixmap::typeCubemap},
		{std::string("LuaScript"), Pixmap::typeScript},
		{std::string("Material"), Pixmap::typeMaterial},
		{std::string("Mesh"), Pixmap::typeMesh},
		{std::string("MeshNode"), Pixmap::typeMesh},
		{std::string("Node"), Pixmap::typeNode},
		{std::string("Prefab"), Pixmap::typePrefabInternal},
		{std::string("ExtrefPrefab"), Pixmap::typePrefabExternal},
		{std::string("PrefabInstance"), Pixmap::typePrefabInstance},
		{std::string{"AnimationChannel"}, Pixmap::typeAnimationChannel},
		{std::string{"Animation"}, Pixmap::typeAnimation}
	};

	std::string getOriginPathFromMimeData(const QMimeData* data) const;
	QMimeData* generateMimeData(const QModelIndexList& indexes, const std::string& originPath) const;
};

}