/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "property_browser/editors/VecNTEditor.h"

#include <algorithm>

namespace raco::property_browser {

template <int N>
VecNTEditorColorPickerButton<N>::VecNTEditorColorPickerButton(PropertyBrowserItem* item, QWidget* parent) : item_(item), QPushButton("", parent) {
	QObject::connect(this, &QPushButton::clicked, [this]() { this->showColorPicker(); });

	setFlat(true);

	setToolTip("Show Color Picker");
	setFixedHeight(24);
	setFixedWidth(24);
	setProperty("slimButton", true);
}

template <int N>
void VecNTEditorColorPickerButton<N>::paintEvent(QPaintEvent* event) {
		QPushButton::paintEvent(event);

		QPainter painter{this};
		QPalette pal{palette()};
		painter.setRenderHint(QPainter::Antialiasing);
		auto backgroundBrush = pal.base();
		auto pen = QPen(pal.windowText(), 1.5);
		painter.setPen(pen);

		if (item_->hasSingleValue()) {
			backgroundBrush.setColor(colorFromItem());
		} else {
			backgroundBrush.setStyle(Qt::BrushStyle::BDiagPattern);
			backgroundBrush.setColor(style::Colors::color(style::Colormap::text));
		}
		painter.setBrush(backgroundBrush);
		painter.drawEllipse(QRect(rect().x() + 4, rect().y() + 4, 16, 16));
	}

template <int N>
void VecNTEditorColorPickerButton<N>::showColorPicker() {
	QColorDialog::ColorDialogOptions options;

	if (N > 3) {
		options = QColorDialog::ColorDialogOption::ShowAlphaChannel;
	}

	QColor currentColor(0, 0, 0, 255);
	if (item_->hasSingleValue()) {
		currentColor = colorFromItem();
	}

	QColor color = QColorDialog::getColor(currentColor, this, "Select Color", options);

	if (color.isValid()) {
		setColorToItem(color);
	}
}

template <>
QColor VecNTEditorColorPickerButton<3>::colorFromItem() const {
	auto r = item_->children().at(0)->as<double>().value();
	auto g = item_->children().at(1)->as<double>().value();
	auto b = item_->children().at(2)->as<double>().value();
	return QColor::fromRgbF(
		std::clamp(r, 0.0, 1.0),
		std::clamp(g, 0.0, 1.0),
		std::clamp(b, 0.0, 1.0));
};

template <>
QColor VecNTEditorColorPickerButton<4>::colorFromItem() const {
	auto r = item_->children().at(0)->as<double>().value();
	auto g = item_->children().at(1)->as<double>().value();
	auto b = item_->children().at(2)->as<double>().value();
	auto a = item_->children().at(3)->as<double>().value();
	return QColor::fromRgbF(
		std::clamp(r, 0.0, 1.0),
		std::clamp(g, 0.0, 1.0),
		std::clamp(b, 0.0, 1.0),
		std::clamp(a, 0.0, 1.0));
};

template <>
void VecNTEditorColorPickerButton<3>::setColorToItem(QColor color) {
	item_->set(std::array<double, 3>{color.redF(), color.greenF(), color.blueF()});
};

template <>
void VecNTEditorColorPickerButton<4>::setColorToItem(QColor color) {
	item_->set(std::array<double, 4>{color.redF(), color.greenF(), color.blueF(), color.alphaF()});
};

template <typename T, int N>
VecNTEditor<T, N>::VecNTEditor(
	PropertyBrowserItem* item,
	QWidget* parent)
	: PropertyEditor(item, parent) {
	static_assert(std::is_floating_point<T>::value || std::is_integral<T>::value, "VecNTEditor requires floating point or integral type");
	auto* layout = new PropertyBrowserGridLayout{this};
	for (int i = 0; i < N; i++) {
		spinboxes_[i].reset(new SpinBoxType{this});
		if (auto rangeAnnotation = item->children().at(i)->query<raco::core::RangeAnnotation<T>>()) {
			spinboxes_[i]->setSoftRange(*rangeAnnotation->min_, *rangeAnnotation->max_);
		}
		QObject::connect(spinboxes_[i].get(), &SpinBoxType::valueEdited, this, [this, item, i](T value) {
			item->children().at(i)->set(value);
			updateColorPicker();
		});
		QObject::connect(item->children().at(i), &PropertyBrowserItem::valueChanged, this, &VecNTEditor<T, N>::updateSpinBoxesAndColorPicker);

		if (i < N - 1) {
			int nextSpinboxIndex = i + 1;
			QObject::connect(spinboxes_[i].get(), &SpinBoxType::focusNextRequested, this, [this, nextSpinboxIndex]() {
				spinboxes_[nextSpinboxIndex]->setFocus();
			});
		}

		layout->addWidget(spinboxes_[i].get(), 0, i);
	}
	QObject::connect(item, &PropertyBrowserItem::childrenChanged, this, &VecNTEditor<T, N>::updateSpinBoxesAndColorPicker);
	QObject::connect(item, &PropertyBrowserItem::childrenChanged, this, [this](const QList<PropertyBrowserItem*>& children) {
		for (int i = 0; i < N; i++) {
			QObject::connect(children.at(i), &PropertyBrowserItem::valueChanged, this, &VecNTEditor<T, N>::updateSpinBoxesAndColorPicker);
		}
	});

	setupColorPicker(item);

	updateSpinBoxesAndColorPicker();

	QObject::connect(item, &PropertyBrowserItem::expandedChanged, this, &VecNTEditor<T, N>::setExpandedMode);
	setExpandedMode(item->expanded());
}

template <typename T, int N>
void VecNTEditor<T, N>::setEnabled(bool val) {
	for (auto* child : layout()->children()) {
		if (auto* widget{dynamic_cast<QWidget*>(child)}) {
			widget->setEnabled(val);
		}
	}
}

template <typename T, int N>
void VecNTEditor<T, N>::setExpandedMode(bool expanded) {
	for (int i = 0; i < N; i++) {
		spinboxes_[i]->setVisible(!expanded);
	}
	if (colorPickerButton_) {
		colorPickerButton_->setVisible(expanded);
	}
}

template <typename T, int N>
void VecNTEditor<T, N>::updateSpinBoxesAndColorPicker() {
	for (int i = 0; i < N; i++) {
		auto childItem = item_->children().at(i);
		auto childValue = childItem->as<T>();
		if (childValue.has_value()) {
			spinboxes_[i]->setValue(childValue.value());
		} else {
			spinboxes_[i]->setMultipleValues();
		}
	}
	updateColorPicker();
}

template <typename T, int N>
void VecNTEditor<T, N>::setupColorPicker(PropertyBrowserItem* item) {
	if constexpr (N == 3 || N == 4) {
		if (item->canBeChosenByColorPicker()) {
			auto* layout = static_cast<PropertyBrowserGridLayout*>(this->layout());
			colorPickerButton_ = new VecNTEditorColorPickerButton<N>(item, this);
			layout->addWidget(colorPickerButton_, 0, 0, Qt::AlignRight);
		}
	}
}

template <typename T, int N>
void VecNTEditor<T, N>::updateColorPicker() {
	if (colorPickerButton_) {
		auto* button = static_cast<VecNTEditorColorPickerButton<N>*>(colorPickerButton_);
		button->update();
	}
}

template class VecNTEditor<double, 2>;
template class VecNTEditor<double, 3>;
template class VecNTEditor<double, 4>;
template class VecNTEditor<int, 2>;
template class VecNTEditor<int, 3>;
template class VecNTEditor<int, 4>;

}  // namespace raco::property_browser