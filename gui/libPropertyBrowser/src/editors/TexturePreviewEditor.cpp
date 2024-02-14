/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "property_browser/editors/TexturePreviewEditor.h"
#include "property_browser/controls/ImageWidget.h"
#include "property_browser/controls/TexturePreviewWidget.h"
#include "property_browser/Utilities.h"

#include "common_widgets/NoContentMarginsLayout.h"

#include "DockWidget.h"

#include <QVBoxLayout>

namespace raco::property_browser {

TexturePreviewEditor::TexturePreviewEditor(PropertyBrowserItem* item, QWidget* label, QWidget* parent) : PropertyEditor(item, parent) {
	const auto verticalLayout = new common_widgets::NoContentMarginsLayout<QVBoxLayout>(parent);
	imageWidget_ = new ImageWidget(this);
	verticalLayout->addWidget(imageWidget_);
	verticalLayout->addStretch(1);
	setLayout(verticalLayout);

	getImageWidget()->setHeightSourceWidget(findAncestor<ads::CDockWidget>(parent));
	
	auto pseudoLabel = qobject_cast<TexturePreviewWidget*>(label);
	assert(pseudoLabel != nullptr);
	pseudoLabel->setImageWidget(getImageWidget());
}

ImageWidget* TexturePreviewEditor::getImageWidget() const {
	return imageWidget_;
}

}  // namespace raco::property_browser
