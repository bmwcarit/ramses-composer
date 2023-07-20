/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/editors/EnumerationEditor.h"

#include "core/EngineInterface.h"
#include "core/Queries.h"
#include "property_browser/PropertyBrowserItem.h"
#include "property_browser/PropertyBrowserLayouts.h"
#include "property_browser/controls/MouseWheelGuard.h"

#include <QComboBox>
#include <QLabel>

namespace raco::property_browser {

EnumerationEditor::EnumerationEditor(PropertyBrowserItem* item, QWidget* parent) : PropertyEditor(item, parent) {
	auto* layout{new PropertyBrowserGridLayout{this}};
	comboBox_ = new QComboBox(this);
	comboBox_->setFocusPolicy(Qt::StrongFocus);
	comboBox_->installEventFilter(new MouseWheelGuard());
	layout->addWidget(comboBox_);

	auto& values = user_types::enumerationDescription(static_cast<core::EUserTypeEnumerations>(item->query<core::EnumerationAnnotation>()->type_.asInt()));
	for (const auto& [entryEnumValue, entryEnumString] : values) {
		comboBox_->addItem(entryEnumString.c_str());
		ramsesEnumIndexToComboBoxIndex_[entryEnumValue] = comboBoxIndexToRamsesEnumIndex_.size();
		comboBoxIndexToRamsesEnumIndex_.emplace_back(entryEnumValue);
	}

	// This should work but doesn't in Qt 5.15.2
	// comboBox_->setPlaceholderText(PropertyBrowserItem::MultipleValueText);
	// ...so we need to use a workaround instead:
	auto placeholder = new QLabel("  " + PropertyBrowserItem::MultipleValueText);
	comboBox_->setLayout(new QVBoxLayout());
	comboBox_->layout()->setContentsMargins(0, 0, 0, 0);
	comboBox_->layout()->addWidget(placeholder);
	// this should work but doesn't
	//QObject::connect(comboBox_, qOverload<int>(&QComboBox::currentIndexChanged), [placeholder](int index) {
	//	placeholder->setVisible(index == -1);
	//});

	auto updateCombobox = [this, item, placeholder]() {
		auto value = item->as<int>();
		if (value.has_value()) {
			comboBox_->setCurrentIndex(ramsesEnumIndexToComboBoxIndex_[value.value()]);
			placeholder->setVisible(false);
		} else {
			comboBox_->setCurrentIndex(-1);
			placeholder->setVisible(true);
		}
	};

	updateCombobox();

	QObject::connect(comboBox_, qOverload<int>(&QComboBox::activated), item, [this, item](int index) {
		item->set(comboBoxIndexToRamsesEnumIndex_[index]);
	});
	QObject::connect(item, &PropertyBrowserItem::valueChanged, this, [this, updateCombobox]() {
		updateCombobox();
	});
}

int EnumerationEditor::currentIndex() const {
	return comboBox_->currentIndex();
}

}  // namespace raco::property_browser
