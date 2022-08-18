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

#include <QStandardItemModel>

#include <set>

namespace raco::property_browser {

	class TagContainerEditor_AvailableTagsItemModel : public QStandardItemModel {
	public:
		TagContainerEditor_AvailableTagsItemModel(QObject* parent) : QStandardItemModel(parent) {
		}

		void addTag(QString const& tagName, QStringList const& renderLayerNames, bool isApplied);
		void setForbiddenTags(std::set<std::string> const& tags);
		std::string tagForIndex(const QModelIndex& index) const;
		QStringList mimeTypes() const override;
		QMimeData* mimeData(const QModelIndexList& indexes) const override;

	private:
		void setTagAllowed(QStandardItem* tagItem, bool isAllowed);
	};

}
