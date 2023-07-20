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

#include "property_browser/PopupDialog.h"

#include "TreeViewWithDel.h"

#include <QCompleter>
#include <QDialog>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QTreeView>

#include <set>

class QApplicationStateChangeEvent;

namespace raco::core {
enum class TagType;
class TagDataCache;
}

namespace raco::property_browser {

	class PropertyBrowserItem;
	class TagContainerEditor_AvailableTagsItemModel;
	class TagContainerEditor_AppliedTagModel;
	class TreeViewWithDel;

	class TagContainerEditor_Popup : public PopupDialog {
	public:
		explicit TagContainerEditor_Popup(PropertyBrowserItem* item, core::TagType tagType, QWidget* anchor);
		~TagContainerEditor_Popup() override;
		void newTagListAccepted();
		void newTagListRejected();

	protected:
		TagContainerEditor_AppliedTagModel* tagListItemModel_{};

	private:
		void addTag(const QModelIndex& itemIndex);
		void onTagListUpdated();
		void updateRenderedBy();
		bool showRenderedBy() const;
		
		core::TagType tagType_;
		PropertyBrowserItem* item_;
		QGridLayout outerLayout_{ this };
		QFrame frame_{ this };
		QGridLayout layout_{ &frame_ };
		QPushButton acceptButton_{ this };
		QPushButton closeButton_{ this };
		TagContainerEditor_AvailableTagsItemModel* availableTagsItemModel_{  };
		QTreeView listOfAvailableTags_{ this };
		std::unique_ptr<TreeViewWithDel> listOfTags_{};
		QCompleter completer_{};
		QLabel referencedBy_{ this };
		QLabel renderedBy_{this};
		
		std::unique_ptr<core::TagDataCache> tagDataCache_{};
		std::set<std::string> forbiddenTags_{};
	};

}

