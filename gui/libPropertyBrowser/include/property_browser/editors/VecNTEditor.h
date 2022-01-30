/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "PropertyEditor.h"
#include "common_widgets/PropertyBrowserButton.h"
#include "core/Queries.h"
#include "data_storage/BasicTypes.h"
#include "property_browser/PropertyBrowserItem.h"
#include "property_browser/PropertyBrowserLayouts.h"
#include "property_browser/controls/SpinBox.h"
#include "style/Colors.h"
#include "style/Icons.h"
#include "style/RaCoStyle.h"
#include <QBitmap>
#include <QColorDialog>
#include <QPainter>
#include <QPalette>
#include <array>
#include <type_traits>

namespace raco::property_browser {

template <int N>
class VecNTEditorColorPickerButton : public QPushButton {
public:
	VecNTEditorColorPickerButton(PropertyBrowserItem* item, QWidget* parent) : item_(item), QPushButton("", parent) {
		QObject::connect(this, &QPushButton::clicked, [this]() { this->showColorPicker(); });

		setFlat(true);

		setToolTip("Show Color Picker");
		setFixedHeight(26);
		setFixedWidth(26);
		setProperty("slimButton", true);
	}

	void paintEvent(QPaintEvent* event) override {
		QPushButton::paintEvent(event);

		QPainter painter{this};

		QPalette pal{palette()};

		painter.setRenderHint(QPainter::Antialiasing);
		auto backgroundBrush = pal.base();
		backgroundBrush.setColor(colorFromItem());
		auto pen = QPen(pal.windowText(), 2);
		painter.setBrush(backgroundBrush);
		painter.setPen(pen);
		painter.drawEllipse(QRect(rect().x() + 4 , rect().y() + 4, 18, 18));
	}

protected:
	PropertyBrowserItem* item_;

	QColor colorFromItem() const;
	void setColorToItem(QColor color);

	void showColorPicker() {
		QColorDialog::ColorDialogOptions options;

		if (N > 3) {
			options = QColorDialog::ColorDialogOption::ShowAlphaChannel;
		}

		auto currentColor = colorFromItem();

		QColor color = QColorDialog::getColor(currentColor, this, "Select Color", options);

		if (color.isValid()) {
			setColorToItem(color);
		}
	}
};

inline double clampColorComponent(double value) {
	if (value > 1.0) {
		return 1.0;
	} else if (value < 0.0) {
		return 0.0;
	} else {
		return value;
	}
};

template <>
inline QColor VecNTEditorColorPickerButton<3>::colorFromItem() const {
	auto r = item_->children().at(0)->valueHandle().as<double>();
	auto g = item_->children().at(1)->valueHandle().as<double>();
	auto b = item_->children().at(2)->valueHandle().as<double>();
	return QColor::fromRgbF(
		clampColorComponent(r),
		clampColorComponent(g),
		clampColorComponent(b));
};

template <>
inline QColor VecNTEditorColorPickerButton<4>::colorFromItem() const {
	auto r = item_->children().at(0)->valueHandle().as<double>();
	auto g = item_->children().at(1)->valueHandle().as<double>();
	auto b = item_->children().at(2)->valueHandle().as<double>();
	auto a = item_->children().at(3)->valueHandle().as<double>();
	return QColor::fromRgbF(
		clampColorComponent(r),
		clampColorComponent(g),
		clampColorComponent(b),
		clampColorComponent(a));
};

template <>
inline void VecNTEditorColorPickerButton<3>::setColorToItem(QColor color) {
	item_->set(std::array<double, 3>{color.redF(), color.greenF(), color.blueF()});
};

template <>
inline void VecNTEditorColorPickerButton<4>::setColorToItem(QColor color) {
	item_->set(std::array<double, 4>{color.redF(), color.greenF(), color.blueF(), color.alphaF()});
};

template <typename T>
struct VecNTEditorTraits {
	typedef IntSpinBox SpinBoxType;
};

template <>
struct VecNTEditorTraits<double> {
	typedef DoubleSpinBox SpinBoxType;
};

template <>
struct VecNTEditorTraits<int> {
	typedef IntSpinBox SpinBoxType;
};

template <typename T, int N>
class VecNTEditor final : public PropertyEditor {
	typedef typename VecNTEditorTraits<T>::SpinBoxType SpinBoxType;

public:
	explicit VecNTEditor(
		PropertyBrowserItem* item,
		QWidget* parent = nullptr)
		: PropertyEditor(item, parent) {
		static_assert(std::is_floating_point<T>::value || std::is_integral<T>::value, "VecNTEditor requires floating point or integral type");
		auto* layout = new PropertyBrowserGridLayout{this};
		for (int i = 0; i < N; i++) {
			spinboxes_[i].reset(new SpinBoxType{this});
			if (auto rangeAnnotation = item->children().at(i)->query<raco::data_storage::RangeAnnotation<T>>()) {
				spinboxes_[i]->setRange(*rangeAnnotation->min_, *rangeAnnotation->max_);
			}
			spinboxes_[i]->setValue(item->children().at(i)->valueHandle().as<T>());
			QObject::connect(spinboxes_[i].get(), &SpinBoxType::valueChanged, this, [this, item, i](T value) {
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

		QObject::connect(item, &PropertyBrowserItem::expandedChanged, this, &VecNTEditor<T, N>::setExpandedMode);
		setExpandedMode(item->expanded());
	}

public Q_SLOTS:
	void setEnabled(bool val) {
		for (auto* child : layout()->children()) {
			if (auto* widget{dynamic_cast<QWidget*>(child)}) {
				widget->setEnabled(val);
			}
		}
	}

	virtual void setExpandedMode(bool expanded) {
		for (int i = 0; i < N; i++) {
			spinboxes_[i]->setVisible(!expanded);
		}
		if (colorPickerButton_) {
			colorPickerButton_->setVisible(expanded);
		}
	}

	void updateSpinBoxesAndColorPicker() {
		for (int i = 0; i < N; i++) {
			spinboxes_[i]->setValue(item_->children().at(i)->valueHandle().as<T>());
		}
		updateColorPicker();
	}

protected:
	QPushButton* colorPickerButton_ = nullptr;
	std::array<std::unique_ptr<SpinBoxType>, N> spinboxes_;

	void setupColorPicker(PropertyBrowserItem* item);
	void updateColorPicker() {
		if (colorPickerButton_) {
			auto* button = static_cast<VecNTEditorColorPickerButton<N>*>(colorPickerButton_);
			button->update();
		}
	}
};

template <typename T, int N>
void VecNTEditor<T, N>::setupColorPicker(PropertyBrowserItem* item) {
	// Don't do anything, only 3 and 4 double vectors are supported.
}

template <>
inline void VecNTEditor<double, 3>::setupColorPicker(PropertyBrowserItem* item) {
	if (item->canBeChosenByColorPicker()) {
		auto* layout = static_cast<PropertyBrowserGridLayout*>(this->layout());
		colorPickerButton_ = new VecNTEditorColorPickerButton<3>(item, this);
		layout->addWidget(colorPickerButton_, 0, 0, Qt::AlignRight);
	}
}

template <>
inline void VecNTEditor<double, 4>::setupColorPicker(PropertyBrowserItem* item) {
	if (item->canBeChosenByColorPicker()) {
		auto* layout = static_cast<PropertyBrowserGridLayout*>(this->layout());
		colorPickerButton_ = new VecNTEditorColorPickerButton<4>(item, this);
		layout->addWidget(colorPickerButton_, 0, 0, Qt::AlignRight);
	}
}

using Vec2fEditor = VecNTEditor<double, 2>;
using Vec3fEditor = VecNTEditor<double, 3>;
using Vec4fEditor = VecNTEditor<double, 4>;
using Vec2iEditor = VecNTEditor<int, 2>;
using Vec3iEditor = VecNTEditor<int, 3>;
using Vec4iEditor = VecNTEditor<int, 4>;

}  // namespace raco::property_browser
