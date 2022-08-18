/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "common_widgets/LinkStartSearchView.h"

namespace raco::common_widgets {

LinkStartViewItem::LinkStartViewItem(const QString& s, const core::ValueHandle& handle, bool allowedStrong, bool allowedWeak) : QStandardItem{s}, handle_{handle}, allowedStrong_(allowedStrong), allowedWeak_(allowedWeak) {
	setDragEnabled(true);

	setToolTip(QString::fromStdString(handle_.getDescriptor().getFullPropertyPath()));
}

LinkStartItemModel::LinkStartItemModel(QObject* parent) : QStandardItemModel{parent} {}

Qt::ItemFlags LinkStartItemModel::flags(const QModelIndex& index) const {
	Qt::ItemFlags defaultFlags = QStandardItemModel::flags(index);

	if (index.isValid()) {
		return Qt::ItemIsDragEnabled | defaultFlags;
	} else {
		return defaultFlags;
	}
}

QStringList LinkStartItemModel::mimeTypes() const {
	return {"text/plain"};
}

QMimeData* LinkStartItemModel::mimeData(const QModelIndexList& indexes) const {
	auto* data{new QMimeData{}};
	auto* item{itemFromIndex(indexes.at(0))};
	if (auto viewItem{dynamic_cast<LinkStartViewItem*>(item)}) {
		auto objectId{viewItem->handle_.rootObject()->objectID()};
		auto propPath{viewItem->handle_.getPropertyPath()};
		data->setData(MimeTypes::EDITOR_OBJECT_ID, objectId.c_str());
		data->setData(MimeTypes::VALUE_HANDLE_PATH, propPath.c_str());
		data->setText(fmt::format("{}#{}", objectId.c_str(), propPath.c_str()).c_str());
	}
	return data;
}

LinkStartItemFilterModel::LinkStartItemFilterModel() : QSortFilterProxyModel{} {}

bool LinkStartItemFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
	QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
	return sourceModel()->data(index).toString().contains(filterRegExp());
}

LinkStartSearchView::LinkStartSearchView(components::SDataChangeDispatcher dispatcher, core::Project* project, const core::ValueHandle& end, QWidget* parent)
	: QWidget{parent},
	  project_{project},
	  end_{end},
	  layout_{this},
	  list_{this},
	  model_{&list_},
	  filterModel_{},
	  projectChanges_{dispatcher->registerOnObjectsLifeCycle(
		  [this, dispatcher](raco::core::SEditorObject obj) {
            if (obj->isType<raco::user_types::LuaScript>()) {
                outputsChanges_[obj] = dispatcher->registerOnPreviewDirty(obj, [this]() {
                    rebuild();
                });
                rebuild();
            } },
		  [this](raco::core::SEditorObject obj) {
			  if (outputsChanges_.find(obj) != outputsChanges_.end()) {
				  outputsChanges_.erase(outputsChanges_.find(obj));
				  rebuild();
			  }
		  })} {
	layout_.addWidget(&list_);
	filterModel_.setSourceModel(&model_);
	list_.setModel(&filterModel_);
	list_.setDragEnabled(true);
	list_.setDragDropMode(QAbstractItemView::DragDropMode::DragOnly);
	list_.setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
	list_.setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);

	QObject::connect(&list_, &QListView::clicked, this, [this](const QModelIndex& index) {
		if (hasValidSelection())
			Q_EMIT clicked(filterModel_.mapToSource(index));
	});
	QObject::connect(&list_, &QListView::activated, this, [this](const QModelIndex& index) {
		if (hasValidSelection())
			Q_EMIT activated(filterModel_.mapToSource(index));
	});
	QObject::connect(list_.selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex& selected, const QModelIndex& deselected) {
		Q_EMIT selectionChanged(hasValidSelection());
	});
	rebuild();
	updateSelection();
}

const core::ValueHandle& LinkStartSearchView::handleFromIndex(const QModelIndex& index) const {
	return (dynamic_cast<const LinkStartViewItem*>(model_.itemFromIndex(index)))->handle_;
}

bool LinkStartSearchView::allowedStrong(const QModelIndex& index) const {
	return (dynamic_cast<const LinkStartViewItem*>(model_.itemFromIndex(index)))->allowedStrong_;
}

bool LinkStartSearchView::allowedWeak(const QModelIndex& index) const {
	return (dynamic_cast<const LinkStartViewItem*>(model_.itemFromIndex(index)))->allowedWeak_;
}

void LinkStartSearchView::rebuild() noexcept {
	model_.clear();
	for (const auto& [handle, strong, weak] : core::Queries::allLinkStartProperties(*project_, end_)) {
		QString title{handle.getPropertyPath().c_str()};
		if (weak && !strong) {
			title.append(" (weak)");
		}
		auto* item{new LinkStartViewItem{title, handle, strong, weak}};
		model_.appendRow(item);
	}
}

void LinkStartSearchView::setFilterByName(const QString& filter) {
	filterModel_.setFilterRegExp(QRegExp(filter, Qt::CaseInsensitive, QRegExp::Wildcard));
	updateSelection();
}

void LinkStartSearchView::updateSelection() noexcept {
	if (filterModel_.rowCount() == 1) {
		list_.setCurrentIndex(filterModel_.index(0, 0));
	} else {
		list_.setCurrentIndex({});
	}
}

bool LinkStartSearchView::hasValidSelection() const noexcept {
	return list_.currentIndex().isValid() && model_.itemFromIndex(selection())->isEnabled();
}

void LinkStartSearchView::focusInEvent(QFocusEvent* event) {
	if (filterModel_.rowCount() > 0) {
		list_.setFocus();
		list_.setCurrentIndex(filterModel_.index(0, 0));
	} else {
		list_.setFocus();
	}
	QWidget::focusInEvent(event);
}

QModelIndex LinkStartSearchView::selection() const noexcept {
	return filterModel_.mapToSource(list_.currentIndex());
}

}  // namespace raco::common_widgets
