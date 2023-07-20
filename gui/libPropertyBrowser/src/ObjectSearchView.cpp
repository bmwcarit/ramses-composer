/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/ObjectSearchView.h"
#include "core/Queries.h"
#include "common_widgets/RaCoClipboard.h"

namespace raco::property_browser {

ObjectSearchViewItem::ObjectSearchViewItem(const QString& s, const core::ValueHandle& handle) : QStandardItem{s}, handle_{handle} {
	setDragEnabled(true);

	setToolTip(QString::fromStdString(core::Queries::getFullObjectHierarchyPath(handle_.rootObject())));
}

ObjectSearchModel::ObjectSearchModel(QObject* parent) : QStandardItemModel{parent} {}

Qt::ItemFlags ObjectSearchModel::flags(const QModelIndex& index) const {
	Qt::ItemFlags defaultFlags = QStandardItemModel::flags(index);

	if (index.isValid()) {
		return Qt::ItemIsDragEnabled | defaultFlags;
	} else {
		return defaultFlags;
	}
}

QStringList ObjectSearchModel::mimeTypes() const {
	return {"text/plain"};
}

QMimeData* ObjectSearchModel::mimeData(const QModelIndexList& indexes) const {
	auto* data{new QMimeData{}};
	auto* item{itemFromIndex(indexes.at(0))};
	if (auto viewItem{dynamic_cast<ObjectSearchViewItem*>(item)}) {
		auto objectId{viewItem->handle_.rootObject()->objectID()};
		auto propPath{viewItem->handle_.getPropertyPath()};
		data->setData(MimeTypes::EDITOR_OBJECT_ID, objectId.c_str());
		data->setData(MimeTypes::VALUE_HANDLE_PATH, propPath.c_str());
		data->setText(fmt::format("{}#{}", objectId.c_str(), propPath.c_str()).c_str());
	}
	return data;
}

ObjectSearchFilterProxyModel::ObjectSearchFilterProxyModel() : QSortFilterProxyModel{} {}

bool ObjectSearchFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
	QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
	return sourceModel()->data(index).toString().contains(filterRegExp());
}

ObjectSearchView::ObjectSearchView(components::SDataChangeDispatcher dispatcher, core::Project* project, const std::set<core::ValueHandle>& objects, QWidget* parent)
	: QWidget{parent},
	  layout_{this},
	  list_{this},
	  model_{&list_},
	  filterModel_{} {
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
}

const core::ValueHandle& ObjectSearchView::handleFromIndex(const QModelIndex& index) const {
	return (dynamic_cast<const ObjectSearchViewItem*>(model_.itemFromIndex(index)))->handle_;
}

void ObjectSearchView::setFilterByName(const QString& filter) {
	filterModel_.setFilterRegExp(QRegExp(filter, Qt::CaseInsensitive, QRegExp::Wildcard));
	updateSelection();
}

void ObjectSearchView::updateSelection() noexcept {
	if (filterModel_.rowCount() == 1) {
		list_.setCurrentIndex(filterModel_.index(0, 0));
	} else {
		list_.setCurrentIndex({});
	}
}

bool ObjectSearchView::hasValidSelection() const noexcept {
	return list_.currentIndex().isValid() && model_.itemFromIndex(selection())->isEnabled();
}

void ObjectSearchView::focusInEvent(QFocusEvent* event) {
	if (filterModel_.rowCount() > 0) {
		list_.setFocus();
		list_.setCurrentIndex(filterModel_.index(0, 0));
	} else {
		list_.setFocus();
	}
	QWidget::focusInEvent(event);
}

QModelIndex ObjectSearchView::selection() const noexcept {
	return filterModel_.mapToSource(list_.currentIndex());
}

}  // namespace raco::property_browser