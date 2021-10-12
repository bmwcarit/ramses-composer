/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "TagContainerEditor_AvailableTagsItemModel.h"

#include <QMimeData>

namespace raco::property_browser {

	void TagContainerEditor_AvailableTagsItemModel::setTagAllowed(QStandardItem* tagItem, bool isAllowed) {
		if (!isAllowed) {
			tagItem->setFlags(Qt::ItemNeverHasChildren);
		} else {
			tagItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsSelectable | Qt::ItemNeverHasChildren);
		}
	}

	void TagContainerEditor_AvailableTagsItemModel::addTag(QString const& tagName, QStringList const& renderLayerNames, bool isAllowed) {
		auto* tagItem = new QStandardItem(tagName);
		auto* renderLayerItem = new QStandardItem(renderLayerNames.join(", "));
		renderLayerItem->setToolTip(renderLayerNames.join("\n"));
		setTagAllowed(tagItem, isAllowed);
		renderLayerItem->setFlags(Qt::NoItemFlags | Qt::ItemNeverHasChildren);

		appendRow({tagItem, renderLayerItem});
	}

	void TagContainerEditor_AvailableTagsItemModel::setForbiddenTags(std::set<std::string> const& tags) {		
		std::set<QString> appliedTags;
		for(auto const& t: tags) {
			appliedTags.insert(QString::fromStdString(t));
		}
		for (int r = 0; r < rowCount(); ++r) {
			auto itemIndex = index(r, 0);
			auto item = itemFromIndex(itemIndex);
			setTagAllowed(item, appliedTags.find(item->text()) == appliedTags.end());
		}
	}


	std::string TagContainerEditor_AvailableTagsItemModel::tagForIndex(const QModelIndex& index) const {
		if (index.column() != 0) {
			return {};
		}
		auto* tagItem = itemFromIndex(index);
		if (tagItem == nullptr) {
			return {};
		}
		return tagItem->text().toStdString();
	}

	QStringList TagContainerEditor_AvailableTagsItemModel::mimeTypes() const {
		return {"text/plain"};
	}

	QMimeData* TagContainerEditor_AvailableTagsItemModel::mimeData(const QModelIndexList& indexes) const {
		assert(indexes.count() == 1);
		auto* data = new QMimeData();
		data->setText(itemFromIndex(indexes[0])->text());
		return data;
	}

}