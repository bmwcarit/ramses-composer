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

#include "components/DataChangeDispatcher.h"

#include "PropertyEditor.h"

#include <array>

class QLabel;
class QListWidget;
class QPushButton;

namespace raco::core {
	enum class TagType;
}

namespace raco::property_browser {
class PropertyBrowserItem;
class TagDataCache;

class TagContainerEditor : public PropertyEditor {
	Q_OBJECT
public:
	explicit TagContainerEditor(PropertyBrowserItem* item, QWidget* parent);

Q_SIGNALS:
	void tagCacheDirty();

protected:
	QListWidget* tagList_{};

private:
	void editButtonClicked() const;
	void updateTags() const;
	void updateTextAndButton() const;
	bool showRenderedBy() const;

	core::TagType tagType_{};
	QPushButton* editButton_{};
	QLabel* renderedBy_ {};

	//std::unique_ptr<TagDataCache> tagDataCache_;
	std::array<components::Subscription, 14> renderLayerSubscriptions_;
	std::vector<components::Subscription> materialPropertySubscriptions_;
};
}  // namespace raco::property_browser
