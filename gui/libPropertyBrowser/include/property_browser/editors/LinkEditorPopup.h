/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "property_browser/LinkStartSearchView.h"
#include "core/Queries.h"
#include "property_browser/PropertyBrowserEditorPopup.h"
#include "property_browser/PropertyBrowserModel.h"
#include "property_browser/PropertyBrowserUtilities.h"
#include "style/Icons.h"

#include <QDropEvent>

namespace raco::property_browser {

using namespace style;

class LinkEditorPopup : public PropertyBrowserEditorPopup {
	QMetaObject::Connection linkStateChangedConnection_;

public:
	using LinkState = core::Queries::LinkState;

	LinkEditorPopup(PropertyBrowserItem* item, QWidget* anchor) : PropertyBrowserEditorPopup{item, anchor, new LinkStartSearchView(item->dispatcher(), item->project(), item->valueHandles(), anchor)} {
		currentRelation_.setReadOnly(true);
		deleteButton_.setFlat(true);
		deleteButton_.setIcon(Icons::instance().remove);
		update();
		linkStateChangedConnection_ = QObject::connect(item_, &PropertyBrowserItem::linkStateChanged, this, &LinkEditorPopup::update);
	}

	~LinkEditorPopup() {
		QObject::disconnect(linkStateChangedConnection_);
	}

protected:
	void update() {
		currentRelation_.setVisible(true);
		deleteButton_.setVisible(true);
		const auto linkText = item_->linkText();
		if (linkText.has_value()) {
			if (!linkText.value().empty()) {  // linked
				currentRelation_.setText(linkText.value().c_str());
			} else {
				currentRelation_.setText("");
				currentRelation_.setVisible(false);
				deleteButton_.setVisible(false);
			}
		} else {
			currentRelation_.setText(PropertyBrowserItem::MultipleValueText);
		}
	}

	void establishObjectRelation() override {
		auto allowedStrong = dynamic_cast<LinkStartSearchView*>(list_)->allowedStrong(list_->selection());
		auto allowedWeak = dynamic_cast<LinkStartSearchView*>(list_)->allowedWeak(list_->selection());
		item_->setLink(list_->handleFromIndex(list_->selection()), !allowedStrong && allowedWeak);
	}

	void removeObjectRelation() override {
		item_->removeLink();
	}
};

}  // namespace raco::property_browser
