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

#include "core/Queries.h"
#include "property_browser/controls/SpinBox.h"
#include "property_browser/PropertyBrowserItem.h"
#include "property_browser/PropertyBrowserLayouts.h"
#include "style/Colors.h"
#include <QPalette>
#include <QWidget>
#include <type_traits>
#include <array>

namespace raco::property_browser {

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
class VecNTEditor final : public QWidget {
	typedef typename VecNTEditorTraits<T>::SpinBoxType SpinBoxType;

public:
	explicit VecNTEditor(
		PropertyBrowserItem* item,
		QWidget* parent = nullptr)
		: QWidget{parent} {
		static_assert(std::is_floating_point<T>::value || std::is_integral<T>::value, "VecNTEditor requires floating point or integral type");
		auto* layout = new PropertyBrowserGridLayout{this};
		for (int i = 0; i < N; i++) {
			spinboxes_[i].reset(new SpinBoxType{this});
			if (auto rangeAnnotation =  item->children().at(i)->query<raco::data_storage::RangeAnnotation<T>>()) {
				spinboxes_[i]->setRange(*rangeAnnotation->min_, *rangeAnnotation->max_);
			}
			spinboxes_[i]->setValue(item->children().at(i)->valueHandle().as<T>());
			QObject::connect(spinboxes_[i].get(), &SpinBoxType::valueChanged, item, [item, i](T value) { item->children().at(i)->set(value); });
			QObject::connect(item->children().at(i), &PropertyBrowserItem::valueChanged, spinboxes_[i].get(), [this, i](raco::core::ValueHandle& handle) {
				spinboxes_[i]->setValue(handle.as<T>());
			});

			if (i < N - 1) {
				int nextSpinboxIndex = i + 1;
				QObject::connect(spinboxes_[i].get(), &SpinBoxType::focusNextRequested, this, [this, nextSpinboxIndex]() {
					spinboxes_[nextSpinboxIndex]->setFocus();
				});			
			}

			layout->addWidget(spinboxes_[i].get(), 0, i);
		}
		QObject::connect(item, &PropertyBrowserItem::childrenChanged, item, [this](const QList<PropertyBrowserItem*>& children) {
			for (int i = 0; i < N; i++) {
				spinboxes_[i]->setValue(children.at(i)->valueHandle().as<T>());
			}
		});			

	}
public Q_SLOTS:
    void setEnabled(bool val) {
		for(auto* child : layout()->children()) {
			if(auto* widget { dynamic_cast<QWidget*>(child)}) {
				widget->setEnabled(val);
			}
		}
	}

 protected:
	std::array<std::unique_ptr<SpinBoxType>, N> spinboxes_;
};

using Vec2fEditor = VecNTEditor<double, 2>;
using Vec3fEditor = VecNTEditor<double, 3>;
using Vec4fEditor = VecNTEditor<double, 4>;
using Vec2iEditor = VecNTEditor<int, 2>;
using Vec3iEditor = VecNTEditor<int, 3>;
using Vec4iEditor = VecNTEditor<int, 4>;

}  // namespace raco::property_browser
