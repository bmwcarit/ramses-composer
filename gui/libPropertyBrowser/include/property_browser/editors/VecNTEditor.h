/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "PropertyEditor.h"
#include "common_widgets/PropertyBrowserButton.h"
#include "core/Queries.h"
#include "property_browser/PropertyBrowserItem.h"
#include "property_browser/PropertyBrowserLayouts.h"
#include "property_browser/controls/SpinBox.h"
#include "style/Colors.h"
#include <QBitmap>
#include <QColorDialog>
#include <QPainter>
#include <array>

namespace raco::property_browser {

template <int N>
class VecNTEditorColorPickerButton : public QPushButton {
public:
	VecNTEditorColorPickerButton(PropertyBrowserItem* item, QWidget* parent);
	void paintEvent(QPaintEvent* event) override;

protected:
	PropertyBrowserItem* item_;

	QColor colorFromItem() const;
	void setColorToItem(QColor color);
	void showColorPicker();
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
class VecNTEditor : public PropertyEditor {
	typedef typename VecNTEditorTraits<T>::SpinBoxType SpinBoxType;

public:
	explicit VecNTEditor(
		PropertyBrowserItem* item,
		QWidget* parent = nullptr);

public Q_SLOTS:
	void setEnabled(bool val);
	virtual void setExpandedMode(bool expanded);
	void updateSpinBoxesAndColorPicker();

protected:
	QPushButton* colorPickerButton_ = nullptr;
	std::array<std::unique_ptr<SpinBoxType>, N> spinboxes_;

	void setupColorPicker(PropertyBrowserItem* item);
	void updateColorPicker();
};

using Vec2fEditor = VecNTEditor<double, 2>;
using Vec3fEditor = VecNTEditor<double, 3>;
using Vec4fEditor = VecNTEditor<double, 4>;
using Vec2iEditor = VecNTEditor<int, 2>;
using Vec3iEditor = VecNTEditor<int, 3>;
using Vec4iEditor = VecNTEditor<int, 4>;

}  // namespace raco::property_browser
