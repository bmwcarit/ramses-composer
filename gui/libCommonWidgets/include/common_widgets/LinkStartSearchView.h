/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "common_widgets/NoContentMarginsLayout.h"
#include "common_widgets/RaCoClipboard.h"
#include "components/DataChangeDispatcher.h"
#include "core/Iterators.h"
#include "core/Project.h"
#include "core/Queries.h"
#include "user_types/LuaScript.h"
#include <QGridLayout>
#include <QListView>
#include <QMimeData>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QWidget>
#include <spdlog/fmt/fmt.h>

namespace raco::common_widgets {

class LinkStartViewItem final : public QStandardItem {
public:
	explicit LinkStartViewItem(const QString& s, const core::ValueHandle& handle);
	core::ValueHandle handle_;
};

class LinkStartItemModel final : public QStandardItemModel {
public:
	explicit LinkStartItemModel(QObject* parent);

	Qt::ItemFlags flags(const QModelIndex& index) const override;
	QStringList mimeTypes() const override;
	QMimeData* mimeData(const QModelIndexList& indexes) const override;
};

class LinkStartItemFilterModel : public QSortFilterProxyModel {
	Q_OBJECT
public:
	LinkStartItemFilterModel();

protected:
	bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
};

/**
 * Basic Widget to display all properties which have a LinkStartAnnotation.
 */
class LinkStartSearchView final : public QWidget {
	Q_OBJECT
public:
	explicit LinkStartSearchView(components::SDataChangeDispatcher dispatcher, core::Project* project, const core::ValueHandle& end, QWidget* parent);

	Q_SLOT void setFilterByName(const QString& filter);

	Q_SIGNAL void clicked(const QModelIndex& index);
	Q_SIGNAL void activated(const QModelIndex& index);
	Q_SIGNAL void selectionChanged(bool valid);
	const core::ValueHandle& handleFromIndex(const QModelIndex& index) const;
	bool hasValidSelection() const noexcept;
	QModelIndex selection() const noexcept;

protected:
	void rebuild() noexcept;
	Q_SLOT void updateSelection() noexcept;
	void focusInEvent(QFocusEvent* event) override;

private:
	core::Project* project_;
	core::ValueHandle end_;
	NoContentMarginsLayout<QGridLayout> layout_;
	QListView list_;
	LinkStartItemModel model_;
	LinkStartItemFilterModel filterModel_;
	raco::components::Subscription projectChanges_;
	std::map<raco::core::SEditorObject, raco::components::Subscription> outputsChanges_;
};

}  // namespace raco::common_widgets
