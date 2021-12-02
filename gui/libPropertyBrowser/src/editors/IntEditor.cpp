/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/editors/IntEditor.h"


#include "core/Queries.h"
#include "data_storage/BasicAnnotations.h"

#include "property_browser/PropertyBrowserItem.h"
#include "property_browser/PropertyBrowserLayouts.h"
#include "property_browser/controls/ScalarSlider.h"
#include "property_browser/controls/SpinBox.h"

#include <QStackedWidget>
#include <QWidget>

namespace raco::property_browser {

IntEditor::IntEditor(
	PropertyBrowserItem* item,
	QWidget* parent) : QWidget{parent} {
	auto* layout = new PropertyBrowserGridLayout{this};
	stack_ = new QStackedWidget{this};

	auto* spinBox = new IntSpinBox{stack_};
	using raco::property_browser::IntSlider;
	auto* slider = new IntSlider{stack_};

	slider->setValue(item->valueHandle().as<int>());
	spinBox->setValue(item->valueHandle().as<int>());

	if (auto rangeAnnotation = item->query<core::RangeAnnotation<int>>()) {
		spinBox->setRange(*rangeAnnotation->min_, *rangeAnnotation->max_);
		slider->setRange(*rangeAnnotation->min_, *rangeAnnotation->max_);
	}

	// connect everything to our item values
	{
		QObject::connect(spinBox, &IntSpinBox::valueChanged, item, [item](int value) { item->set(value); });
		QObject::connect(slider, &IntSlider::valueEdited, item, [item](int value) { item->set(value); });
		QObject::connect(item, &PropertyBrowserItem::valueChanged, this, [slider, spinBox](core::ValueHandle& handle) {
			slider->setValue(handle.as<int>());
			spinBox->setValue(handle.as<int>());
		});
	}

	// State change: Show spinbox or slider
	QObject::connect(slider, &IntSlider::singleClicked, this, [this, spinBox]() { stack_->setCurrentWidget(spinBox); });
	QObject::connect(spinBox, &IntSpinBox::editingFinished, this, [this, slider]() { stack_->setCurrentWidget(slider); });
	QObject::connect(spinBox, &IntSpinBox::focusNextRequested, this, [this, item]() { item->requestNextSiblingFocus(); });
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
