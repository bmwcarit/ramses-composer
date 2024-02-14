/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/editors/DoubleEditor.h"

#include "core/BasicAnnotations.h"
#include "core/Queries.h"

#include "property_browser/PropertyBrowserItem.h"
#include "property_browser/PropertyBrowserLayouts.h"

#include <QStackedWidget>

namespace raco::property_browser {

DoubleEditor::DoubleEditor(
	PropertyBrowserItem* item,
	QWidget* parent) : PropertyEditor(item, parent) {
	auto* layout = new PropertyBrowserGridLayout{this};
	stack_ = new QStackedWidget{this};

	slider_ = new DoubleSlider{stack_};
	spinBox_ = new DoubleSpinBox{stack_};

	setValueToControls(slider_, spinBox_);

	if (auto rangeAnnotation = item->query<core::RangeAnnotation<double>>()) {
		spinBox_->setSoftRange(*rangeAnnotation->min_, *rangeAnnotation->max_);
		slider_->setSoftRange(*rangeAnnotation->min_, *rangeAnnotation->max_);
	}

	// connect everything to our item values
	QObject::connect(spinBox_, &DoubleSpinBox::valueEdited, item, [item](double value) {
		item->set(value);
	});
	QObject::connect(slider_, &DoubleSlider::valueEdited, item, [item](double value) {
		item->set(value);
	});
	QObject::connect(item, &PropertyBrowserItem::valueChanged, this, [this, item]() {
		setValueToControls(slider_, spinBox_);
	});

	QObject::connect(spinBox_, &DoubleSpinBox::saveFocusInValue, item, [this]() {
		focusInValues_.clear();
		for (const auto& handle : item_->valueHandles()) {
			focusInValues_[handle] = handle.asDouble();
		}
	});
	QObject::connect(spinBox_, &DoubleSpinBox::restoreFocusInValue, item, [this]() {
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
	QObject::connect(slider_, &DoubleSlider::singleClicked, this, [this]() { stack_->setCurrentWidget(spinBox_); });
	QObject::connect(spinBox_, &DoubleSpinBox::editingFinished, this, [this]() {
		stack_->setCurrentWidget(slider_);
		slider_->clearFocus();
	});
	QObject::connect(spinBox_, &DoubleSpinBox::focusNextRequested, this, [this, item]() { item->requestNextSiblingFocus(); });
	QObject::connect(item, &PropertyBrowserItem::widgetRequestFocus, this, [this]() {
		stack_->setCurrentWidget(spinBox_);
		spinBox_->setFocus();
	});

	stack_->addWidget(slider_);
	stack_->addWidget(spinBox_);

	stack_->setCurrentWidget(slider_);
	layout->addWidget(stack_);

	// eventFilter to capture right click for showing copy dialog.
	installEventFilter(this);
	canDisplayCopyDialog = true;
	setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
}

void DoubleEditor::setValueToControls(DoubleSlider* slider, DoubleSpinBox* spinBox) const {
	auto value = item_->as<double>();
	if (value.has_value()) {
		slider->setValue(value.value());
		spinBox->setValue(value.value());
	} else {
		slider->setMultipleValues();
		spinBox->setMultipleValues();
	}
}

}  // namespace raco::property_browser
