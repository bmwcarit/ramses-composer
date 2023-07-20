/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "TagContainerEditor_AppliedTagModel.h"
#include "core/TagDataCache.h"

#include <QMimeData>
#include <QWidget>

namespace raco::property_browser {

	TagContainerEditor_AppliedTagModel::TagContainerEditor_AppliedTagModel(QWidget* parent, raco::core::TagType tagType) : QStandardItemModel(parent), tagType_(tagType) {
		addTreeItemForAddTag();

		QObject::connect(this, &QStandardItemModel::itemChanged, this, &TagContainerEditor_AppliedTagModel::tagChanged);
		QObject::connect(this, &QStandardItemModel::rowsRemoved, [this]() { Q_EMIT tagListChanged(); });
		QObject::connect(this, &QStandardItemModel::rowsInserted, [this]() { Q_EMIT tagListChanged(); });
		
	}

	bool TagContainerEditor_AppliedTagModel::canDropMimeData(const QMimeData* data, Qt::DropAction, int, int, const QModelIndex&) const {
		if (data->hasText()) {
			const QString tag = data->text();
			return isTagAllowed(tag);
		}
		return false;
	}

	bool TagContainerEditor_AppliedTagModel::dropMimeData(const QMimeData* data, Qt::DropAction, int row, int, const QModelIndex&) {
		if (data->hasText()) {
			const QString tag = data->text();
			addTagInternal(tag, row);
			return true;
		}
		return false;
	}

	bool TagContainerEditor_AppliedTagModel::setData(const QModelIndex& indexToSet, const QVariant& value, int role) {
		if (role != Qt::EditRole) {
			return QStandardItemModel::setData(indexToSet, value, role);
		}
		if (indexToSet.column() == 0) {
			QString const newTag = value.toString().trimmed();
			if (newTag == itemFromIndex(indexToSet)->text()) {
				return true;
			}
			// Avoid duplicate tags but allow empty ones which will be removed in tagChanged.
			if (!newTag.isEmpty() && !isTagAllowed(newTag)) {
				return false;
			}			
		}
		return QStandardItemModel::setData(indexToSet, value, role);
	}

	Qt::ItemFlags TagContainerEditor_AppliedTagModel::flags(const QModelIndex& index) const {
		if (!index.isValid()) {
			return QStandardItemModel::flags(index); // needed because the root can accepts drops
		}
		if (index.row() == rowCount() - 1) {
			if (index.column() == 1) {
				return Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;
			} else {
				return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;				
			}
		}
		if (orderIsReadOnly_ && index.column() == 1) {
			return Qt::ItemIsSelectable | Qt::ItemNeverHasChildren;
		} else {
			return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemNeverHasChildren;
		}
	}

	void TagContainerEditor_AppliedTagModel::setOrderIsReadOnly(bool readonly) {
		orderIsReadOnly_ = readonly;
	}

	int TagContainerEditor_AppliedTagModel::rowForTag(QString const& s) const {
		for (int i = 0; i < rowCount(); ++i) {
			if (tagForRow(i) == s) {
				return i;
			}
		}
		return -1;
	}

	bool TagContainerEditor_AppliedTagModel::containsTag(QString const& s) const {
		for (int i = 0; i < rowCount(); ++i) {
			if (tagForRow(i) == s) {
				return true;
			}
		}
		return false;
	}

	QStandardItem* TagContainerEditor_AppliedTagModel::addTag(std::string const& tag, int row, std::optional<int> orderIndex) {
		return addTagInternal(QString::fromStdString(tag), row, orderIndex);
	}

	QString TagContainerEditor_AppliedTagModel::tagForRow(int row) const {
		return data(index(row, 0)).toString();
	}

	int TagContainerEditor_AppliedTagModel::orderIndexForRow(int row) const {
		if (data(index(row, 1)).toString() == "Multiple values") {
			return 0;
		}
		return data(index(row, 1)).toInt();
	}

	int TagContainerEditor_AppliedTagModel::orderIndexForLastRow() const {
		return rowCount() > 1 ? orderIndexForRow(rowCount() - 2) : 0;
	}

	QStandardItem* TagContainerEditor_AppliedTagModel::addTagInternal(QString tag, int row, std::optional<int> orderIndex) {
		if (!isTagAllowed(tag)) return nullptr;
		auto* treeWidgetItem = new QStandardItem(tag);
		row = row >= 0 ? std::min(row, rowCount()-1) : (rowCount() - 1);
		if (tagType_ == core::TagType::NodeTags_Referencing) {
			if (!orderIndex.has_value()) {
				if (row == rowCount() - 1) {
					orderIndex = orderIndexForLastRow();
				} else {
					orderIndex = orderIndexForRow(row);
				}
				auto* orderIndexItem = new QStandardItem(QString::fromStdString(std::to_string(orderIndex.value())));
				insertRow(row, {treeWidgetItem, orderIndexItem});
			} else if (orderIndex == INT_MIN) {
				insertRow(row, {treeWidgetItem,
					new QStandardItem("Multiple values")});
			} else {
				insertRow(row, {treeWidgetItem,
					new QStandardItem(QString::fromStdString(std::to_string(orderIndex.value())))});
			}
		} else {
			insertRow(row, treeWidgetItem);			
		}
		return treeWidgetItem;
	}

	QStandardItem* TagContainerEditor_AppliedTagModel::addTreeItemForAddTag() {
		auto* treeWidgetItem = new QStandardItem(addItemText_);
		if (tagType_ == raco::core::TagType::NodeTags_Referencing) {
			auto* orderIndexItem = new QStandardItem();
			appendRow({treeWidgetItem, orderIndexItem});
		} else {
			appendRow(treeWidgetItem);			
		}
		addTagWidgetItem_ = treeWidgetItem;
		return treeWidgetItem;
	}

	bool TagContainerEditor_AppliedTagModel::isAddTagItem(QStandardItem* item) const {
		return item == addTagWidgetItem_;
	}

	void TagContainerEditor_AppliedTagModel::tagChanged(QStandardItem* item) {
		if (isAddTagItem(item) && item->text() != addItemText_) {
			if (tagType_ == raco::core::TagType::NodeTags_Referencing) {				
				setData(index(rowCount() - 1, 1), QVariant::fromValue(orderIndexForLastRow()), Qt::DisplayRole);
			}
			addTreeItemForAddTag();
		}
		QString newTag = item->text();
		newTag = newTag.trimmed();
		if (newTag.isEmpty()) {
			removeRow(item->row());
		}
		Q_EMIT tagListChanged();
	}

	bool TagContainerEditor_AppliedTagModel::isTagAllowed(QString const& s) const {
		return !s.isEmpty() && !containsTag(s) && forbiddenTags_.find(s.toStdString()) == forbiddenTags_.end();
	}

	std::vector<std::pair<std::string, int>> TagContainerEditor_AppliedTagModel::renderableTags() const {
		std::vector<std::pair<std::string, int>> sortedTags;
		if (tagType_ == raco::core::TagType::NodeTags_Referencing) {
			for (int i = 0; i < rowCount(); ++i) {
				if (isAddTagItem(item(i, 0))) continue;
				std::string tagName = tagForRow(i).toStdString();
				int orderIndex = orderIndexForRow(i);
				sortedTags.insert(
					std::upper_bound(sortedTags.begin(), sortedTags.end(), orderIndex, [](int oi, auto const& p) { return oi < p.second; }),
					{tagName, orderIndex});
			}
		}
		return sortedTags;
	}

}