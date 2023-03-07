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
#include "core/BasicAnnotations.h"
#include "property_browser/PropertyBrowserItem.h"
#include "property_browser/PropertyBrowserLayouts.h"
#include "property_browser/controls/MouseWheelGuard.h"
#include "style/Colors.h"
#include "user_types/Enumerations.h"
#include "core/EngineInterface.h"

#include <QComboBox>

namespace raco::property_browser {

EnumerationEditor::EnumerationEditor(PropertyBrowserItem* item, QWidget* parent) : PropertyEditor(item, parent) {
	auto* layout{new PropertyBrowserGridLayout{this}};
	comboBox_ = new QComboBox(this);
	comboBox_->setFocusPolicy(Qt::StrongFocus);
	comboBox_->installEventFilter(new MouseWheelGuard());
	layout->addWidget(comboBox_);

	auto& values = raco::user_types::enumerationDescription(static_cast<raco::core::EUserTypeEnumerations>(item->query<raco::core::EnumerationAnnotation>()->type_.asInt()));
	for (const auto& [entryEnumValue, entryEnumString] : values) {
		comboBox_->addItem(entryEnumString.c_str());
		ramsesEnumIndexToComboBoxIndex_[entryEnumValue] = comboBoxIndexToRamsesEnumIndex_.size();
		comboBoxIndexToRamsesEnumIndex_.emplace_back(entryEnumValue);
	}
	if (item->type() == data_storage::PrimitiveType::Bool) {
		assert(values.size() == 2);
		comboBox_->setCurrentIndex(item->valueHandle().asBool() ? 1 : 0);		
	} else {
		comboBox_->setCurrentIndex(ramsesEnumIndexToComboBoxIndex_[item->valueHandle().asInt()]);
	}
	QObject::connect(comboBox_, qOverload<int>(&QComboBox::activated), item, [this, item](int index) {
		if (item->type() == data_storage::PrimitiveType::Bool) {
			item->set(index > 0);
		} else {
			item->set(comboBoxIndexToRamsesEnumIndex_[index]);
		}
	});
	QObject::connect(item, &PropertyBrowserItem::valueChanged, this, [this] (raco::core::ValueHandle& handle) {
		if (handle.type() == data_storage::PrimitiveType::Bool) {
			comboBox_->setCurrentIndex(handle.asBool() ? 1 : 0);
		} else {
			comboBox_->setCurrentIndex(ramsesEnumIndexToComboBoxIndex_[handle.asInt()]);
		}
	});
}

int EnumerationEditor::currentIndex() const {
	return comboBox_->currentIndex();
}


}  // namespace raco::property_browser
