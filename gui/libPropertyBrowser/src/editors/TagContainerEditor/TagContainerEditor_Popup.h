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

#include "common_widgets/PopupDialog.h"

#include "TreeViewWithDel.h"

#include <QCompleter>
#include <QDialog>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QTreeView>

#include <set>

class QApplicationStateChangeEvent;

namespace raco::property_browser {

	enum class TagType;
	class PropertyBrowserItem;
	class TagDataCache;
	class TagContainerEditor_AvailableTagsItemModel;
	class TagContainerEditor_AppliedTagModel;
	class TreeViewWithDel;

	class TagContainerEditor_Popup : public common_widgets::PopupDialog {
	public:
		explicit TagContainerEditor_Popup(PropertyBrowserItem* item, TagType tagType, QWidget* anchor);
		~TagContainerEditor_Popup() override;
		void newTagListAccepted();
		void newTagListRejected();

	private:
		void addTag(const QModelIndex& itemIndex);
		void onTagListUpdated();
		void updateRenderedBy();
		bool showRenderedBy() const;
		
		TagType tagType_;
		PropertyBrowserItem* item_;
		QGridLayout outerLayout_{ this };
		QFrame frame_{ this };
		QGridLayout layout_{ &frame_ };
		QPushButton acceptButton_{ this };
		QPushButton closeButton_{ this };
		TagContainerEditor_AvailableTagsItemModel* availableTagsItemModel_{  };
		TagContainerEditor_AppliedTagModel* tagListItemModel_{};
		QTreeView listOfAvailableTags_{ this };
		std::unique_ptr<TreeViewWithDel> listOfTags_{};
		QCompleter completer_{};
		QLabel referencedBy_{ this };
		QLabel renderedBy_{this};
		
		std::unique_ptr<TagDataCache> tagDataCache_{};
		std::set<std::string> forbiddenTags_{};
	};

}

