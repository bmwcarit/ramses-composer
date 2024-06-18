/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "object_tree_view/ObjectTreeView.h"

#include "ObjectTreeFilter.h"

#include "components/RaCoPreferences.h"
#include "core/EditorObject.h"
#include "core/PathManager.h"
#include "core/Project.h"
#include "user_types/Node.h"
#include "user_types/AnimationChannel.h"

#include "object_tree_view_model/ObjectTreeViewDefaultModel.h"
#include "object_tree_view_model/ObjectTreeViewExternalProjectModel.h"
#include "object_tree_view_model/ObjectTreeViewPrefabModel.h"
#include "object_tree_view_model/ObjectTreeViewRenderViewModel.h"
#include "object_tree_view_model/ObjectTreeViewResourceModel.h"
#include "object_tree_view_model/ObjectTreeViewSortProxyModels.h"
#include "utils/u8path.h"

#include <QContextMenuEvent>
#include <QFileDialog>
#include <QGuiApplication>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QProcess>
#include <QShortcut>
#include <QSortFilterProxyModel>

namespace raco::object_tree::view {

using namespace object_tree::model;

ObjectTreeView::ObjectTreeView(const QString &viewTitle, ObjectTreeViewDefaultModel *viewModel, ObjectTreeViewDefaultSortFilterProxyModel *sortFilterProxyModel, QWidget *parent)
	: QTreeView(parent), treeModel_(viewModel), proxyModel_(sortFilterProxyModel), viewTitle_(viewTitle) {
	setAlternatingRowColors(true);
	setContextMenuPolicy(Qt::CustomContextMenu);

	setDragDropMode(QAbstractItemView::DragDrop);
	setDragEnabled(true);
	setDropIndicatorShown(true);
	setSelectionMode(QAbstractItemView::ExtendedSelection);
	viewport()->setAcceptDrops(true);

	if (proxyModel_) {
		proxyModel_->setSourceModel(treeModel_);
		
		setSortingEnabled(proxyModel_->sortingEnabled());
		sortByColumn(treeModel_->defaultSortColumn(), Qt::AscendingOrder);
		QTreeView::setModel(proxyModel_);
	} else {
		QTreeView::setModel(treeModel_);
	}

	setTextElideMode(treeModel_->textElideMode());


	connect(this, &QTreeView::customContextMenuRequested, this, &ObjectTreeView::showContextMenu);
	connect(this, &QTreeView::expanded, this, &ObjectTreeView::expanded);
	connect(this, &QTreeView::collapsed, this, &ObjectTreeView::collapsed);

	connect(this->selectionModel(), &QItemSelectionModel::selectionChanged, [this](const auto &selectedItemList, const auto &deselectedItemList) {
		Q_EMIT newObjectTreeItemsSelected(getSelectedObjects());
	});

	connect(treeModel_, &ObjectTreeViewDefaultModel::modelAboutToBeReset, this, &ObjectTreeView::saveItemExpansionStates);
	connect(treeModel_, &ObjectTreeViewDefaultModel::modelReset, this, &ObjectTreeView::restoreItemExpansionStates);
	connect(treeModel_, &ObjectTreeViewDefaultModel::modelAboutToBeReset, this, &ObjectTreeView::saveItemSelectionStates);
	connect(treeModel_, &ObjectTreeViewDefaultModel::modelReset, this, &ObjectTreeView::restoreItemSelectionStates);

	resizeColumnToContents(ObjectTreeViewDefaultModel::COLUMNINDEX_PREVIEW_VISIBILITY);
	resizeColumnToContents(ObjectTreeViewDefaultModel::COLUMNINDEX_ABSTRACT_VIEW_VISIBILITY);
	setColumnWidth(ObjectTreeViewDefaultModel::COLUMNINDEX_NAME, width() / 3);

	// hidden column for data only used for filtering, enable to reveal object IDs
	setColumnHidden(ObjectTreeViewDefaultModel::COLUMNINDEX_ID, true);
	setColumnHidden(ObjectTreeViewDefaultModel::COLUMNINDEX_USERTAGS, true);
	for (auto column : treeModel_->hiddenColumns()) {
		setColumnHidden(column, true);
	}
	
	auto copyShortcut = new QShortcut(QKeySequence::Copy, this, nullptr, nullptr, Qt::WidgetShortcut);
	QObject::connect(copyShortcut, &QShortcut::activated, this, &ObjectTreeView::copyObjects);

	auto pasteShortcut = new QShortcut(QKeySequence::Paste, this, nullptr, nullptr, Qt::WidgetShortcut);
	QObject::connect(pasteShortcut, &QShortcut::activated, this, [this]() {
		pasteObjects(getSelectedInsertionTargetIndex());
	});

	auto cutShortcut = new QShortcut(QKeySequence::Cut, this, nullptr, nullptr, Qt::WidgetShortcut);
	QObject::connect(cutShortcut, &QShortcut::activated, this, &ObjectTreeView::cutObjects);
	auto deleteShortcut = new QShortcut({"Del"}, this, nullptr, nullptr, Qt::WidgetShortcut);
	QObject::connect(deleteShortcut, &QShortcut::activated, this, &ObjectTreeView::deleteObjects);

	auto duplicateShortcut = new QShortcut({"Ctrl+D"}, this, nullptr, nullptr, Qt::WidgetShortcut);
	QObject::connect(duplicateShortcut, &QShortcut::activated, this, &ObjectTreeView::duplicateObjects);

	auto isolateShortcut = new QShortcut({"Ctrl+I"}, this, nullptr, nullptr, Qt::WidgetShortcut);
	QObject::connect(isolateShortcut, &QShortcut::activated, this, &ObjectTreeView::isolateNodes);

	auto hideShortcut = new QShortcut({"Ctrl+H"}, this, nullptr, nullptr, Qt::WidgetShortcut);
	QObject::connect(hideShortcut, &QShortcut::activated, this, &ObjectTreeView::hideNodes);

	auto showAllShortcut = new QShortcut({"Ctrl+U"}, this, nullptr, nullptr, Qt::WidgetShortcut);
	QObject::connect(showAllShortcut, &QShortcut::activated, this, &ObjectTreeView::showAllNodes);

	auto extrefPasteShortcut = new QShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_V, this, nullptr, nullptr, Qt::WidgetShortcut);
	QObject::connect(extrefPasteShortcut, &QShortcut::activated, this, &ObjectTreeView::pasteAsExtRef);
}

core::SEditorObjectSet ObjectTreeView::getSelectedObjects() const {
	auto selectedObjects = indicesToSEditorObjects(selectionModel()->selectedIndexes());
	return core::SEditorObjectSet(selectedObjects.begin(), selectedObjects.end());
}

namespace {
	int getTotalParentCount(const QModelIndex &a) {
		auto current = a;
		auto depth = 0;
		while (current.parent().isValid()) {
			current = current.parent();
			depth++;
		}

		return depth;
	}

	bool compareModelIndexesInsideTreeView(const QModelIndex &a, const QModelIndex &b) {
		if (a.parent() == b.parent()) {
			if (a.row() == b.row()) {
				// For some reason there are 4 entries per column (?) those need to have a fixed order too.
				return a.column() < b.column();
			}

			return a.row() < b.row();
		}

		if (a == b.parent()) {
			return true;
		}

		if (a.parent() == b) {
			return false;
		}

		int aParentCount = getTotalParentCount(a);
		int bParentCount = getTotalParentCount(b);

		if (aParentCount > bParentCount) {
			return compareModelIndexesInsideTreeView(a.parent(), b);
		}
		if (aParentCount < bParentCount) {
			return compareModelIndexesInsideTreeView(a, b.parent());
		}

		return compareModelIndexesInsideTreeView(
			a.parent().isValid() ? a.parent() : a,
			b.parent().isValid() ? b.parent() : b);
	}
}  // namespace

std::vector<core::SEditorObject> ObjectTreeView::getSortedSelectedEditorObjects() const {
	QList<QModelIndex> selectedIndexes = selectionModel()->selectedIndexes();
	std::sort(selectedIndexes.begin(), selectedIndexes.end(), compareModelIndexesInsideTreeView);
	auto selectedObjects = indicesToSEditorObjects(selectedIndexes);

	std::vector<SEditorObject> result;
	for (const auto &item : selectedObjects){
		if (result.end() == std::find(result.begin(), result.end(), item)){
			result.push_back(item);
		}
	}

	return result;
}

void ObjectTreeView::copyObjects() {
	auto selectedIndices = getSelectedIndices(true);
	if (!selectedIndices.empty()) {
		if (canCopyAtIndices(selectedIndices)) {
			treeModel_->copyObjectsAtIndices(selectedIndices, false);
		}
	}
}

void ObjectTreeView::pasteObjects(const QModelIndex &index, bool asExtRef) {
	if (canPasteIntoIndex(index, asExtRef)) {
		treeModel_->pasteObjectAtIndex(index, asExtRef);
	} else if (canPasteIntoIndex({}, asExtRef)) {
		treeModel_->pasteObjectAtIndex({}, asExtRef);
	}
}

void ObjectTreeView::cutObjects() {
	auto selectedIndices = getSelectedIndices(true);
	if (!selectedIndices.isEmpty() && canCutIndices(selectedIndices)) {
		treeModel_->cutObjectsAtIndices(selectedIndices, false);
	}
}

void ObjectTreeView::deleteObjects() {
	auto selectedIndices = getSelectedIndices();
	if (!selectedIndices.empty() && treeModel_->canDeleteAtIndices(selectedIndices)) {
		auto delObjAmount = treeModel_->deleteObjectsAtIndices(selectedIndices);

		if (delObjAmount > 0) {
			selectionModel()->Q_EMIT selectionChanged({}, {});
		}
	}
}

void object_tree::view::ObjectTreeView::duplicateObjects() {
	auto selectedIndices = getSelectedIndices(true);
	if (!selectedIndices.isEmpty() && treeModel_->canDuplicateAtIndices(selectedIndices)) {
		treeModel_->duplicateObjectsAtIndices(selectedIndices);
	}
}

void ObjectTreeView::isolateNodes() {
	auto selectedIndices = getSelectedIndices(true);
	if (!selectedIndices.isEmpty()) {
		treeModel_->isolateNodes(selectedIndices);
	}
}

void ObjectTreeView::hideNodes() {
	auto selectedIndices = getSelectedIndices(true);
	if (!selectedIndices.isEmpty()) {
		treeModel_->hideNodes(selectedIndices);
	}
}

void ObjectTreeView::pasteAsExtRef() {
	std::string error;
	if (!treeModel_->pasteObjectAtIndex({}, true, &error)) {
		QMessageBox::warning(this, "Paste As External Reference",
			fmt::format("Update of pasted external references failed!\n\n{}", error).c_str());
	}
}

void ObjectTreeView::showAllNodes() {
	treeModel_->showAllNodes();
}

void ObjectTreeView::selectObject(const QString &objectID, bool blockSignals) {
	selectionModel()->blockSignals(blockSignals);
	if (objectID.isEmpty()) {
		resetSelection();
	} else {
		auto objectIndex = indexFromTreeNodeID(objectID.toStdString());
		if (objectIndex.isValid()) {
			resetSelection();
			selectionModel()->select(objectIndex, SELECTION_MODE);
			scrollTo(objectIndex);
		}
	}
	selectionModel()->blockSignals(false);
}

void ObjectTreeView::expandAllParentsOfObject(const QString &objectID) {
	auto objectIndex = indexFromTreeNodeID(objectID.toStdString());
	if (objectIndex.isValid()) {
		expandAllParentsOfObject(objectIndex);
	}
}

void ObjectTreeView::expanded(const QModelIndex &index) {
	if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::KeyboardModifier::ShiftModifier)) {
		expandRecursively(index);
	}
}

void ObjectTreeView::collapsed(const QModelIndex &index) {
	if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::KeyboardModifier::ShiftModifier)) {
		collapseRecusively(index);
	}
}

void ObjectTreeView::collapseRecusively(const QModelIndex& index) {
	collapse(index);
	
	for (int i = 0; i < index.model()->rowCount(index); ++i) {
		collapseRecusively(index.model()->index(i, 0, index));
	}
}

object_tree::model::ObjectTreeViewDefaultModel *ObjectTreeView::treeModel() const {
	return treeModel_;
}

QString ObjectTreeView::getViewTitle() const {
	return viewTitle_;
}

void ObjectTreeView::requestNewNode(const std::string& nodeType, const std::string &nodeName, const QModelIndex &parent) {
	auto createdObject = treeModel_->createNewObject(nodeType, nodeName, parent);
	selectObject(QString::fromStdString(createdObject->objectID()), false);
}

void ObjectTreeView::showContextMenu(const QPoint &p) {
	auto *treeViewMenu = createCustomContextMenu(p);
	if (treeViewMenu) {
		treeViewMenu->exec(viewport()->mapToGlobal(p));
	}
}

bool ObjectTreeView::canCopyAtIndices(const QModelIndexList &parentIndices) {
	return treeModel_->canCopyAtIndices(parentIndices);
}

bool ObjectTreeView::canCutIndices(const QModelIndexList &indices) {
	return treeModel_->canCopyAtIndices(indices) && treeModel_->canDeleteAtIndices(indices);
}

bool ObjectTreeView::canPasteIntoIndex(const QModelIndex &index, bool asExtref) {
	if (!RaCoClipboard::hasEditorObject()) {
		return false;
	} else {
		auto [pasteObjects, sourceProjectTopLevelObjectIds] = treeModel_->getObjectsAndRootIdsFromClipboardString(RaCoClipboard::get());
		return treeModel_->canPasteIntoIndex(index, pasteObjects, sourceProjectTopLevelObjectIds, asExtref);
	}

}

bool ObjectTreeView::canProgrammaticallyGoToObject() {
	return treeModel_->canProgramaticallyGotoObject();
}

bool ObjectTreeView::containsObject(const QString &objectID) const {
	return treeModel_->indexFromTreeNodeID(objectID.toStdString()).isValid();
}

void ObjectTreeView::setFilterKeyColumn(int column) {
	if (proxyModel_) {
		proxyModel_->setFilterKeyColumn(column);
	}
}

FilterResult ObjectTreeView::filterObjects(const QString &filterString) {
	FilterResult filterResult = filterString.isEmpty() ? FilterResult::Success : FilterResult::Failed;

	if (proxyModel_) {
		proxyModel_->removeCustomFilter();

		if (!filterString.isEmpty()) {
			const auto input = filterString.toStdString();
			ObjectTreeFilter objectTreeFilter;
			objectTreeFilter.setDefaultFilterKeyColumn(proxyModel_->filterKeyColumn());
			const auto filterFunction = objectTreeFilter.parse(input, filterResult);

			if (filterFunction && filterResult != FilterResult::Failed) {
				proxyModel_->setCustomFilter(filterFunction);
				restoreItemExpansionStates();
				viewport()->update();
			}
		}
	}
	return filterResult;
}

bool ObjectTreeView::hasProxyModel() const {
	return proxyModel_ != nullptr;
}

void ObjectTreeView::resetSelection() {
	selectionModel()->reset();
	viewport()->update();
}

QMenu* ObjectTreeView::createCustomContextMenu(const QPoint &p) {
	if (dynamic_cast<ObjectTreeViewRenderViewModel *>(treeModel_)) {
		return nullptr;
	}

	auto treeViewMenu = new QMenu(this);

	auto selectedItemIndices = getSelectedIndices(true);
	auto insertionTargetIndex = getSelectedInsertionTargetIndex();
	
	bool canDeleteSelectedIndices = treeModel_->canDeleteAtIndices(selectedItemIndices);
	bool canCopySelectedIndices = treeModel_->canCopyAtIndices(selectedItemIndices);
	bool canCutSelectedIndices = canCutIndices(selectedItemIndices);

	auto externalProjectModel = (dynamic_cast<ObjectTreeViewExternalProjectModel *>(treeModel_));
	auto prefabModel = (dynamic_cast<ObjectTreeViewPrefabModel *>(treeModel_));
	auto resourceModel = (dynamic_cast<ObjectTreeViewResourceModel *>(treeModel_));
	bool isScenegraphView = !(prefabModel || resourceModel || externalProjectModel);
	bool canInsertMeshAsset = false;
	bool haveCreatableTypes = false;
	bool canConvAnimChannel = false;

	for (auto const& typeName : treeModel_->creatableTypes(insertionTargetIndex)) {
		auto actionCreate = treeViewMenu->addAction(QString::fromStdString("Create " + typeName), [this, typeName, insertionTargetIndex]() {
			requestNewNode(typeName, "", insertionTargetIndex);
		});
		haveCreatableTypes = true;
		if (typeName == user_types::Node::typeDescription.typeName) {
			canInsertMeshAsset = true;
		}
		if (typeName == user_types::AnimationChannel::typeDescription.typeName) {
			canConvAnimChannel = true;
		}
	}

	if (canInsertMeshAsset) {
		treeViewMenu->addSeparator();

		auto actionImport = treeViewMenu->addAction("Import glTF Assets...", [this, insertionTargetIndex]() {
			auto sceneFolder = core::PathManager::getCachedPath(core::PathManager::FolderTypeKeys::Mesh, treeModel_->project()->currentFolder());
			auto file = QFileDialog::getOpenFileName(this, "Load Asset File", QString::fromStdString(sceneFolder.string()), "glTF files (*.gltf *.glb);; All files (*.*)");
			if (!file.isEmpty()) {
				treeModel_->importMeshScenegraph(file, insertionTargetIndex);
			}
		});
		actionImport->setEnabled(canInsertMeshAsset);
	}

	if (canConvAnimChannel) {
		treeViewMenu->addSeparator();
		auto actionConvert = treeViewMenu->addAction(
			"Convert AnimationChannel to AnimationChannelRaco", [this, selectedItemIndices] {
				treeModel_->convertToAnimationChannelRaco(selectedItemIndices);
			});
		actionConvert->setEnabled(treeModel_->canConvertAnimationChannels(selectedItemIndices));
	}

	if (!externalProjectModel || haveCreatableTypes) {
		treeViewMenu->addSeparator();
	}

	auto actionDelete = treeViewMenu->addAction(
		"Delete", [this, selectedItemIndices]() {
			treeModel_->deleteObjectsAtIndices(selectedItemIndices);
			selectionModel()->Q_EMIT selectionChanged({}, {});
		},
		QKeySequence::Delete);
	actionDelete->setEnabled(canDeleteSelectedIndices);

	auto actionCopy = treeViewMenu->addAction(
		"Copy", [this, selectedItemIndices]() { 
		treeModel_->copyObjectsAtIndices(selectedItemIndices, false); 
	}, QKeySequence::Copy);
	actionCopy->setEnabled(canCopySelectedIndices);

	auto [pasteObjects, sourceProjectTopLevelObjectIds] = treeModel_->getObjectsAndRootIdsFromClipboardString(RaCoClipboard::get());
	QAction* actionPaste;
	if (treeModel_->canPasteIntoIndex(insertionTargetIndex, pasteObjects, sourceProjectTopLevelObjectIds)) {
		actionPaste = treeViewMenu->addAction(
			"Paste Here", [this, insertionTargetIndex]() { 
				std::string error;
				if (!treeModel_->pasteObjectAtIndex(insertionTargetIndex, false, &error)) {
					QMessageBox::warning(this, "Paste Failed", fmt::format("Paste failed:\n\n{}", error).c_str());
				}
			}, QKeySequence::Paste);
	} else if (treeModel_->canPasteIntoIndex({}, pasteObjects, sourceProjectTopLevelObjectIds)) {
		actionPaste = treeViewMenu->addAction(
			"Paste Into Project", [this]() { 
				std::string error;
				if (!treeModel_->pasteObjectAtIndex(QModelIndex(), false, &error)) {
					QMessageBox::warning(this, "Paste Failed", fmt::format("Paste failed:\n\n{}", error).c_str());
				}
			}, QKeySequence::Paste);
	} else {
		actionPaste = treeViewMenu->addAction("Paste", [](){}, QKeySequence::Paste);
		actionPaste->setEnabled(false);
	}

	auto actionDuplicate = treeViewMenu->addAction(
		"Duplicate", [this, selectedItemIndices] {
			treeModel_->duplicateObjectsAtIndices(selectedItemIndices);
		},
		QKeySequence("Ctrl+D"));
	actionDuplicate->setEnabled(treeModel_->canDuplicateAtIndices(selectedItemIndices));

	auto actionCut = treeViewMenu->addAction(
		"Cut", [this, selectedItemIndices]() {
		treeModel_->cutObjectsAtIndices(selectedItemIndices, false); 
	}, QKeySequence::Cut);
	actionCut->setEnabled(canCutSelectedIndices);

	treeViewMenu->addSeparator();

	auto actionCopyDeep = treeViewMenu->addAction("Copy (Deep)", [this, selectedItemIndices]() { 
		treeModel_->copyObjectsAtIndices(selectedItemIndices, true); 
	});
	actionCopyDeep->setEnabled(canCopySelectedIndices);

	auto actionCutDeep = treeViewMenu->addAction("Cut (Deep)", [this, selectedItemIndices]() { 		
		treeModel_->cutObjectsAtIndices(selectedItemIndices, true); 
	});
	actionCutDeep->setEnabled(canCutSelectedIndices);

	if (!externalProjectModel) {
		auto actionDeleteUnrefResources = treeViewMenu->addAction("Delete Unused Resources", [this] { treeModel_->deleteUnusedResources(); });
		actionDeleteUnrefResources->setEnabled(treeModel_->canDeleteUnusedResources());

		if (isScenegraphView) {
			treeViewMenu->addSeparator();
			 treeViewMenu->addAction("Isolate Subtree", [this] () {
					isolateNodes();
				},
				QKeySequence("Ctrl+I"));

			 treeViewMenu->addAction(
				"Hide Subtree", [this]() {
					hideNodes();
				},
				QKeySequence("Ctrl+H"));

			 treeViewMenu->addAction(
				"Show All Nodes", [this]() {
					showAllNodes();
				},
				QKeySequence("Ctrl+U"));
		}

		treeViewMenu->addSeparator();
		auto extrefPasteAction = treeViewMenu->addAction("Paste As External Reference", this, &ObjectTreeView::pasteAsExtRef);

		// Pasting extrefs will ignore the current selection and always paste on top level.
		extrefPasteAction->setEnabled(treeModel_->canPasteIntoIndex({}, pasteObjects, sourceProjectTopLevelObjectIds, true));
	}

	if (externalProjectModel) {
		treeViewMenu->addSeparator();
		treeViewMenu->addAction("Add Project...", [this, externalProjectModel]() {
			auto projectFile = QFileDialog::getOpenFileName(this, tr("Import Project"), components::RaCoPreferences::instance().userProjectsDirectory, tr("Ramses Composer Assembly (*.rca);; All files (*.*)"));
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

		auto actionCloseImportedProject = treeViewMenu->addAction("Remove Project", [this, selectedItemIndices, externalProjectModel]() { externalProjectModel->removeProjectsAtIndices(selectedItemIndices); });
		actionCloseImportedProject->setEnabled(externalProjectModel->canRemoveProjectsAtIndices(selectedItemIndices));
	}

	std::map<std::string, std::string> externalProjects;
	for (const auto &index : selectedItemIndices) {
		const auto path = treeModel_->externalProjectPathAtIndex(index);
		if (!path.empty() && externalProjects.find(path) == externalProjects.end()) {
			const auto id = treeModel_->objectIdAtIndex(index);
			if (!id.empty()) {
				externalProjects[path] = id;
			}
		}
	}

	const auto actionOpenProject = treeViewMenu->addAction("Open External Projects", [this, externalProjects]() {
		if (!externalProjects.empty()) {
			const auto appPath = QCoreApplication::applicationFilePath();
			for (const auto [path, id] : externalProjects) {
				QProcess::startDetached(appPath, {QString::fromStdString(path), "-i", QString::fromStdString(id)});
			}
		}
	});
	actionOpenProject->setEnabled(!externalProjects.empty());

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

std::vector<core::SEditorObject> ObjectTreeView::indicesToSEditorObjects(const QModelIndexList &indices) const {
	QModelIndexList itemIndices;
	if (proxyModel_) {
		for (const auto &index : indices) {
			itemIndices.append(proxyModel_->mapToSource(index));
		}
	} else {
		itemIndices = indices;
	}
	return treeModel_->indicesToSEditorObjects(itemIndices);
}

std::string ObjectTreeView::indexToTreeNodeID(const QModelIndex &index) const {
	auto itemIndex = index;
	if (proxyModel_) {
		itemIndex = proxyModel_->mapToSource(index);
	}
	return treeModel_->indexToTreeNode(itemIndex)->getID();
}

QModelIndex ObjectTreeView::indexFromTreeNodeID(const std::string &id) const {
	auto index = treeModel_->indexFromTreeNodeID(id);
	if (proxyModel_) {
		index = proxyModel_->mapFromSource(index);
	}

	return index;
}


QModelIndexList ObjectTreeView::getSelectedIndices(bool sorted) const {
	auto selectedIndices = selectionModel()->selectedRows();

	if (proxyModel_) {
		for (auto& index : selectedIndices) {
			index = proxyModel_->mapToSource(index);
		}
	}

	// For operation like copy, cut and move, the order of selection matters
	if (sorted) {
		auto sortedList = selectedIndices.toVector();
		std::sort(sortedList.begin(), sortedList.end(), ObjectTreeViewDefaultModel::isIndexAboveInHierachyOrPosition);
		return QModelIndexList(sortedList.begin(), sortedList.end());
	}

	return selectedIndices;
}

QModelIndex ObjectTreeView::getSelectedInsertionTargetIndex() const {
	auto selectedIndices = getSelectedIndices();

	// For single selection, just insert unter the selection.
	if (selectedIndices.count() == 0) {
		return {};
	} else if (selectedIndices.count() == 1) {
		return selectedIndices.front();
	}

	// For multiselection, look for the index on the highest hierachy level which is topmost in its hierachy level.
	// Partially sort selectedIndices so that the first index of the list is the highest level topmost index of the tree.
	std::nth_element(selectedIndices.begin(), selectedIndices.begin(), selectedIndices.end(), ObjectTreeViewDefaultModel::isIndexAboveInHierachyOrPosition);
	
	// Now take this highest level topmost index from the list and insert into its parent, if it exists.
	auto canidate = selectedIndices.front();
	if (canidate.isValid()) {
		return canidate.parent();		
	} else {
		return canidate;
	}
}


void ObjectTreeView::iterateThroughTree(QAbstractItemModel *model, std::function<void(QModelIndex &)> nodeFunc, QModelIndex currentIndex) {
	if (currentIndex.row() != -1) {
		nodeFunc(currentIndex);
	}
	for (int i = 0; i < model->rowCount(currentIndex); ++i) {
		auto childIndex = model->index(i, 0, currentIndex);
		iterateThroughTree(model, nodeFunc, childIndex);
	}
}

void ObjectTreeView::saveItemExpansionStates() {
	expandedItemIDs_.clear();
	iterateThroughTree(
		model(), [this](QModelIndex &index) {
			if (isExpanded(index)) {
				expandedItemIDs_.insert(indexToTreeNodeID(index));
			}
		},
		QModelIndex());
}

void ObjectTreeView::restoreItemExpansionStates() {
	for (const auto &expandedObjectID : expandedItemIDs_) {
		auto expandedObjectIndex = indexFromTreeNodeID(expandedObjectID);
		if (expandedObjectIndex.isValid()) {
			blockSignals(true);
			expand(expandedObjectIndex);
			blockSignals(false);
		}
	}
}

void ObjectTreeView::saveItemSelectionStates() {
	selectedItemIDs_.clear();
	for (auto obj : indicesToSEditorObjects(selectionModel()->selectedIndexes())) {
		selectedItemIDs_.insert(obj->objectID());
	}
}

void ObjectTreeView::restoreItemSelectionStates() {
	// This doesn't emit any signals:
	selectionModel()->reset();
	std::vector<QModelIndex> selectedObjects;

	QItemSelection selectedItems;
	auto selectionIt = selectedItemIDs_.begin();
	while (selectionIt != selectedItemIDs_.end()) {
		const auto &selectionID = *selectionIt;
		auto selectedObjectIndex = indexFromTreeNodeID(selectionID);
		if (selectedObjectIndex.isValid()) {
			selectedItems.select(selectedObjectIndex, selectedObjectIndex);
			selectedObjects.emplace_back(selectedObjectIndex);
			expandAllParentsOfObject(selectedObjectIndex);
			++selectionIt;
		} else {
			selectionIt = selectedItemIDs_.erase(selectionIt);
		}
	}

	if (!selectedObjects.empty()) {
		// This does emit a single selectionChanged() signal
		// TODO is it correct not to send any signal if the selection is now empty?
		selectionModel()->select(selectedItems, SELECTION_MODE);
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