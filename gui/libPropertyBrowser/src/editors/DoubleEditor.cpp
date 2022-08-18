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


#include "core/Queries.h"
#include "core/BasicAnnotations.h"

#include "property_browser/PropertyBrowserItem.h"
#include "property_browser/PropertyBrowserLayouts.h"
#include "property_browser/controls/ScalarSlider.h"
#include "property_browser/controls/SpinBox.h"

#include <QStackedWidget>

namespace raco::property_browser {

DoubleEditor::DoubleEditor(
	PropertyBrowserItem* item,
	QWidget* parent) : PropertyEditor(item, parent) {
	auto* layout = new PropertyBrowserGridLayout{this};
	stack_ = new QStackedWidget{this};

	DoubleSpinBox* spinBox = new DoubleSpinBox{stack_};
	using raco::property_browser::DoubleSlider;
	auto* slider = new DoubleSlider{stack_};

	slider->setValue(item->valueHandle().as<double>());
	spinBox->setValue(item->valueHandle().as<double>());

	if (auto rangeAnnotation = item->query<core::RangeAnnotation<double>>()) {
		spinBox->setSoftRange(*rangeAnnotation->min_, *rangeAnnotation->max_);
		slider->setSoftRange(*rangeAnnotation->min_, *rangeAnnotation->max_);
	}

	// connect everything to our item values
	{
		QObject::connect(spinBox, &DoubleSpinBox::valueEdited, item, [item](double value) {
			item->set(value);
		});
		QObject::connect(slider, &DoubleSlider::valueEdited, item, [item](double value) {
			item->set(value);
		});
		QObject::connect(item, &PropertyBrowserItem::valueChanged, this, [slider, spinBox](core::ValueHandle& handle) {
			slider->setValue(handle.as<double>());
			spinBox->setValue(handle.as<double>());
		});
	}

	// State change: Show spinbox or slider
	QObject::connect(slider, &DoubleSlider::singleClicked, this, [this, spinBox]() { stack_->setCurrentWidget(spinBox); });
	QObject::connect(spinBox, &DoubleSpinBox::editingFinished, this, [this, slider]() {
		stack_->setCurrentWidget(slider);
		slider->clearFocus();
	});
	QObject::connect(spinBox, &DoubleSpinBox::focusNextRequested, this, [this, item]() { item->requestNextSiblingFocus(); });
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
