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

#include "data_storage/Table.h"

#include <QStandardItemModel>

#include <optional>
#include <set>

namespace raco::core {
enum class TagType;
}

namespace raco::property_browser {

class TagContainerEditor_AppliedTagModel : public QStandardItemModel {
		Q_OBJECT
	public:
		TagContainerEditor_AppliedTagModel(QWidget* parent, core::TagType tagType);

		bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const override;
		bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
		bool setData(const QModelIndex& index, const QVariant& value, int role) override;
		Qt::ItemFlags flags(const QModelIndex& index) const override;

		void setForbiddenTags(std::set<std::string> const& forbiddenTags) {
			forbiddenTags_ = forbiddenTags;
		}

		void setOrderIsReadOnly(bool readonly);

		int rowForTag(QString const& s) const;
		bool containsTag(QString const& s) const;
		QStandardItem* addTag(std::string const& tag, int row = -1, std::optional<int> orderIndex = std::optional<int>());
		bool isAddTagItem(QStandardItem* item) const;
		template <class TContainer = std::vector<std::string>>
		TContainer tags() const {
			TContainer newTags;
			for (int i = 0; i < rowCount(); ++i) {
				if (item(i) == nullptr || isAddTagItem(item(i))) continue;
				newTags.insert(newTags.end(), item(i)->text().toStdString());
			}
			return newTags;
		}

		std::vector<std::pair<std::string, int>> renderableTags() const;

		inline static QString const addItemText_{ "<add tag>" };

	Q_SIGNALS:
		void tagListChanged();

	private:
		QStandardItem* addTagInternal(QString tag, int row = -1, std::optional<int> orderIndex = std::optional<int>());
		QStandardItem* addTreeItemForAddTag();
		void tagChanged(QStandardItem* item);
		bool isTagAllowed(QString const& s) const;
		QString tagForRow(int row) const;
		int orderIndexForRow(int row) const;
		int orderIndexForLastRow() const;

		core::TagType const tagType_;
		QStandardItem* addTagWidgetItem_{ nullptr };
		std::set<std::string> forbiddenTags_;
		bool orderIsReadOnly_ = false;
	};

}
