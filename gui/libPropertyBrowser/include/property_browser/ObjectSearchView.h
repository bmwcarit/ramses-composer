/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "common_widgets/NoContentMarginsLayout.h"
#include "components/DataChangeDispatcher.h"
#include "core/Iterators.h"
#include "core/Project.h"
#include "user_types/LuaScript.h"
#include <QGridLayout>
#include <QListView>
#include <QMimeData>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QWidget>

namespace raco::property_browser {

class ObjectSearchViewItem : public QStandardItem {
public:
	explicit ObjectSearchViewItem(const QString& s, const core::ValueHandle& handle);
	core::ValueHandle handle_;
};

class ObjectSearchModel final : public QStandardItemModel {
public:
	explicit ObjectSearchModel(QObject* parent);

	Qt::ItemFlags flags(const QModelIndex& index) const override;
	QStringList mimeTypes() const override;
	QMimeData* mimeData(const QModelIndexList& indexes) const override;
};

class ObjectSearchFilterProxyModel : public QSortFilterProxyModel {
	Q_OBJECT
public:
	ObjectSearchFilterProxyModel();

protected:
	bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
};

class ObjectSearchView : public QWidget {
	Q_OBJECT
public:
	explicit ObjectSearchView(components::SDataChangeDispatcher dispatcher, core::Project* project, const std::set<core::ValueHandle>& objects, QWidget* parent);

	Q_SLOT void setFilterByName(const QString& filter);

	Q_SIGNAL void clicked(const QModelIndex& index);
	Q_SIGNAL void activated(const QModelIndex& index);
	Q_SIGNAL void selectionChanged(bool valid);
	const core::ValueHandle& handleFromIndex(const QModelIndex& index) const;

	bool hasValidSelection() const noexcept;
	QModelIndex selection() const noexcept;

protected:
	virtual void rebuild() noexcept = 0;
	Q_SLOT void updateSelection() noexcept;
	void focusInEvent(QFocusEvent* event) override;

	common_widgets::NoContentMarginsLayout<QGridLayout> layout_;
	QListView list_;
	ObjectSearchModel model_;
	ObjectSearchFilterProxyModel filterModel_;
};

}  // namespace raco::property_browser