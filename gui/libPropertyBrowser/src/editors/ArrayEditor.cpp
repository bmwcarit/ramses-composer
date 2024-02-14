/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "property_browser/editors/ArrayEditor.h"
#include "property_browser/PropertyBrowserLayouts.h"

#include "common_widgets/PropertyBrowserButton.h"

#include "style/Icons.h"

#include <QLayout>

namespace raco::property_browser {

ArrayEditor::ArrayEditor(PropertyBrowserItem* item, QWidget* parent)
	: PropertyEditor(item, parent) {
	auto layout{new PropertyBrowserHBoxLayout{this}};
	layout->addStretch(1);
	this->setLayout(layout);

	descriptionLabel_ = new QLabel("Array Size:", this);
	layout->addWidget(descriptionLabel_);
	
	shrinkButton_ = new common_widgets::PropertyBrowserButton(style::Icons::instance().decrement, {}, this);
	layout->addWidget(shrinkButton_);

	sizeLabel_ = new QLabel(this);
	layout->addWidget(sizeLabel_);

	growButton_ = new common_widgets::PropertyBrowserButton(style::Icons::instance().increment, {}, this);
	layout->addWidget(growButton_);

	QObject::connect(shrinkButton_, &QPushButton::clicked, [this]() {
		if (item_->size() > 1) {
			item_->resizeArray(item_->size() - 1);
		}
	});

	QObject::connect(growButton_, &QPushButton::clicked, [this]() {
		item_->resizeArray(item_->size() + 1);
	});

	QObject::connect(item_, &PropertyBrowserItem::childrenChanged, this, &ArrayEditor::updateLabel);

	updateLabel();
}

void ArrayEditor::updateLabel() {
	sizeLabel_->setText(fmt::format("{}", item_->size()).c_str());
}

}  // namespace raco::property_browser