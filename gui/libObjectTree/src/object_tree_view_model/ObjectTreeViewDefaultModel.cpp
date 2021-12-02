/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "object_tree_view_model/ObjectTreeViewDefaultModel.h"

#include "common_widgets/MeshAssetImportDialog.h"
#include "core/Context.h"
#include "core/CommandInterface.h"
#include "core/EditorObject.h"
#include "core/ExternalReferenceAnnotation.h"
#include "core/Project.h"
#include "user_types/Prefab.h"
#include "core/Queries.h"
#include "log_system/log.h"
#include "object_tree_view_model/ObjectTreeNode.h"
#include "components/Naming.h"
#include "style/Colors.h"
#include "user_types/Mesh.h"
#include "user_types/PrefabInstance.h"
#include "user_types/UserObjectFactory.h"

#include <QApplication>
#include <QClipboard>
#include <QDataStream>
#include <QJsonObject>
#include <QMimeData>
#include <QProgressDialog>
#include <QSet>

namespace raco::object_tree::model {

using namespace raco::core;
using namespace raco::style;

ObjectTreeViewDefaultModel::ObjectTreeViewDefaultModel(raco::core::CommandInterface* commandInterface, components::SDataChangeDispatcher dispatcher, core::ExternalProjectsStoreInterface* externalProjectStore, const std::vector<std::string>& allowedCreatableUserTypes)
	: dispatcher_{dispatcher},
	  commandInterface_{commandInterface},
	  externalProjectStore_{externalProjectStore},
	  allowedUserCreatableUserTypes_(allowedCreatableUserTypes) {
	resetInvisibleRootNode();
	ObjectTreeViewDefaultModel::setUpTreeModificationFunctions();

	lifeCycleSubscriptions_["objectLifecycle"].emplace_back(dispatcher_->registerOnObjectsLifeCycle(
		[this](auto sEditorObject) { dirty_ = true; },
		[this](auto sEditorObject) { dirty_ = true; }));

	afterDispatchSubscription_ = dispatcher_->registerOnAfterDispatch([this]() {
		if (dirty_) {
			buildObjectTree();
		}
	});

	nodeSubscriptions_["objectName"].emplace_back(dispatcher_->registerOnPropertyChange("objectName", [this](ValueHandle handle) {
		// Small optimization: Only set model dirty if the object with the changed name is actually in the model.
		if (indexes_.count(handle.rootObject()->objectID()) > 0) {
			dirty_ = true;
		}
	}));
	nodeSubscriptions_["children"].emplace_back(dispatcher_->registerOnPropertyChange("children", [this](ValueHandle handle) {
		dirty_ = true;
	}));

	extProjectChangedSubscription_ = dispatcher_->registerOnExternalProjectMapChanged([this]() { dirty_ = true; });

	dirty_ = true;
}

int ObjectTreeViewDefaultModel::columnCount(const QModelIndex& parent) const {
	return COLUMNINDEX_COLUMN_COUNT;
}

QVariant ObjectTreeViewDefaultModel::data(const QModelIndex& index, int role) const {
	if (!index.isValid()) {
		return QVariant();
	}

	switch (auto editorObj = indexToSEditorObject(index); role) {
		case Qt::ItemDataRole::DecorationRole: {
			switch (index.column()) {
				case COLUMNINDEX_NAME:
					if (editorObj->query<ExternalReferenceAnnotation>() && editorObj->as<user_types::Prefab>()) {
						return QVariant(Icons::icon(typeIconMap.at("ExtrefPrefab")));
					} else {
						auto itr = typeIconMap.find(editorObj->getTypeDescription().typeName);
						if (itr == typeIconMap.end())
							return QVariant();
						return QVariant(Icons::icon(itr->second));
					}
			}
			return QVariant(QIcon());
		}
		case Qt::ForegroundRole: {
			if (editorObj->query<ExternalReferenceAnnotation>()) {
				return QVariant(Colors::color(Colormap::externalReference));
			} else if (Queries::isReadOnly(editorObj)) {
				return QVariant(Colors::color(Colormap::textDisabled));
			} else {
				return QVariant(Colors::color(Colormap::text));
			}
		}
		case Qt::ItemDataRole::DisplayRole: {
			switch (index.column()) {
				case COLUMNINDEX_NAME:
					return QVariant(QString::fromStdString(editorObj->objectName()));
				case COLUMNINDEX_TYPE:
					return QVariant(QString::fromStdString(editorObj->getTypeDescription().typeName));
				case COLUMNINDEX_PROJECT: {
					if (auto extrefAnno = editorObj->query<ExternalReferenceAnnotation>()) {
						return QVariant(QString::fromStdString(project()->lookupExternalProjectName(*extrefAnno->projectID_)));
					}
					return QVariant();
				}
			}
		}
	}

	return QVariant();
}

QVariant ObjectTreeViewDefaultModel::headerData(int section, Qt::Orientation orientation, int role) const {
	switch (role) {
		case Qt::ItemDataRole::DisplayRole: {
			switch (section) {
				case COLUMNINDEX_NAME:
					return QVariant("Name");
				case COLUMNINDEX_TYPE:
					return QVariant("Type");
				case COLUMNINDEX_PROJECT:
					return QVariant("Project Name");
			}
		}
	}

	return QVariant();
}

QModelIndex ObjectTreeViewDefaultModel::index(int row, int column, const QModelIndex& parent) const {
	auto* parentNode = indexToTreeNode(parent);

	if (!parentNode) {
		return getInvisibleRootIndex();
	} else {
		return createIndex(row, column, parentNode->getChild(row));
	}
}

bool ObjectTreeViewDefaultModel::canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const {
	if (action == Qt::IgnoreAction) {
		return false;
	}
	if (!data->hasFormat(OBJECT_EDITOR_ID_MIME_TYPE)) {
		return false;
	}

	auto idList{decodeMimeData(data)};
	auto dragDroppingProjectNode = externalProjectStore_->isExternalProject(idList.front().toStdString());

	if (dragDroppingProjectNode) {
		return false;
	}

	auto originPath = getOriginPathFromMimeData(data);
	auto droppingFromOtherProject = originPath != project()->currentPath();
	auto droppingAsExternalReference = QGuiApplication::queryKeyboardModifiers().testFlag(Qt::KeyboardModifier::AltModifier);
	if (droppingAsExternalReference && parent.isValid()) {
		return false;
	}

	std::vector<SEditorObject> objectsFromId;
	if (idList.empty()) {
		return false;
	}
	for (const auto& id : idList) {
		auto objectFromId = Queries::findById((droppingFromOtherProject) ? *externalProjectStore_->getExternalProjectCommandInterface(originPath)->project() : *project(), id.toStdString());
		auto tryingToDropNonExtRefAsExtRef = droppingFromOtherProject && droppingAsExternalReference && !raco::core::Queries::canPasteObjectAsExternalReference(objectFromId, objectFromId->getParent() == nullptr);
		if (tryingToDropNonExtRefAsExtRef) {
			return false;
		}
		objectsFromId.emplace_back(objectFromId);
	}
	return objectsAreAllowedInModel(objectsFromId, parent);
}

QModelIndex ObjectTreeViewDefaultModel::parent(const QModelIndex& child) const {
	if (!child.isValid()) {
		return QModelIndex();
	}

	auto* childNode = indexToTreeNode(child);
	auto* parentNode = childNode->getParent();

	if (parentNode) {
		if (parentNode == invisibleRootNode_.get()) {
			return getInvisibleRootIndex();
		}
		return createIndex(parentNode->row(), COLUMNINDEX_NAME, parentNode);
	}

	return QModelIndex();
}

Qt::DropActions ObjectTreeViewDefaultModel::supportedDropActions() const {
	return Qt::MoveAction | Qt::CopyAction;
}

std::string ObjectTreeViewDefaultModel::getOriginPathFromMimeData(const QMimeData* data) const {
	QByteArray encodedData = data->data(OBJECT_EDITOR_ID_MIME_TYPE);
	QDataStream stream(&encodedData, QIODevice::ReadOnly);

	QString mimeDataOriginProjectPath;
	stream >> mimeDataOriginProjectPath;

	return mimeDataOriginProjectPath.toStdString();
}

QMimeData* raco::object_tree::model::ObjectTreeViewDefaultModel::generateMimeData(const QModelIndexList& indexes, const std::string& originPath) const {
	QMimeData* mimeData = new QMimeData();
	QByteArray encodedData;

	QDataStream stream(&encodedData, QIODevice::WriteOnly);
	LOG_TRACE(log_system::OBJECT_TREE_VIEW, "Start - Creating mime data of size {}", indexes.size());
	stream << QString::fromStdString(originPath);
	for (const auto& index : indexes) {
		if (index.isValid() && index.column() == COLUMNINDEX_NAME) {
			auto obj = indexToSEditorObject(index);
			// Object ID
			stream << QString::fromStdString(obj->objectID());
			LOG_TRACE(log_system::OBJECT_TREE_VIEW, "Add - {}", obj->objectID());
		}
	}
	LOG_TRACE(log_system::OBJECT_TREE_VIEW, "End - Creating mime data");

	mimeData->setData(OBJECT_EDITOR_ID_MIME_TYPE, encodedData);
	return mimeData;
}

QStringList ObjectTreeViewDefaultModel::decodeMimeData(const QMimeData* data) const {
	QByteArray encodedData = data->data(OBJECT_EDITOR_ID_MIME_TYPE);
	QDataStream stream(&encodedData, QIODevice::ReadOnly);
	QStringList itemIDs;

	// Skipping
	QString mimeDataOriginProjectPath;
	stream >> mimeDataOriginProjectPath;

	while (!stream.atEnd()) {
		QString text;
		stream >> text;
		itemIDs << text;
	}

	return itemIDs;
}

bool ObjectTreeViewDefaultModel::dropMimeData(const QMimeData* data, Qt::DropAction action,
	int row, int column, const QModelIndex& parent) {
	if (action == Qt::IgnoreAction) {
		return true;
	}

	if (!data->hasFormat(OBJECT_EDITOR_ID_MIME_TYPE)) {
		return false;
	}

	auto originPath = getOriginPathFromMimeData(data);
	auto mimeDataContainsLocalInstances = originPath == project()->currentPath();
	auto movedItemIDs = decodeMimeData(data);


	if (mimeDataContainsLocalInstances) {
		SEditorObject parentObj;
		if (parent.isValid()) {
			parentObj = indexToSEditorObject(parent);
		}
		for (const auto& itemID : movedItemIDs) {
			if (auto childObj = project()->getInstanceByID(itemID.toStdString())) {
				moveScenegraphChild(childObj, parentObj, row);
			}
		}
	} else {
		auto originCommandInterface = externalProjectStore_->getExternalProjectCommandInterface(originPath);
		std::vector<SEditorObject> objs;
		for (const auto& movedItemID : movedItemIDs) {
			if (auto externalProjectObj = originCommandInterface->project()->getInstanceByID(movedItemID.toStdString())) {
				objs.emplace_back(externalProjectObj);
			}
		}
		auto serializedObjects = originCommandInterface->copyObjects(objs, true);

		auto pressedKeys = QGuiApplication::queryKeyboardModifiers();
		pasteObjectAtIndex(parent, pressedKeys.testFlag(Qt::KeyboardModifier::AltModifier), nullptr, serializedObjects);
	}

	return true;
}

Qt::ItemFlags ObjectTreeViewDefaultModel::flags(const QModelIndex& index) const {
	Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);

	if (index.isValid()) {
		return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
	} else {
		return Qt::ItemIsDropEnabled | defaultFlags;
	}
}

QMimeData* ObjectTreeViewDefaultModel::mimeData(const QModelIndexList& indexes) const {
	return generateMimeData(indexes, project()->currentPath());
}

QStringList ObjectTreeViewDefaultModel::mimeTypes() const {
	return {OBJECT_EDITOR_ID_MIME_TYPE};
}

void ObjectTreeViewDefaultModel::buildObjectTree() {
	LOG_TRACE(raco::log_system::OBJECT_TREE_VIEW, "Rebuilding Object Tree Model");
	dirty_ = false;
	if (!commandInterface_) {
		return;
	}

	// We don't have a settings object in the unit tests.
	if (project()->settings()) {
		nodeSubscriptions_["objectName"].emplace_back(dispatcher_->registerOn(ValueHandle(project()->settings(), {"objectName"}), [this]() {
			dirty_ = true;
		}));
	}

	auto& allEditorObjects = project()->instances();
	auto filteredEditorObjects = objectFilterFunc_(allEditorObjects);

	beginResetModel();

	resetInvisibleRootNode();
	treeBuildFunc_(invisibleRootNode_.get(), filteredEditorObjects);
	updateTreeIndexes();

	endResetModel();
}

void ObjectTreeViewDefaultModel::setUpTreeModificationFunctions() {
	setProjectObjectFilterFunction([](const auto& editorObjVec) {
		return editorObjVec;
	});

	setTreeBuildingFunction([this](auto* rootPtr, const auto& filteredEditorObjVec) {
		std::unordered_map<std::string, ObjectTreeNode*> nodeCache;
		std::vector<ObjectTreeNode*> sceneGraphNodes(filteredEditorObjVec.size());

		for (auto i = 0U; i < sceneGraphNodes.size(); ++i) {
			auto currentEditorObj = filteredEditorObjVec[i];
			sceneGraphNodes[i] = new ObjectTreeNode(filteredEditorObjVec[i]);
			nodeCache[currentEditorObj->objectID()] = sceneGraphNodes[i];
		}

		for (auto i = 0U; i < sceneGraphNodes.size(); ++i) {
			auto currentObj = filteredEditorObjVec[i];
			auto node = nodeCache[currentObj->objectID()];

			if (!currentObj->getParent()) {
				rootPtr->addChild(node);
			}
			Property<Table, ArraySemanticAnnotation, HiddenProperty> objChildren = currentObj->children_;
			auto childrenVec = objChildren->asVector<SEditorObject>();
			for (const auto& childObj : childrenVec) {
				node->addChild(nodeCache[childObj->objectID()]);
			}
		}
	});
}

SEditorObject ObjectTreeViewDefaultModel::createNewObject(const EditorObject::TypeDescriptor& typeDesc, const std::string& nodeName, const QModelIndex& parent) {
	std::vector<SEditorObject> nodes;
	if (parent == getInvisibleRootIndex()) {
		std::copy_if(project()->instances().begin(), project()->instances().end(), std::back_inserter(nodes), [](const SEditorObject& obj) { return obj->getParent() == nullptr; });
	} else {
		std::copy_if(project()->instances().begin(), project()->instances().end(), std::back_inserter(nodes), [this, parent](const SEditorObject& obj) {
			if (parent.isValid()) {
				return obj->getParent() == indexToSEditorObject(parent);
			} else {
				return false;
			}
		});
	}
	auto name = project()->findAvailableUniqueName(nodes.begin(), nodes.end(), nullptr, nodeName.empty() ? raco::components::Naming::format(typeDesc.typeName) : nodeName);

	auto newObj = commandInterface_->createObject(typeDesc.typeName, name, std::string(), parent.isValid() ? indexToSEditorObject(parent) : nullptr);

	return newObj;
}

bool ObjectTreeViewDefaultModel::canCopy(const QModelIndex& index) const {
	return index.isValid();
}

bool ObjectTreeViewDefaultModel::canDelete(const QModelIndex& index) const {
	if (index.isValid()) {
		auto obj = indexToSEditorObject(index);
		return obj && core::Queries::canDeleteObjects(*commandInterface_->project(), {obj});
	}
	return false;
}

bool ObjectTreeViewDefaultModel::canInsertMeshAssets(const QModelIndex& index) const {
	if (index.isValid()) {
		const auto sceneRootNode = indexToSEditorObject(index);
		return Queries::canPasteIntoObject(*project(), sceneRootNode);
	}

	return false;
}

bool ObjectTreeViewDefaultModel::canPasteInto(const QModelIndex& index, const std::string& serializedObjs, bool asExtRef) const {
	auto deserialization{raco::serialization::deserializeObjects(serializedObjs,
		raco::user_types::UserObjectFactoryInterface::deserializationFactory(commandInterface_->objectFactory()))};
	auto topLevelObjects = BaseContext::getTopLevelObjectsFromDeserializedObjects(deserialization, commandInterface_->objectFactory(), project());

	if (asExtRef) {
		if (index.isValid()) {
			return false;
		}
		for (const auto& topLevelObj : topLevelObjects) {
			if (!Queries::canPasteObjectAsExternalReference(topLevelObj, deserialization.rootObjectIDs.find(topLevelObj->objectID()) != deserialization.rootObjectIDs.end())) {
				return false;
			}
		}
	}


	return objectsAreAllowedInModel(topLevelObjects, index);
}

size_t ObjectTreeViewDefaultModel::deleteObjectAtIndex(const QModelIndex& index) {
	auto obj = indexToSEditorObject(index);
	return commandInterface_->deleteObjects({obj});
}

bool ObjectTreeViewDefaultModel::canDeleteUnusedResources() const {
	return core::Queries::canDeleteUnreferencedResources(*commandInterface_->project());
}

void ObjectTreeViewDefaultModel::deleteUnusedResources() {
	commandInterface_->deleteUnreferencedResources();
}

void ObjectTreeViewDefaultModel::copyObjectAtIndex(const QModelIndex& index, bool deepCopy) {
	RaCoClipboard::set(commandInterface_->copyObjects({indexToSEditorObject(index)}, deepCopy));
}

bool ObjectTreeViewDefaultModel::pasteObjectAtIndex(const QModelIndex& index, bool pasteAsExtref, std::string* outError, const std::string& serializedObjects) {
	bool success = true;
	commandInterface_->pasteObjects(serializedObjects, indexToSEditorObject(index), pasteAsExtref, &success, outError);
	return success;
}

void ObjectTreeViewDefaultModel::cutObjectAtIndex(const QModelIndex& index, bool deepCut) {
	auto obj = indexToSEditorObject(index);
	auto text = commandInterface_->cutObjects({obj}, deepCut);
	if (!text.empty()) {
		RaCoClipboard::set(text);
	}
}

void ObjectTreeViewDefaultModel::moveScenegraphChild(SEditorObject child, SEditorObject parent, int row) {
	commandInterface_->moveScenegraphChild(child, parent, row);
}

void ObjectTreeViewDefaultModel::importMeshScenegraph(const QString& filePath, const QModelIndex& selectedIndex) {
	MeshDescriptor meshDesc;
	meshDesc.absPath = filePath.toStdString();
	meshDesc.bakeAllSubmeshes = false;

	auto selectedObject = selectedIndex.isValid() ? indexToSEditorObject(selectedIndex) : nullptr;

	if (auto sceneGraph = commandInterface_->meshCache()->getMeshScenegraph(meshDesc)) {
		auto importStatus = raco::common_widgets::MeshAssetImportDialog(*sceneGraph, nullptr).exec();
		if (importStatus == QDialog::Accepted) {
			commandInterface_->insertAssetScenegraph(*sceneGraph, meshDesc.absPath, selectedObject);
		}
	} else {
		auto meshError = commandInterface_->meshCache()->getMeshError(meshDesc.absPath);
		Q_EMIT meshImportFailed(meshDesc.absPath, meshError);
	}
}

int ObjectTreeViewDefaultModel::rowCount(const QModelIndex& parent) const {
	if (auto* parentNode = indexToTreeNode(parent)) {
		return static_cast<int>(parentNode->childCount());
	}
	return 0;
}

QModelIndex ObjectTreeViewDefaultModel::getInvisibleRootIndex() const {
	return invisibleRootIndex_;
}

void ObjectTreeViewDefaultModel::iterateThroughTree(std::function<void(QModelIndex&)> nodeFunc, QModelIndex& currentIndex) {
	if (currentIndex.row() != -1) {
		nodeFunc(currentIndex);
	}
	for (int i = 0; i < rowCount(currentIndex); ++i) {
		auto childIndex = index(i, 0, currentIndex);
		iterateThroughTree(nodeFunc, childIndex);
	}
}

ObjectTreeNode* ObjectTreeViewDefaultModel::indexToTreeNode(const QModelIndex& index) const {
	if (index.isValid()) {
		if (auto* node = static_cast<ObjectTreeNode*>(index.internalPointer())) {
			return node;
		}
	}
	return invisibleRootNode_.get();
}

SEditorObject ObjectTreeViewDefaultModel::indexToSEditorObject(const QModelIndex& index) const {
	return indexToTreeNode(index)->getRepresentedObject();
}

QModelIndex ObjectTreeViewDefaultModel::indexFromObjectID(const std::string& id) const {
	auto cachedID = indexes_.find(id);
	if (cachedID != indexes_.end()) {
		return cachedID->second;
	}

	return QModelIndex();
}

void ObjectTreeViewDefaultModel::setProjectObjectFilterFunction(const ObjectFilterFunc& func) {
	objectFilterFunc_ = func;
}

void ObjectTreeViewDefaultModel::setTreeBuildingFunction(const ObjectTreeBuildFunc& func) {
	treeBuildFunc_ = func;
}

void ObjectTreeViewDefaultModel::resetInvisibleRootNode() {
	invisibleRootNode_ = std::make_unique<ObjectTreeNode>(nullptr);
}

void raco::object_tree::model::ObjectTreeViewDefaultModel::updateTreeIndexes() {
	indexes_.clear();
	iterateThroughTree([&](const auto& modelIndex) {
		indexes_[indexToSEditorObject(modelIndex)->objectID()] = modelIndex;
	},
		invisibleRootIndex_);
}

bool ObjectTreeViewDefaultModel::objectsAreAllowedInModel(const std::vector<core::SEditorObject>& objs, const QModelIndex& parentIndex) const {
	if (!core::Queries::canPasteIntoObject(*commandInterface_->project(), indexToSEditorObject(parentIndex))) {
		return false;
	}

	auto objsContainSceneGraphObjs = false;
	auto objsContainPrefabs = false;
	auto objsContainPrefabInstances = false;

	for (const auto& obj : objs) {
		if (obj->as<user_types::Prefab>()) {
			objsContainPrefabs = true;
		} else if (obj->as<user_types::PrefabInstance>()) {
			objsContainPrefabInstances = true;
		}

		if (!obj->getTypeDescription().isResource) {
			objsContainSceneGraphObjs = true;
		}
	}

	if (objsContainPrefabs && !objsContainPrefabInstances) {
		return false;
	}

	return objsContainSceneGraphObjs;
}


UserObjectFactoryInterface* ObjectTreeViewDefaultModel::objectFactory() {
	return commandInterface_->objectFactory();
}

Project* ObjectTreeViewDefaultModel::project() const {
	return commandInterface_->project();
}

Qt::TextElideMode raco::object_tree::model::ObjectTreeViewDefaultModel::textElideMode() const {
	return Qt::TextElideMode::ElideRight;
}

std::vector<std::string> raco::object_tree::model::ObjectTreeViewDefaultModel::allowedCreatableUserTypes(const QModelIndexList& selectedIndexes) const {
	return allowedUserCreatableUserTypes_;
}

}  // namespace raco::object_tree::model