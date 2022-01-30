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
#include "core/Queries.h"
#include "core/ExternalReferenceAnnotation.h"

#include <QFileDialog>

namespace raco::object_tree::model {

ObjectTreeViewExternalProjectModel::ObjectTreeViewExternalProjectModel(raco::core::CommandInterface* commandInterface, components::SDataChangeDispatcher dispatcher, core::ExternalProjectsStoreInterface* externalProjectsStore)
	: ObjectTreeViewDefaultModel(commandInterface, dispatcher, externalProjectsStore, {}, true) {
	// don't rebuild tree when creating/deleting local objects
	lifeCycleSubscriptions_.clear();

	projectChangedSubscription_ = dispatcher_->registerOnExternalProjectChanged([this]() { dirty_ = true; });
}

QVariant ObjectTreeViewExternalProjectModel::data(const QModelIndex& index, int role) const {
	if (!index.isValid()) {
		return QVariant();
	}

	auto treeNode = indexToTreeNode(index);
	if (treeNode->getType() == ObjectTreeNodeType::ExternalProject) {
		if (role == Qt::ForegroundRole) {
			if (commandInterface_->project()->usesExternalProjectByPath(indexToTreeNode(index)->getExternalProjectPath())) {
				return QVariant(raco::style::Colors::color(raco::style::Colormap::externalReference));
			} else {
				return QVariant(raco::style::Colors::color(raco::style::Colormap::text));
			}

		}

		if (role == Qt::ItemDataRole::DecorationRole && index.column() == COLUMNINDEX_NAME) {
			bool failed = false;

			auto originPath = treeNode->getExternalProjectPath();
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

	return ObjectTreeViewDefaultModel::data(index, role);
}

void ObjectTreeViewExternalProjectModel::addProject(const QString& projectPath) {
	std::vector<std::string> stack;
	stack.emplace_back(commandInterface_->project()->currentPath());
	if (externalProjectStore_->addExternalProject(projectPath.toStdString(), stack)) {
		LOG_INFO(raco::log_system::OBJECT_TREE_VIEW, "Added Project {} to Project Browser", projectPath.toStdString());
	}
}

void ObjectTreeViewExternalProjectModel::removeProjectsAtIndices(const QModelIndexList& indices) {
	std::set<std::string> projectsToRemove;
	for (const auto& index : indices) {
		projectsToRemove.emplace(indexToTreeNode(index)->getExternalProjectPath());
	}

	for (const auto& projectPath : projectsToRemove) {
		externalProjectStore_->removeExternalProject(projectPath);
	}
}

bool ObjectTreeViewExternalProjectModel::canRemoveProjectsAtIndices(const QModelIndexList& indices) {
	if (indices.isEmpty()) {
		return false;	
	}

	for (const auto& index : indices) {
		if (!index.isValid()) {
			return false;
		}		

		auto projectPath = indexToTreeNode(index)->getExternalProjectPath();
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

		auto treeNode = indexToTreeNode(index);
		auto object = treeNode->getRepresentedObject();
		if (object) {
			objects.push_back(object);

			// canCopyAtIndices already enforces that we only copy objects from the same project, so we can just take the project path from the first index
			if (!commandInterface) {
				auto originPath = treeNode->getExternalProjectPath();
				commandInterface = externalProjectStore_->getExternalProjectCommandInterface(originPath);
			}
		}
	}

	if (commandInterface) {
		RaCoClipboard::set(commandInterface->copyObjects(objects, deepCopy));
	}
}

void ObjectTreeViewExternalProjectModel::setNodeExternalProjectInfo(ObjectTreeNode* node) const {
	auto parent = node;

	while (parent->getRepresentedObject() || parent->getType() == ObjectTreeNodeType::ExtRefGroup) {
		parent = parent->getParent();
	}

	assert(parent->getType() == ObjectTreeNodeType::ExternalProject);

	auto projectPath = parent->getExternalProjectPath();
	auto projectName = parent->getExternalProjectName();
	if (auto obj = node->getRepresentedObject()) {
		if (auto extrefAnno = obj->query<raco::core::ExternalReferenceAnnotation>()) {
			if (auto originCommandInterface = externalProjectStore_->getExternalProjectCommandInterface(projectPath)) {
				auto project = originCommandInterface->project();
				// Keep the project path from the root project, even in case of extrefs. 
				// The utility functions in Context ensure proper copy paste behaviour for these.
				projectName = project->lookupExternalProjectName(*extrefAnno->projectID_);
			}
		}
	}
	node->setBelongsToExternalProject(projectPath, projectName);
}

void ObjectTreeViewExternalProjectModel::buildObjectTree() {
	beginResetModel();

	resetInvisibleRootNode();
	for (const auto& [projectPath, commandInterface] : externalProjectStore_->allExternalProjects()) {
		auto projectRootNode = new ObjectTreeNode(ObjectTreeNodeType::ExternalProject, invisibleRootNode_.get());
		auto projectName = commandInterface->project()->projectName();
		projectRootNode->setBelongsToExternalProject(projectPath, projectName);
		if (commandInterface) {
			auto filteredExternalProjectObjects = filterForTopLevelObjects(commandInterface->project()->instances());
			constructTreeUnderNode(projectRootNode, filteredExternalProjectObjects, groupExternalReferences_);
		}
	}
	updateTreeIndexes();
	endResetModel();

	dirty_ = false;
}

std::vector<core::SEditorObject> ObjectTreeViewExternalProjectModel::filterForTopLevelObjects(const std::vector<core::SEditorObject>& objects) const {
	return raco::core::Queries::filterForVisibleTopLevelObjects(objects);
}

Qt::ItemFlags ObjectTreeViewExternalProjectModel::flags(const QModelIndex& index) const {
	Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);

	if (auto obj = indexToSEditorObject(index)) {
		return Qt::ItemIsDragEnabled | defaultFlags;
	} else {
		return defaultFlags;
	}
}

QMimeData* ObjectTreeViewExternalProjectModel::mimeData(const QModelIndexList& indices) const {
	if (!canCopyAtIndices(indices)) {
		return nullptr;
	}

	auto originPath = indexToTreeNode(indices.front())->getExternalProjectPath();
	return ObjectTreeViewDefaultModel::generateMimeData(indices, originPath);
}

Qt::TextElideMode ObjectTreeViewExternalProjectModel::textElideMode() const {
	return Qt::TextElideMode::ElideLeft;
}

bool ObjectTreeViewExternalProjectModel::canCopyAtIndices(const QModelIndexList& indices) const {

	bool atLeastOneCopyableItem = false;
	std::string originPath;
	for (const auto& index : indices) {
		if (indexToSEditorObject(index)) {
			atLeastOneCopyableItem = true;

			// Make sure all items are (or are an extref) in the same external project
			auto indexOriginPath = indexToTreeNode(index)->getParent()->getExternalProjectPath();
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