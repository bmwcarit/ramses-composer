/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/editors/Int64Editor.h"

#include "core/BasicAnnotations.h"
#include "core/Queries.h"

#include "property_browser/PropertyBrowserItem.h"
#include "property_browser/PropertyBrowserLayouts.h"

#include <QStackedWidget>
#include <QWidget>

namespace raco::property_browser {

Int64Editor::Int64Editor(
	PropertyBrowserItem* item,
	QWidget* parent) : PropertyEditor(item, parent) {
	auto* layout = new PropertyBrowserGridLayout{this};
	stack_ = new QStackedWidget{this};

	auto* slider = new Int64Slider{stack_};
	auto* spinBox = new Int64SpinBox{stack_};

	setValueToControls(slider, spinBox);

	if (auto rangeAnnotation = item->query<core::RangeAnnotation<int64_t>>()) {
		spinBox->setSoftRange(*rangeAnnotation->min_, *rangeAnnotation->max_);
		slider->setSoftRange(*rangeAnnotation->min_, *rangeAnnotation->max_);
	}

	// connect everything to our item values
	QObject::connect(spinBox, &Int64SpinBox::valueEdited, item, [item](int64_t value) { item->set(value); });
	QObject::connect(slider, &Int64Slider::valueEdited, item, [item](int64_t value) { item->set(value); });
	QObject::connect(item, &PropertyBrowserItem::valueChanged, this, [this, item, slider, spinBox]() {
		setValueToControls(slider, spinBox);
	});

	QObject::connect(spinBox, &Int64SpinBox::saveFocusInValue, item, [this]() {
		focusInValues_.clear();
		for (const auto& handle : item_->valueHandles()) {
			focusInValues_[handle] = handle.asInt64();
		}
	});
	QObject::connect(spinBox, &Int64SpinBox::restoreFocusInValue, item, [this, slider, spinBox]() {
		std::string desc = fmt::format("Restore value of property '{}'", item_->getPropertyPath());
		item_->commandInterface()->executeCompositeCommand(
			[this]() {
				for (const auto& handle : item_->valueHandles()) {
					item_->commandInterface()->set(handle, focusInValues_[handle]);
				}
			},
			desc);
		setValueToControls(slider, spinBox);
	});

	// State change: Show spinbox or slider
	QObject::connect(slider, &Int64Slider::singleClicked, this, [this, spinBox]() { stack_->setCurrentWidget(spinBox); });
	QObject::connect(spinBox, &Int64SpinBox::editingFinished, this, [this, slider]() {
		stack_->setCurrentWidget(slider);
		slider->clearFocus();
	});
	QObject::connect(spinBox, &Int64SpinBox::focusNextRequested, this, [this, item]() { item->requestNextSiblingFocus(); });
	QObject::connect(item, &PropertyBrowserItem::widgetRequestFocus, this, [this, spinBox]() {
		stack_->setCurrentWidget(spinBox);
		spinBox->setFocus();
	});

	stack_->addWidget(slider);
	stack_->addWidget(spinBox);

	stack_->setCurrentWidget(slider);
	layout->addWidget(stack_);
}

void Int64Editor::setValueToControls(Int64Slider* slider, Int64SpinBox* spinBox) const {
	auto value = item_->as<int64_t>();
	if (value.has_value()) {
		slider->setValue(value.value());
		spinBox->setValue(value.value());
	} else {
		slider->setMultipleValues();
		spinBox->setMultipleValues();
	}
}

}  // namespace raco::property_browser
