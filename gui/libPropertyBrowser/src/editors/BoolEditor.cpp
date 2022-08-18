/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/editors/BoolEditor.h"

#include "core/Queries.h"
#include "property_browser/PropertyBrowserItem.h"
#include "style/Colors.h"

#include <QCheckBox>
#include <property_browser/PropertyBrowserLayouts.h>
namespace raco::property_browser {

BoolEditor::BoolEditor(
	PropertyBrowserItem* item,
	QWidget* parent)
	: PropertyEditor(item, parent) {

	auto* layout{new PropertyBrowserHBoxLayout{this}};
	checkBox_ = new QCheckBox{this};
	QObject::connect(checkBox_, &QCheckBox::clicked, item, [item](bool checked) { item->set(checked); });
	QObject::connect(item, &PropertyBrowserItem::valueChanged, this, [this](core::ValueHandle& handle) { checkBox_->setChecked(handle.asBool()); });
	checkBox_->setChecked(item->valueHandle().asBool());
	layout->addWidget(checkBox_);

	QObject::connect(item, &PropertyBrowserItem::widgetRequestFocus, this, [this]() {
		checkBox_->setFocus();
	});
}

}  // namespace raco::property_browser
