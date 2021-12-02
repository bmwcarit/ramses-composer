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

#include <QApplication>
#include <QMouseEvent>
#include <QObject>
#include <QPainter>
#include <QWidget>
#include <type_traits>

namespace raco::property_browser {

template <typename T>
class ScalarSlider : public QWidget {
public:
	explicit ScalarSlider(QWidget* parent = nullptr);
	T value() const noexcept;
	bool insideRange() const noexcept;
	
	void setRange(T min, T max);

protected:
	void slotSetValue(T value);
	void paintEvent(QPaintEvent* event) override;
	void enterEvent(QEvent* event) override;
	void leaveEvent(QEvent* event) override;
	void focusInEvent(QFocusEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	virtual void signalValueEdited(T value) = 0;
	virtual void signalValueChange(T value) = 0;
	virtual void signalSingleClicked() = 0;

private:
	void addValue(T step);
	T min_{static_cast<T>(0.0)};
	T max_{static_cast<T>(1.0)};
	T value_{static_cast<T>(0.5)};
	T stepSize_{static_cast<T>(std::is_integral<T>::value ? 1 : 0.1)};
	bool mouseIsDragging_{false};
	bool mouseIsInside_{false};
	QPoint mousePivot_;
	int mouseDraggingCurrentOffsetX_;
};

class DoubleSlider final : public ScalarSlider<double> {
	Q_OBJECT
public:
	explicit DoubleSlider(QWidget* parent) : ScalarSlider{parent} {}

Q_SIGNALS:
	void valueEdited(double value);
	void valueChanged(double value);
	void singleClicked();
public Q_SLOTS:
	void setValue(double v) { slotSetValue(v); }

protected:
	void signalValueEdited(double value) override {
		Q_EMIT valueEdited(value);
	}
	void signalValueChange(double value) override {
		Q_EMIT valueChanged(value);
	}
	void signalSingleClicked() override {
		Q_EMIT singleClicked();
	}
};

class IntSlider final : public ScalarSlider<int> {
	Q_OBJECT
public:
	explicit IntSlider(QWidget* parent) : ScalarSlider{parent} {}

Q_SIGNALS:
	void valueChanged(int value);
	void valueEdited(int value);
	void singleClicked();
public Q_SLOTS:
	void setValue(int v) { slotSetValue(v); }

protected:
	void signalValueEdited(int value) override {
		Q_EMIT valueEdited(value);
	}
	void signalValueChange(int value) override {
		Q_EMIT valueChanged(value);
	}
	void signalSingleClicked() override {
		Q_EMIT singleClicked();
	}
};

}  // namespace raco::property_browser
