/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/editors/IntEditor.h"

#include "core/BasicAnnotations.h"
#include "core/Queries.h"

#include "property_browser/PropertyBrowserItem.h"
#include "property_browser/PropertyBrowserLayouts.h"
#include "property_browser/controls/ScalarSlider.h"
#include "property_browser/controls/SpinBox.h"

#include <QStackedWidget>
#include <QWidget>

namespace raco::property_browser {

IntEditor::IntEditor(
	PropertyBrowserItem* item,
	QWidget* parent) : PropertyEditor(item, parent) {
	auto* layout = new PropertyBrowserGridLayout{this};
	stack_ = new QStackedWidget{this};

	spinBox_ = new IntSpinBox{stack_};
	using property_browser::IntSlider;
	slider_ = new IntSlider{stack_};

	setValueToControls(slider_, spinBox_);

	if (auto rangeAnnotation = item->query<core::RangeAnnotation<int>>()) {
		spinBox_->setSoftRange(*rangeAnnotation->min_, *rangeAnnotation->max_);
		slider_->setSoftRange(*rangeAnnotation->min_, *rangeAnnotation->max_);
	}

	// connect everything to our item values
	QObject::connect(spinBox_, &IntSpinBox::valueEdited, item, [item](int value) { item->set(value); });
	QObject::connect(slider_, &IntSlider::valueEdited, item, [item](int value) { item->set(value); });
	QObject::connect(item, &PropertyBrowserItem::valueChanged, this, [this, item]() {
		setValueToControls(slider_, spinBox_);
	});

	QObject::connect(spinBox_, &IntSpinBox::saveFocusInValue, item, [this]() {
		focusInValues_.clear();
		for (const auto& handle : item_->valueHandles()) {
			focusInValues_[handle] = handle.asInt();
		}
	});
	QObject::connect(spinBox_, &IntSpinBox::restoreFocusInValue, item, [this]() {
		std::string desc = fmt::format("Restore value of property '{}'", item_->getPropertyPath());
		item_->commandInterface()->executeCompositeCommand(
			[this]() {
				for (const auto& handle : item_->valueHandles()) {
					item_->commandInterface()->set(handle, focusInValues_[handle]);
				}
			},
			desc);
		setValueToControls(slider_, spinBox_);
	});

	// State change: Show spinbox or slider
	QObject::connect(slider_, &IntSlider::singleClicked, this, [this]() { stack_->setCurrentWidget(spinBox_); });
	QObject::connect(spinBox_, &IntSpinBox::editingFinished, this, [this]() {
		stack_->setCurrentWidget(slider_);
		slider_->clearFocus();
	});
	QObject::connect(spinBox_, &IntSpinBox::focusNextRequested, this, [this, item]() { item->requestNextSiblingFocus(); });
	QObject::connect(item, &PropertyBrowserItem::widgetRequestFocus, this, [this]() {
		stack_->setCurrentWidget(spinBox_);
		spinBox_->setFocus();
	});

	stack_->addWidget(slider_);
	stack_->addWidget(spinBox_);

	stack_->setCurrentWidget(slider_);
	layout->addWidget(stack_);
}

void IntEditor::setValueToControls(IntSlider* slider, IntSpinBox* spinBox) const {
	auto value = item_->as<int>();
	if (value.has_value()) {
		slider->setValue(value.value());
		spinBox->setValue(value.value());
	} else {
		slider->setMultipleValues();
		spinBox->setMultipleValues();
	}
}

}  // namespace raco::property_browser
