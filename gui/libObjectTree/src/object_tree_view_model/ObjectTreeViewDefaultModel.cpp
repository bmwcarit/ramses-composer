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

ObjectTreeViewDefaultModel::ObjectTreeViewDefaultModel(raco::core::CommandInterface* commandInterface,
	components::SDataChangeDispatcher dispatcher, 
	core::ExternalProjectsStoreInterface* externalProjectStore, 
	const std::vector<std::string>& allowedCreatableUserTypes, 
	bool groupExternalReferences)
	: dispatcher_{dispatcher},
	  commandInterface_{commandInterface},
	  externalProjectStore_{externalProjectStore},
	  allowedUserCreatableUserTypes_(allowedCreatableUserTypes),
	  groupExternalReferences_(groupExternalReferences) {
	resetInvisibleRootNode();

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

	auto treeNode = indexToTreeNode(index);

	switch (role) {
		case Qt::ItemDataRole::DecorationRole: {

			auto editorObj = treeNode->getRepresentedObject();
			if (editorObj && index.column() == COLUMNINDEX_NAME) {
				auto itr = typeIconMap.find(editorObj->getTypeDescription().typeName);
				if (itr == typeIconMap.end())
					return QVariant();
				return QVariant(itr->second);
			}
			return QVariant();
		}
		case Qt::ForegroundRole: {
			auto editorObj = treeNode->getRepresentedObject();
			if (editorObj && editorObj->query<ExternalReferenceAnnotation>() || treeNode->getType() == ObjectTreeNodeType::ExtRefGroup) {
				return QVariant(Colors::color(Colormap::externalReference));
			} else if (editorObj && Queries::isReadOnly(editorObj)) {
				return QVariant(Colors::color(Colormap::textDisabled));
			}
			return QVariant(Colors::color(Colormap::text));			
		}
		case Qt::ItemDataRole::DisplayRole: {
			switch (index.column()) {
				case COLUMNINDEX_NAME:
					return QVariant(QString::fromStdString(treeNode->getDisplayName()));
				case COLUMNINDEX_TYPE:
					return QVariant(QString::fromStdString(treeNode->getDisplayType()));
				case COLUMNINDEX_PROJECT: {
					return QVariant(QString::fromStdString(treeNode->getExternalProjectName()));
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
		return {};
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
	auto parentType = indexToTreeNode(parent)->getType();
	auto droppingAsExternalReference = QGuiApplication::queryKeyboardModifiers().testFlag(Qt::KeyboardModifier::AltModifier) || 
		parentType == ObjectTreeNodeType::ExtRefGroup;
	if (droppingAsExternalReference && (!droppingFromOtherProject || parentType == ObjectTreeNodeType::EditorObject)) {
		return false;
	}

	std::vector<SEditorObject> objectsFromId;
	std::set<std::string> sourceProjectTopLevelObjectIds;
	if (idList.empty()) {
		return false;
	}
	for (const auto& id : idList) {
		auto objectFromId = Queries::findById((droppingFromOtherProject) ? *externalProjectStore_->getExternalProjectCommandInterface(originPath)->project() : *project(), id.toStdString());
		if (objectFromId->getParent() == nullptr) {
			sourceProjectTopLevelObjectIds.emplace(id.toStdString());
		}
		objectsFromId.emplace_back(objectFromId);
	}
	return canPasteIntoIndex(parent, objectsFromId, sourceProjectTopLevelObjectIds, droppingAsExternalReference);
}

QModelIndex ObjectTreeViewDefaultModel::parent(const QModelIndex& child) const {
	if (!child.isValid()) {
		return {};
	}

	auto* childNode = indexToTreeNode(child);
	auto* parentNode = childNode->getParent();

	if (parentNode) {
		if (parentNode == invisibleRootNode_.get()) {
			return {};
		}
		return createIndex(parentNode->row(), COLUMNINDEX_NAME, parentNode);
	}

	return {};
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
	stream << QString::fromStdString(originPath);
	for (const auto& index : indexes) {
		if (index.column() == COLUMNINDEX_NAME) {
			if (auto obj = indexToSEditorObject(index)) {
				// Object ID
				stream << QString::fromStdString(obj->objectID());
			}
		}
	}

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
		std::vector<SEditorObject> objs;
		for (const auto& movedItemID : movedItemIDs) {
			if (auto childObj = project()->getInstanceByID(movedItemID.toStdString())) {
				objs.emplace_back(childObj);
			}
		}

		moveScenegraphChildren(objs, indexToSEditorObject(parent), row);
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
		auto pasteAsExtRef = pressedKeys.testFlag(Qt::KeyboardModifier::AltModifier) || indexToTreeNode(parent)->getType() == ObjectTreeNodeType::ExtRefGroup;
		pasteObjectAtIndex(parent, pasteAsExtRef, nullptr, serializedObjects);
	}

	return true;
}

Qt::ItemFlags ObjectTreeViewDefaultModel::flags(const QModelIndex& index) const {
	Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);

	if (auto obj = indexToSEditorObject(index); obj && !obj->query<ExternalReferenceAnnotation>()) {
		return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
	} else {
		return Qt::ItemIsDropEnabled | defaultFlags;
	}
}

QMimeData* ObjectTreeViewDefaultModel::mimeData(const QModelIndexList& indices) const {

	// Sort the list to make sure order after dropping remains consistent, regardless of what selectiong order is.
	auto sortedList = indices.toVector();
	std::sort(sortedList.begin(), sortedList.end(), ObjectTreeViewDefaultModel::isIndexAboveInHierachyOrPosition);

	return generateMimeData(QModelIndexList(sortedList.begin(), sortedList.end()), project()->currentPath());
}

QStringList ObjectTreeViewDefaultModel::mimeTypes() const {
	return {OBJECT_EDITOR_ID_MIME_TYPE};
}

void ObjectTreeViewDefaultModel::buildObjectTree() {
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

	auto filteredEditorObjects = filterForTopLevelObjects(project()->instances());

	beginResetModel();

	resetInvisibleRootNode();
	constructTreeUnderNode(invisibleRootNode_.get(), filteredEditorObjects, groupExternalReferences_);
	updateTreeIndexes();

	endResetModel();
}

void ObjectTreeViewDefaultModel::setNodeExternalProjectInfo(ObjectTreeNode* node) const {
	if (auto obj = node->getRepresentedObject()) {
		if (auto extrefAnno = obj->query<ExternalReferenceAnnotation>()) {
			node->setBelongsToExternalProject(
				project()->lookupExternalProjectPath(*extrefAnno->projectID_),
				project()->lookupExternalProjectName(*extrefAnno->projectID_));
		}
	}
}

void ObjectTreeViewDefaultModel::constructTreeUnderNode(ObjectTreeNode* rootNode, const std::vector<core::SEditorObject>& children, bool groupExternalReferences) {

	auto rootObject = rootNode->getRepresentedObject();
	auto extrefParent = groupExternalReferences ? nullptr : rootNode;

	for (const auto& obj : children) {
		auto parentNode = rootNode;

		if (obj->query<ExternalReferenceAnnotation>()) {
			if (!extrefParent) {
				extrefParent = new ObjectTreeNode(ObjectTreeNodeType::ExtRefGroup, nullptr);
				rootNode->addChildFront(extrefParent);
			}
			parentNode = extrefParent;
		}

		auto node = new ObjectTreeNode(obj, parentNode);
		setNodeExternalProjectInfo(node);
		constructTreeUnderNode(node, obj->children_->asVector<SEditorObject>(), false);			
	}
}

std::vector<SEditorObject> ObjectTreeViewDefaultModel::filterForTopLevelObjects(const std::vector<SEditorObject>& objects) const {
	return Queries::filterForTopLevelObjectsByTypeName(objects, allowedUserCreatableUserTypes_);
}

SEditorObject ObjectTreeViewDefaultModel::createNewObject(const EditorObject::TypeDescriptor& typeDesc, const std::string& nodeName, const QModelIndex& parent) {
	SEditorObject parentObj = indexToSEditorObject(parent);

	std::vector<SEditorObject> nodes;
	std::copy_if(project()->instances().begin(), project()->instances().end(), std::back_inserter(nodes), [this, parentObj](const SEditorObject& obj) {
		return obj->getParent() == parentObj;
	});

	auto name = project()->findAvailableUniqueName(nodes.begin(), nodes.end(), nullptr, nodeName.empty() ? raco::components::Naming::format(typeDesc.typeName) : nodeName);
	auto newObj = commandInterface_->createObject(typeDesc.typeName, name, parent.isValid() ? parentObj : nullptr);

    Q_EMIT editNodeOpreations();

	return newObj;
}

bool ObjectTreeViewDefaultModel::canCopyAtIndices(const QModelIndexList& indices) const {
	for (const auto& index : indices) {
		if (indexToSEditorObject(index)) {
			return true;
		}
	}
	return false;
}

bool ObjectTreeViewDefaultModel::canDeleteAtIndices(const QModelIndexList& indices) const {
	return !Queries::filterForDeleteableObjects(*commandInterface_->project(), indicesToSEditorObjects(indices)).empty();
}

bool ObjectTreeViewDefaultModel::isObjectAllowedIntoIndex(const QModelIndex& index, const core::SEditorObject& obj) const {
	if (auto parentObj = indexToSEditorObject(index); parentObj && !core::Queries::canPasteIntoObject(*commandInterface_->project(), parentObj)) {
		return false;
	}

	auto types = typesAllowedIntoIndex(index);

	return std::find(types.begin(), types.end(), obj->getTypeDescription().typeName) != types.end();
}

std::pair<std::vector<core::SEditorObject>, std::set<std::string>> ObjectTreeViewDefaultModel::getObjectsAndRootIdsFromClipboardString(const std::string& serializedObjs) const {
	auto deserialization{raco::serialization::deserializeObjects(serializedObjs)};
	if (deserialization) {
		auto objects = BaseContext::getTopLevelObjectsFromDeserializedObjects(*deserialization, project());
		return {objects, deserialization->rootObjectIDs};
	}
	return {};
}

bool ObjectTreeViewDefaultModel::canPasteIntoIndex(const QModelIndex& index, const std::vector<core::SEditorObject>& objects, const std::set<std::string>& sourceProjectTopLevelObjectIds, bool asExtRef) const {
	if (asExtRef) {
		// Only allow top level extref pasting
		if (indexToSEditorObject(index)) {
			return false;
		}

		// Allow pasting objects if any objects fits the location.
		for (const auto& obj : objects) {
			if (Queries::canPasteObjectAsExternalReference(obj, sourceProjectTopLevelObjectIds.find(obj->objectID()) != sourceProjectTopLevelObjectIds.end()) &&
				isObjectAllowedIntoIndex(index, obj)) {
				return true;
			}
		}
	}
	else {
		// Allow pasting objects if any objects fits the location.
		for (const auto& obj : objects) {
			if (isObjectAllowedIntoIndex(index, obj)) {
				return true;
			}
		}
	}


	return false;
}

bool ObjectTreeViewDefaultModel::canDuplicateAtIndices(const QModelIndexList& indices) const {
	if (indices.empty()) {
		return false;
	}

	std::vector<SEditorObject> objs;

	for (const auto& index : indices) {
		auto obj = indexToSEditorObject(index);
		if (!obj) {
			return false;
		}

		objs.emplace_back(obj);
	}

	return Queries::canDuplicateObjects(objs, *project());
}

size_t ObjectTreeViewDefaultModel::deleteObjectsAtIndices(const QModelIndexList& indices) {
    size_t t = commandInterface_->deleteObjects(indicesToSEditorObjects(indices));
    Q_EMIT editNodeOpreations();
    return t;
}

bool ObjectTreeViewDefaultModel::canDeleteUnusedResources() const {
	return core::Queries::canDeleteUnreferencedResources(*commandInterface_->project());
}

bool ObjectTreeViewDefaultModel::canProgramaticallyGotoObject() const {
	return true;
}

void ObjectTreeViewDefaultModel::deleteUnusedResources() {
	commandInterface_->deleteUnreferencedResources();
}

void ObjectTreeViewDefaultModel::copyObjectsAtIndices(const QModelIndexList& indices, bool deepCopy) {
	auto objects = indicesToSEditorObjects(indices);
	RaCoClipboard::set(commandInterface_->copyObjects(objects, deepCopy));
}

bool ObjectTreeViewDefaultModel::pasteObjectAtIndex(const QModelIndex& index, bool pasteAsExtref, std::string* outError, const std::string& serializedObjects) {
	bool success = true;
	try {
		commandInterface_->pasteObjects(serializedObjects, indexToSEditorObject(index), pasteAsExtref);
	} catch (std::exception &error) {
		success = false;
		*outError = error.what();
	}
	Q_EMIT editNodeOpreations();
	return success;
}

std::vector<SEditorObject> ObjectTreeViewDefaultModel::duplicateObjectsAtIndices(const QModelIndexList& indices) {
	auto objects = indicesToSEditorObjects(indices);
	return commandInterface_->duplicateObjects(objects);
}

void ObjectTreeViewDefaultModel::cutObjectsAtIndices(const QModelIndexList& indices, bool deepCut) {
	auto objects = indicesToSEditorObjects(indices);
	auto text = commandInterface_->cutObjects(objects, deepCut);
	if (!text.empty()) {
		RaCoClipboard::set(text);
	}
    Q_EMIT editNodeOpreations();
}

void ObjectTreeViewDefaultModel::moveScenegraphChildren(const std::vector<SEditorObject>& objects, SEditorObject parent, int row) {
	commandInterface_->moveScenegraphChildren(objects, parent, row);
    Q_EMIT editNodeOpreations();
}

void ObjectTreeViewDefaultModel::importMeshScenegraph(const QString& filePath, const QModelIndex& selectedIndex, bool &keyAnimation) {
	MeshDescriptor meshDesc;
	meshDesc.absPath = filePath.toStdString();
	meshDesc.bakeAllSubmeshes = false;
	auto selectedObject = indexToSEditorObject(selectedIndex);

    if (auto sceneGraph = commandInterface_->meshCache()->getMeshScenegraph(meshDesc)) {
		auto importDialog = new raco::common_widgets::MeshAssetImportDialog(*sceneGraph, nullptr);
		auto importStatus = importDialog->exec();
        keyAnimation = importDialog->animationKeyFrameButton_->isChecked();
		if (importStatus == QDialog::Accepted && selectedObject != nullptr) {
			bool projectZup = project()->settings()->axes_.asBool();
			ValueHandle translation_y{selectedObject, &user_types::Node::translation_, &core::Vec3f::y};
			ValueHandle translation_z{selectedObject, &user_types::Node::translation_, &core::Vec3f::z};
            ValueHandle rotation_x{selectedObject, &user_types::Node::rotation_, &core::Vec3f::x};
			float y = translation_y.as<float>();
			float z = translation_z.as<float>();
			float rot_x = rotation_x.as<float>();
            if (importDialog->yAxesUpButton_->isChecked() && projectZup) {
                commandInterface_->set(translation_y, -z);
                commandInterface_->set(translation_z, y);
                commandInterface_->set(rotation_x, rot_x + 90.0);
            } else if (importDialog->zAxesUpButton_->isChecked() && !projectZup) {
                commandInterface_->set(translation_y, z);
                commandInterface_->set(translation_z, -y);
                commandInterface_->set(rotation_x, rot_x - 90.0);
            }
			commandInterface_->insertAssetScenegraph(*sceneGraph, meshDesc.absPath, selectedObject);
		}
	} else {
		auto meshError = commandInterface_->meshCache()->getMeshError(meshDesc.absPath);
		Q_EMIT meshImportFailed(meshDesc.absPath, meshError);
	}
    Q_EMIT editNodeOpreations();
}

int ObjectTreeViewDefaultModel::rowCount(const QModelIndex& parent) const {
	if (auto* parentNode = indexToTreeNode(parent)) {
		return static_cast<int>(parentNode->childCount());
	}
	return 0;
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

std::vector<core::SEditorObject> ObjectTreeViewDefaultModel::indicesToSEditorObjects(const QModelIndexList& indices) const {
	std::vector<SEditorObject> objects;
	for (const auto& index : indices) {
		if (auto obj = indexToSEditorObject(index); obj) {
			objects.push_back(obj);
		}
	}
	return objects;
}

QModelIndex ObjectTreeViewDefaultModel::indexFromTreeNodeID(const std::string& id) const {
	auto cachedID = indexes_.find(id);
	if (cachedID != indexes_.end()) {
		return cachedID->second;
	}

	return QModelIndex();
}

void ObjectTreeViewDefaultModel::resetInvisibleRootNode() {
	invisibleRootNode_ = std::make_unique<ObjectTreeNode>(ObjectTreeNodeType::Root, nullptr);
}

void raco::object_tree::model::ObjectTreeViewDefaultModel::updateTreeIndexes() {
	indexes_.clear();
	iterateThroughTree([&](const auto& modelIndex) {
		indexes_[indexToTreeNode(modelIndex)->getID()] = modelIndex;
	},
		invisibleRootIndex_);
}

UserObjectFactoryInterface* ObjectTreeViewDefaultModel::objectFactory() {
	return commandInterface_->objectFactory();
}

Project* ObjectTreeViewDefaultModel::project() const {
	return commandInterface_->project();
}

Qt::TextElideMode ObjectTreeViewDefaultModel::textElideMode() const {
	return Qt::TextElideMode::ElideRight;
}

bool ObjectTreeViewDefaultModel::isIndexAboveInHierachyOrPosition(QModelIndex left, QModelIndex right) {
	while (left.parent() != right.parent()) {
		left = left.parent();
		right = right.parent();

		if (!left.isValid()) {
			return true;
		} else if (!right.isValid()) {
			return false;
		}
	}

	return left.row() < right.row();
}

std::vector<std::string> raco::object_tree::model::ObjectTreeViewDefaultModel::typesAllowedIntoIndex(const QModelIndex& index) const {
	if (!core::Queries::canPasteIntoObject(*commandInterface_->project(), indexToSEditorObject(index))) {
		return {};
	} else {
		return allowedUserCreatableUserTypes_;
	}
}

}  // namespace raco::object_tree::model
