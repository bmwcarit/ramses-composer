/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "object_tree_view_model/ObjectTreeViewExternalProjectModel.h"

#include "style/Colors.h"

#include "core/Project.h"
#include "core/CommandInterface.h"

#include <QFileDialog>

namespace raco::object_tree::model {

ObjectTreeViewExternalProjectModel::ObjectTreeViewExternalProjectModel(raco::core::CommandInterface* commandInterface, components::SDataChangeDispatcher dispatcher, core::ExternalProjectsStoreInterface* externalProjectsStore)
	: ObjectTreeViewDefaultModel(commandInterface, dispatcher, externalProjectsStore) {
	// don't rebuild tree when creating/deleting local objects
	lifeCycleSubscriptions_.clear();

	projectChangedSubscription_ = dispatcher_->registerOnExternalProjectChanged([this]() { dirty_ = true; });
}

QVariant ObjectTreeViewExternalProjectModel::data(const QModelIndex& index, int role) const {
	if (!index.isValid()) {
		return QVariant();
	}

	auto editorObj = indexToSEditorObject(index);
	if (editorObj->as<ProjectNode>()) {
		if (role == Qt::ForegroundRole) {
			if (commandInterface_->project()->usesExternalProjectByPath(editorObj->objectName())) {
				return QVariant(raco::style::Colors::color(raco::style::Colormap::externalReference));
			}
		}

		if (role == Qt::ItemDataRole::DecorationRole && index.column() == COLUMNINDEX_NAME) {
			bool failed = false;

			auto originPath = getOriginProjectPathOfSelectedIndex(index);
			auto* originCommandInterface = externalProjectStore_->getExternalProjectCommandInterface(originPath);
			if (originCommandInterface) {
				failed = originCommandInterface->project()->externalReferenceUpdateFailed();
			}

			if (failed) {
				return QVariant(raco::style::Icons::icon(Pixmap::error));
			} else {
				return QVariant(QIcon());
			}
		}
	}

	if (role == Qt::ItemDataRole::DisplayRole && index.column() == COLUMNINDEX_PROJECT) {
		auto originPath = getOriginProjectPathOfSelectedIndex(index);
		auto* originCommandInterface = externalProjectStore_->getExternalProjectCommandInterface(originPath);
		if (originCommandInterface) {
			auto editorObj = indexToSEditorObject(index);
			return QVariant(QString::fromStdString(originCommandInterface->project()->getProjectNameForObject(editorObj)));
		}
		return QVariant(QString());
	}
	return ObjectTreeViewDefaultModel::data(index, role);
}

void raco::object_tree::model::ObjectTreeViewExternalProjectModel::addProject(const QString& projectPath) {
	std::vector<std::string> stack;
	stack.emplace_back(commandInterface_->project()->currentPath());
	if (externalProjectStore_->addExternalProject(projectPath.toStdString(), stack)) {
		LOG_INFO(raco::log_system::OBJECT_TREE_VIEW, "Added Project {} to Project Browser", projectPath.toStdString());
	}
}

void raco::object_tree::model::ObjectTreeViewExternalProjectModel::removeProjectsAtIndices(const QModelIndexList& indices) {
	std::set<std::string> projectsToRemove;
	for (const auto& index : indices) {
		projectsToRemove.emplace(getOriginProjectPathOfSelectedIndex(index));
	}

	for (const auto& projectPath : projectsToRemove) {
		externalProjectStore_->removeExternalProject(projectPath);
	}
}

bool raco::object_tree::model::ObjectTreeViewExternalProjectModel::canRemoveProjectsAtIndices(const QModelIndexList& indices) {
	if (indices.isEmpty()) {
		return false;	
	}

	for (const auto& index : indices) {
		if (!index.isValid()) {
			return false;
		}		

		auto projectPath = getOriginProjectPathOfSelectedIndex(index);
		if (!externalProjectStore_->canRemoveExternalProject(projectPath)) {
			return false;
		}		
	}
	return true;
}

void ObjectTreeViewExternalProjectModel::copyObjectsAtIndices(const QModelIndexList& indices, bool deepCopy) {

	raco::core::CommandInterface* commandInterface = nullptr;

	std::vector<raco::core::SEditorObject> objects;
	for (const auto& index : indices) {

		auto object = indexToSEditorObject(index);
		if (&object->getTypeDescription() != &ProjectNode::typeDescription) {		

			objects.push_back(indexToSEditorObject(index));

			// canCopyAtIndices already enforces that we only copy objects from the same project, so we can just take the project path from the first index
			if (!commandInterface) {
				const auto& index = indices.front();
				auto originPath = getOriginProjectPathOfSelectedIndex(index);
				commandInterface = externalProjectStore_->getExternalProjectCommandInterface(originPath);
			}
		}
	}

	if (commandInterface) {
		RaCoClipboard::set(commandInterface->copyObjects(objects, deepCopy));
	}
}

void raco::object_tree::model::ObjectTreeViewExternalProjectModel::buildObjectTree() {
	beginResetModel();

	resetInvisibleRootNode();
	for (const auto& [projectPath, commandInterface] : externalProjectStore_->allExternalProjects()) {
		auto projectObj = std::make_shared<ProjectNode>(projectPath, projectPath);
		auto projectRootNode = new ObjectTreeNode(projectObj);
		invisibleRootNode_->addChild(projectRootNode);
		if (commandInterface) {
			auto filteredExternalProjectObjects = objectFilterFunc_(commandInterface->project()->instances());
			ObjectTreeViewDefaultModel::treeBuildFunc_(projectRootNode, filteredExternalProjectObjects);
		}
	}
	updateTreeIndexes();
	endResetModel();

	dirty_ = false;
}

Qt::ItemFlags ObjectTreeViewExternalProjectModel::flags(const QModelIndex& index) const {
	Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);

	return Qt::ItemIsDragEnabled | defaultFlags;
}

QMimeData* ObjectTreeViewExternalProjectModel::mimeData(const QModelIndexList& indexes) const {
	auto originPath = getOriginProjectPathOfSelectedIndex(indexes.front());
	return ObjectTreeViewDefaultModel::generateMimeData(indexes, originPath);
}

std::string ObjectTreeViewExternalProjectModel::getOriginProjectPathOfSelectedIndex(const QModelIndex& index) const {
	auto obj = indexToTreeNode(index);

	while (obj->getRepresentedObject()->getTypeDescription().typeName != ProjectNode::typeDescription.typeName) {
		obj = obj->getParent();
	}

	assert(obj->getRepresentedObject() != invisibleRootNode_->getRepresentedObject());

	return obj->getRepresentedObject()->objectName();
}

Qt::TextElideMode ObjectTreeViewExternalProjectModel::textElideMode() const {
	return Qt::TextElideMode::ElideLeft;
}

bool ObjectTreeViewExternalProjectModel::canCopyAtIndices(const QModelIndexList& indices) const {

	bool atLeastOneCopyableItem = false;
	std::string originPath;
	for (const auto& index : indices) {
		if (index.isValid() && indexToSEditorObject(index)->as<ProjectNode>() == nullptr) {
			atLeastOneCopyableItem = true;

			// Make sure all items belong to the same external project
			auto indexOriginPath = getOriginProjectPathOfSelectedIndex(index);
			if (originPath == "") {
				originPath = indexOriginPath;
			} else if (indexOriginPath != originPath) {
				return false;
			}
		}

	}

	return atLeastOneCopyableItem;
}

bool ObjectTreeViewExternalProjectModel::canDeleteAtIndices(const QModelIndexList& indices) const {
	return false;
}

bool ObjectTreeViewExternalProjectModel::canPasteIntoIndex(const QModelIndex& index, const std::vector<core::SEditorObject>& objects, const std::set<std::string>& sourceProjectTopLevelObjectIds, bool asExtRef) const {
	return false;
}

bool ObjectTreeViewExternalProjectModel::isObjectAllowedIntoIndex(const QModelIndex& index, const core::SEditorObject& obj) const {
	return false;
}

size_t ObjectTreeViewExternalProjectModel::deleteObjectsAtIndices(const QModelIndexList& index) {
	// Don't modify external project structure.
	return 0;
}

void ObjectTreeViewExternalProjectModel::cutObjectsAtIndices(const QModelIndexList& indices, bool deepCut) {
	// Don't modify external project structure.
}

bool ObjectTreeViewExternalProjectModel::pasteObjectAtIndex(const QModelIndex& index, bool pasteAsExtref, std::string* outError, const std::string& serializedObjects) {
	// Don't modify external project structure.
	return true;
}

}  // namespace raco::object_tree::model