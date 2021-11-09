/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "object_tree_view/ObjectTreeView.h"


#include "components/RaCoPreferences.h"
#include "core/EditorObject.h"
#include "core/PathManager.h"
#include "core/Project.h"
#include "core/UserObjectFactoryInterface.h"

#include "object_tree_view_model/ObjectTreeNode.h"
#include "object_tree_view_model/ObjectTreeViewDefaultModel.h"
#include "object_tree_view_model/ObjectTreeViewExternalProjectModel.h"
#include "object_tree_view_model/ObjectTreeViewPrefabModel.h"
#include "object_tree_view_model/ObjectTreeViewResourceModel.h"
#include "utils/PathUtils.h"

#include <QContextMenuEvent>
#include <QFileDialog>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QShortcut>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>

namespace raco::object_tree::view {

using namespace raco::object_tree::model;

ObjectTreeView::ObjectTreeView(const QString &viewTitle, ObjectTreeViewDefaultModel *viewModel, QSortFilterProxyModel *sortFilterProxyModel, QWidget *parent)
	: QTreeView(parent), treeModel_(viewModel), proxyModel_(sortFilterProxyModel), viewTitle_(viewTitle) {
	setAlternatingRowColors(true);
	setContextMenuPolicy(Qt::CustomContextMenu);

	setDragDropMode(QAbstractItemView::DragDrop);
	setDragEnabled(true);
	setDropIndicatorShown(true);
	setSelectionMode(QAbstractItemView::SingleSelection);
	viewport()->setAcceptDrops(true);

	if (proxyModel_) {
		proxyModel_->setSourceModel(treeModel_);
		QTreeView::setModel(proxyModel_);
	} else {
		QTreeView::setModel(treeModel_);
	}

	setTextElideMode(treeModel_->textElideMode());

	connect(this, &ObjectTreeView::customContextMenuRequested, this, &ObjectTreeView::showContextMenu);
	connect(this, &ObjectTreeView::expanded, [this](const QModelIndex &index) {
		auto editorObj = indexToSEditorObject(index);
		expandedItemIDs_.insert(editorObj->objectID());
	});
	connect(this, &ObjectTreeView::collapsed, [this](const QModelIndex &index) {
		auto editorObj = indexToSEditorObject(index);
		expandedItemIDs_.erase(editorObj->objectID());
	});

	connect(this->selectionModel(), &QItemSelectionModel::selectionChanged, [this](const auto &selectedItemList, const auto &deselectedItemList) {
		if (auto externalProjectModel = (dynamic_cast<ObjectTreeViewExternalProjectModel *>(treeModel_))) {
			Q_EMIT externalObjectSelected();
			return;
		}

		std::set<ValueHandle> handles;
		for (const auto &selectedItemIndex : selectedItemList.indexes()) {
			auto selObj = indexToSEditorObject(selectedItemIndex);
			handles.emplace(selObj);
			selectedItemIDs_.emplace(selObj->objectID());
		}

		for (const auto &deselectedItem : deselectedItemList.indexes()) {
			auto selObj = indexToSEditorObject(deselectedItem);
			selectedItemIDs_.erase(selObj->objectID());
		}

		Q_EMIT newObjectTreeItemsSelected(handles);
	});

	connect(treeModel_, &ObjectTreeViewDefaultModel::modelReset, this, &ObjectTreeView::restoreItemExpansionStates);
	connect(treeModel_, &ObjectTreeViewDefaultModel::modelReset, this, &ObjectTreeView::restoreItemSelectionStates);

	setColumnWidth(ObjectTreeViewDefaultModel::COLUMNINDEX_NAME, width() / 3);

	auto cutShortcut = new QShortcut(QKeySequence::Cut, this, nullptr, nullptr, Qt::WidgetShortcut);
	QObject::connect(cutShortcut, &QShortcut::activated, this, &ObjectTreeView::cut);
	auto deleteShortcut = new QShortcut(QKeySequence::Delete, this, nullptr, nullptr, Qt::WidgetShortcut);
	QObject::connect(deleteShortcut, &QShortcut::activated, this, &ObjectTreeView::shortcutDelete);
}

std::set<core::ValueHandle> ObjectTreeView::getSelectedHandles() const {
	std::set<ValueHandle> handles;

	for (const auto &selectedItemIndex : selectionModel()->selectedIndexes()) {
		auto selObj = indexToSEditorObject(selectedItemIndex);
		handles.emplace(selObj);
	}

	return handles;
}

void ObjectTreeView::globalCopyCallback() {
	if (selectionModel()->selectedIndexes().size() > 0) {
		auto selectedItemIndex{selectionModel()->selectedIndexes().at(0)};
		if (proxyModel_) {
			selectedItemIndex = proxyModel_->mapToSource(selectedItemIndex);
		}
		if (canCopy(selectedItemIndex)) {
			treeModel_->copyObjectAtIndex(selectedItemIndex, false);
		}
	}
}

void ObjectTreeView::shortcutDelete() {
	if (selectionModel()->selectedIndexes().size() > 0) {
		auto selectedItemIndex{selectionModel()->selectedIndexes().at(0)};
		if (proxyModel_) {
			selectedItemIndex = proxyModel_->mapToSource(selectedItemIndex);
		}
		auto delObjAmount = treeModel_->deleteObjectAtIndex(selectedItemIndex);
		if (delObjAmount > 0) {
			selectionModel()->Q_EMIT selectionChanged({}, {});
		}
	}
}

void ObjectTreeView::selectObject(const QString &objectID) {
	if (objectID.isEmpty()) {
		resetSelection();
		return;
	}

	auto objectIndex = indexFromObjectID(objectID.toStdString());
	if (objectIndex.isValid()) {
		resetSelection();
		selectionModel()->select(objectIndex, SELECTION_MODE);
		scrollTo(objectIndex);
	}
}

void ObjectTreeView::expandAllParentsOfObject(const QString &objectID) {
	auto objectIndex = indexFromObjectID(objectID.toStdString());
	if (objectIndex.isValid()) {
		expandAllParentsOfObject(objectIndex);
	}
}

void ObjectTreeView::cut() {
	if (selectionModel()->selectedIndexes().size() > 0) {
		auto selectedItemIndex{selectionModel()->selectedIndexes().at(0)};
		if (proxyModel_) {
			selectedItemIndex = proxyModel_->mapToSource(selectedItemIndex);
		}
		treeModel_->cutObjectAtIndex(selectedItemIndex, false);
	}
}

void ObjectTreeView::globalPasteCallback(const QModelIndex &index, bool asExtRef) {
	auto selectionIndex = (proxyModel_) ? proxyModel_->mapToSource(index) : index;
	if (canPasteInto(selectionIndex)) {
		treeModel_->pasteObjectAtIndex(selectionIndex, asExtRef);
	}
}

QString ObjectTreeView::getViewTitle() const {
	return viewTitle_;
}

void ObjectTreeView::requestNewNode(EditorObject::TypeDescriptor nodeType, const std::string &nodeName, const QModelIndex &parent) {
	Q_EMIT dockSelectionFocusRequested(this);

	selectedItemIDs_.clear();
	auto createdObject = treeModel_->createNewObject(nodeType, nodeName, parent);
	selectedItemIDs_.insert(createdObject->objectID());
}

void ObjectTreeView::showContextMenu(const QPoint &p) {
	auto *treeViewMenu = createCustomContextMenu(p);
	treeViewMenu->exec(viewport()->mapToGlobal(p));
}

bool ObjectTreeView::canCopy(const QModelIndex &parentIndex) {
	return treeModel_->canCopy(parentIndex);
}

bool ObjectTreeView::canPasteInto(const QModelIndex &parentIndex, bool asExtref) {
	if (RaCoClipboard::hasEditorObject()) {
		return treeModel_->canPasteInto(parentIndex, RaCoClipboard::get(), asExtref);
	}
	return false;
}

QSortFilterProxyModel* ObjectTreeView::proxyModel() const {
	return proxyModel_;
}

void ObjectTreeView::resetSelection() {
	selectionModel()->reset();
	selectedItemIDs_.clear();
	viewport()->update();
}

QMenu* ObjectTreeView::createCustomContextMenu(const QPoint &p) {
	auto treeViewMenu = new QMenu(this);

	auto selectedItemIndex = indexAt(p);
	if (proxyModel_) {
		selectedItemIndex = proxyModel_->mapToSource(selectedItemIndex);
	}

	auto externalProjectModel = (dynamic_cast<ObjectTreeViewExternalProjectModel *>(treeModel_));
	auto prefabModel = (dynamic_cast<ObjectTreeViewPrefabModel *>(treeModel_));
	auto resourceModel = (dynamic_cast<ObjectTreeViewResourceModel *>(treeModel_));
	auto allTypes = treeModel_->objectFactory()->getTypes();
	auto allowedCreatableUserTypes = treeModel_->allowedCreatableUserTypes({selectedItemIndex});

	for (auto type : allowedCreatableUserTypes) {
		if (allTypes.count(type) > 0 && treeModel_->objectFactory()->isUserCreatable(type)) {
			auto typeDescriptor = allTypes[type];
			treeViewMenu->addAction(QString::fromStdString("Create " + type), [this, typeDescriptor, selectedItemIndex]() {
				requestNewNode(typeDescriptor.description, "", selectedItemIndex);
			});
		}
	}

	treeViewMenu->addSeparator();

	auto actionImport = treeViewMenu->addAction("Import glTF Assets...", [this, selectedItemIndex]() {
		auto sceneFolder = raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Mesh, treeModel_->project()->currentFolder());
		auto file = QFileDialog::getOpenFileName(this, "Load Asset File", QString::fromStdString(sceneFolder), "glTF files (*.gltf *.glb)");
		if (!file.isEmpty()) {
			treeModel_->importMeshScenegraph(file, selectedItemIndex);
		}
	});
	actionImport->setEnabled(treeModel_->canInsertMeshAssets(selectedItemIndex));

	if (!externalProjectModel || !allowedCreatableUserTypes.empty()) {
		treeViewMenu->addSeparator();
	}

	auto actionDelete = treeViewMenu->addAction(
		"Delete", [this, selectedItemIndex]() {
			treeModel_->deleteObjectAtIndex(selectedItemIndex);
			selectionModel()->Q_EMIT selectionChanged({}, {});
		},
		QKeySequence::Delete);
	actionDelete->setEnabled(treeModel_->canDelete(selectedItemIndex));

	auto actionCopy = treeViewMenu->addAction(
		"Copy", [this, selectedItemIndex]() { treeModel_->copyObjectAtIndex(selectedItemIndex, false); }, QKeySequence::Copy);
	actionCopy->setEnabled(canCopy(selectedItemIndex));

	auto actionPaste = treeViewMenu->addAction(
		(resourceModel) ? "Paste on Top Level" : "Paste", [this, selectedItemIndex]() {
		treeModel_->pasteObjectAtIndex(selectedItemIndex); }, QKeySequence::Paste);
	actionPaste->setEnabled(canPasteInto(selectedItemIndex));

	auto actionCut = treeViewMenu->addAction(
		"Cut", [this, selectedItemIndex]() { treeModel_->cutObjectAtIndex(selectedItemIndex, false); }, QKeySequence::Cut);
	actionCut->setEnabled(treeModel_->canDelete(selectedItemIndex));

	treeViewMenu->addSeparator();

	auto actionCopyDeep = treeViewMenu->addAction("Copy (Deep)", [this, selectedItemIndex]() { treeModel_->copyObjectAtIndex(selectedItemIndex, true); });
	actionCopyDeep->setEnabled(canCopy(selectedItemIndex));

	auto actionCutDeep = treeViewMenu->addAction("Cut (Deep)", [this, selectedItemIndex]() { treeModel_->cutObjectAtIndex(selectedItemIndex, true); });
	actionCutDeep->setEnabled(treeModel_->canDelete(selectedItemIndex));

	if (!externalProjectModel) {
		auto actionDeleteUnrefResources = treeViewMenu->addAction("Delete Unused Resources", [this] { treeModel_->deleteUnusedResources(); });
		actionDeleteUnrefResources->setEnabled(treeModel_->canDeleteUnusedResources());

		treeViewMenu->addSeparator();
		auto extrefPasteAction = treeViewMenu->addAction(
			"Paste As External Reference", [this, selectedItemIndex]() {
				std::string error;
				if (!treeModel_->pasteObjectAtIndex(selectedItemIndex, true, &error)) {
					QMessageBox::warning(this, "Paste As External Reference",
						fmt::format("Update of pasted external references failed!\n\n{}", error).c_str());
				}
			});
		extrefPasteAction->setEnabled(canPasteInto(selectedItemIndex, true));
	}

	if (externalProjectModel) {
		treeViewMenu->addSeparator();
		treeViewMenu->addAction("Add Project...", [this, externalProjectModel]() {
			auto projectFile = QFileDialog::getOpenFileName(this, tr("Import Project"), raco::components::RaCoPreferences::instance().userProjectsDirectory, tr("Ramses Composer Assembly (*.rca)"));
			if (projectFile.isEmpty()) {
				return;
			}
			if (projectFile.toStdString() == treeModel_->project()->currentPath()) {
				auto errorMessage = QString("Can't import external project with the same path as the currently open project %1.").arg(QString::fromStdString(treeModel_->project()->currentPath()));
				QMessageBox::critical(this, "Import Error", errorMessage);
				LOG_ERROR(log_system::OBJECT_TREE_VIEW, errorMessage.toStdString());
				return;
			}
			externalProjectModel->addProject(projectFile);
		});

		auto actionCloseImportedProject = treeViewMenu->addAction("Remove Project", [this, selectedItemIndex, externalProjectModel]() { externalProjectModel->removeProject(selectedItemIndex); });
		actionCloseImportedProject->setEnabled(selectedItemIndex.isValid() && externalProjectModel->canRemoveProject(selectedItemIndex));
	}

	return treeViewMenu;
}

void ObjectTreeView::dragMoveEvent(QDragMoveEvent *event) {
	setDropIndicatorShown(true);
	QTreeView::dragMoveEvent(event);

	// clear up QT drop indicator position confusion and don't allow below-item indicator for expanded items
	// because dropping an item above expanded items with the below-item indicator drops it to the wrong position
	auto indexBelowMousePos = indexAt(event->pos());
	if (isExpanded(indexBelowMousePos) && dropIndicatorPosition() == BelowItem) {
		event->setDropAction(Qt::DropAction::IgnoreAction);
		event->accept();
		setDropIndicatorShown(false);
	}
}

void ObjectTreeView::mousePressEvent(QMouseEvent *event) {
	QTreeView::mousePressEvent(event);
	if (!indexAt(event->pos()).isValid()) {
		resetSelection();
		Q_EMIT newObjectTreeItemsSelected({});
	}
}

core::SEditorObject ObjectTreeView::indexToSEditorObject(const QModelIndex &index) const {
	auto itemIndex = index;
	if (proxyModel_) {
		itemIndex = proxyModel_->mapToSource(index);
	}
	return treeModel_->indexToSEditorObject(itemIndex);
}

QModelIndex ObjectTreeView::indexFromObjectID(const std::string &id) const {
	auto index = treeModel_->indexFromObjectID(id);
	if (proxyModel_) {
		index = proxyModel_->mapFromSource(index);
	}

	return index;
}

void ObjectTreeView::restoreItemExpansionStates() {
	for (const auto &expandedObjectID : expandedItemIDs_) {
		auto expandedObjectIndex = indexFromObjectID(expandedObjectID);
		if (expandedObjectIndex.isValid()) {
			blockSignals(true);
			expand(expandedObjectIndex);
			blockSignals(false);
		}
	}
}

void ObjectTreeView::restoreItemSelectionStates() {
	selectionModel()->reset();
	std::vector<QModelIndex> selectedObjects;

	auto selectionIt = selectedItemIDs_.begin();
	while (selectionIt != selectedItemIDs_.end()) {
		const auto &selectionID = *selectionIt;
		auto selectedObjectIndex = indexFromObjectID(selectionID);
		if (selectedObjectIndex.isValid()) {
			selectionModel()->select(selectedObjectIndex, SELECTION_MODE);
			selectedObjects.emplace_back(selectedObjectIndex);
			expandAllParentsOfObject(selectedObjectIndex);
			++selectionIt;
		} else {
			selectionIt = selectedItemIDs_.erase(selectionIt);
		}
	}

	if (!selectedObjects.empty()) {
		scrollTo(selectedObjects.front());
	}
}

void ObjectTreeView::expandAllParentsOfObject(const QModelIndex &index) {
	for (auto parentIndex = index.parent(); parentIndex.isValid(); parentIndex = parentIndex.parent()) {
		if (!isExpanded(parentIndex)) {
			expand(parentIndex);
		}
	}
}

}  // namespace raco::object_tree::view