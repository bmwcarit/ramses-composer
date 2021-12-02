/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/controls/ScalarSlider.h"
#include "style/Colors.h"
#include "style/Icons.h"
#include "style/RaCoStyle.h"

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWidget>
#include <type_traits>

namespace raco::property_browser {

using namespace raco::style;

template <typename T>
ScalarSlider<T>::ScalarSlider(QWidget* parent)
	: QWidget{parent} {
	static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value, "ScalarSlider requires integral or floating point type");

	// adjust size to LineEdits
	setMaximumSize(QWIDGETSIZE_MAX, 22);
	setFocusPolicy(Qt::FocusPolicy::StrongFocus);
	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
}

template <typename T>
T ScalarSlider<T>::value() const noexcept {
	return value_;
}

template <typename T>
bool ScalarSlider<T>::insideRange() const noexcept {
	return true;
	// todo: disabled range check until we have full range handling
	//return min_ <= value_ && max_ >= value_;
}

template <typename T>
void ScalarSlider<T>::setRange(T min, T max) {
	min_ = min;
	max_ = max;
	update();
}

template <typename T>
void ScalarSlider<T>::slotSetValue(T v) {
	if (value_ != v) {
		value_ = v;
		update();
		signalValueChange(v);
	}
}

template <typename T>
void ScalarSlider<T>::paintEvent(QPaintEvent* event) {
	QPainter painter{this};

	QPalette pal{palette()};
	// We are handling our own colorstate
	pal.setCurrentColorGroup(QPalette::ColorGroup::Active);

	auto backgroundBrush = pal.base();
	auto sliderBrush = pal.highlight();
	sliderBrush.setColor(sliderBrush.color().darker());

	if (!insideRange()) {
		sliderBrush.setColor(Colors::color(Colormap::errorColor));
		backgroundBrush.setColor(Colors::color(Colormap::errorColorDark));
	}
	if (!isEnabled()) {
		sliderBrush.setColor(sliderBrush.color().darker());
		backgroundBrush.setColor(Colors::color(Colormap::grayEditDisabled));
		painter.setPen(Colors::color(Colormap::textDisabled));
	} else if (hasFocus()) {
		sliderBrush.setColor(sliderBrush.color().lighter());		
	}

	if (const RaCoStyle* roundStyle = qobject_cast<const RaCoStyle*>(style())) {
		roundStyle->drawRoundedRect(rect(), &painter, backgroundBrush);

		auto valueRect{rect()};
		valueRect.setWidth(((static_cast<double>(value_) - min_) / (max_ - min_)) * valueRect.width());

		painter.setClipRect(valueRect);
		roundStyle->drawRoundedRect(rect(), &painter, sliderBrush);
		painter.setClipRect(rect());
	}

	QString text = std::is_integral<T>::value ? QString::number(value_) : QLocale(QLocale::C).toString(static_cast<double>(value_), 'f', 5);
	painter.drawText(rect(), Qt::AlignHCenter | Qt::AlignVCenter, text);

	if (mouseIsInside_ && !mouseIsDragging_) {
		painter.drawPixmap(rect().x() - 4, rect().y() - 2, Icons::pixmap(Pixmap::decrement));
		painter.drawPixmap(rect().width() - rect().height(), rect().y() - 2, Icons::pixmap(Pixmap::increment));
	}
}

template <typename T>
void ScalarSlider<T>::focusInEvent(QFocusEvent* event) {
	if (event->reason() == Qt::FocusReason::TabFocusReason || event->reason() == Qt::FocusReason::BacktabFocusReason) {
		signalSingleClicked();
	}
}

template <typename T>
void ScalarSlider<T>::enterEvent(QEvent* event) {
	if (isEnabled()) {
		mouseIsInside_ = true;
		update();
		setCursor(Qt::SizeHorCursor);
		QWidget::enterEvent(event);
	}
}

template <typename T>
void ScalarSlider<T>::leaveEvent(QEvent* event) {
	mouseIsInside_ = false;
	update();
	setCursor(Qt::ArrowCursor);
	QWidget::leaveEvent(event);
}

template <typename T>
void ScalarSlider<T>::mousePressEvent(QMouseEvent* event) {
	if (isEnabled()) {
		setFocus(Qt::FocusReason::MouseFocusReason);
		mousePivot_ = event->globalPos();
		mouseDraggingCurrentOffsetX_ = 0;
		setCursor(Qt::CursorShape::BlankCursor);
		update();
	}
}

template <typename T>
void ScalarSlider<T>::addValue(T step) {
	T newValue = value() + step;
	slotSetValue(newValue);
	signalValueEdited(newValue);
}

template <typename T>
void ScalarSlider<T>::mouseReleaseEvent(QMouseEvent* event) {
	if (!mouseIsDragging_) {
		if (event->localPos().x() < rect().x() + 24l) {
			addValue(-1);
		} else if (event->localPos().x() > rect().right() - 24l) {
			addValue(1);
		} else {
			signalSingleClicked();
		}
	}
	mouseIsDragging_ = false;
	setCursor(Qt::CursorShape::SizeHorCursor);
	clearFocus();
	update();
}

template <typename T>
void ScalarSlider<T>::mouseMoveEvent(QMouseEvent* event) {
	if (hasFocus()) {
		mouseIsDragging_ = true;
		mouseDraggingCurrentOffsetX_ += event->globalPos().x() - mousePivot_.x();
		cursor().setPos(mousePivot_.x(), mousePivot_.y());
		
		double widgetFraction = (mouseDraggingCurrentOffsetX_ / static_cast<double>(rect().width()));
		T newValue = value() + static_cast<T>((max_ - min_) * widgetFraction);
		if (newValue != value()) {
			mouseDraggingCurrentOffsetX_ -= static_cast<int>(widgetFraction * rect().width());
			slotSetValue(newValue);
			signalValueEdited(newValue);
		}
	}
}

template class ScalarSlider<double>;
template class ScalarSlider<int>;

}  // namespace raco::property_browser
