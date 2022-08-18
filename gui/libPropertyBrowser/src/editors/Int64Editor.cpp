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


#include "core/Queries.h"
#include "core/BasicAnnotations.h"

#include "property_browser/PropertyBrowserItem.h"
#include "property_browser/PropertyBrowserLayouts.h"
#include "property_browser/controls/ScalarSlider.h"
#include "property_browser/controls/SpinBox.h"

#include <QStackedWidget>
#include <QWidget>

namespace raco::property_browser {

Int64Editor::Int64Editor(
	PropertyBrowserItem* item,
	QWidget* parent) : PropertyEditor(item, parent) {
	auto* layout = new PropertyBrowserGridLayout{this};
	stack_ = new QStackedWidget{this};

	auto* spinBox = new Int64SpinBox{stack_};
	using raco::property_browser::Int64Slider;
	auto* slider = new Int64Slider{stack_};

	slider->setValue(item->valueHandle().as<int64_t>());
	spinBox->setValue(item->valueHandle().as<int64_t>());

	if (auto rangeAnnotation = item->query<core::RangeAnnotation<int64_t>>()) {
		spinBox->setSoftRange(*rangeAnnotation->min_, *rangeAnnotation->max_);
		slider->setSoftRange(*rangeAnnotation->min_, *rangeAnnotation->max_);
	}

	// connect everything to our item values
	{
		QObject::connect(spinBox, &Int64SpinBox::valueEdited, item, [item](int64_t value) { item->set(value); });
		QObject::connect(slider, &Int64Slider::valueEdited, item, [item](int64_t value) { item->set(value); });
		QObject::connect(item, &PropertyBrowserItem::valueChanged, this, [slider, spinBox](core::ValueHandle& handle) {
			slider->setValue(handle.as<int64_t>());
			spinBox->setValue(handle.as<int64_t>());
		});
	}

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

}  // namespace raco::property_browser
