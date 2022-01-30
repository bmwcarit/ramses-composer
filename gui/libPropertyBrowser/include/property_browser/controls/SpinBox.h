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

#include "property_browser/PropertyBrowserLayouts.h"
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QKeyEvent>
#include <optional>

namespace raco::property_browser {

template <typename T>
struct SpinBoxTraits {
	typedef QSpinBox BaseType;
};

template <>
struct SpinBoxTraits<double> {
	typedef QDoubleSpinBox BaseType;
};

template <>
struct SpinBoxTraits<int> {
	typedef QSpinBox BaseType;
};

template <typename T>
std::optional<T> evaluateLuaExpression(QString expression);

template <typename T>
class InternalSpinBox : public SpinBoxTraits<T>::BaseType {
public:
	InternalSpinBox(QWidget* parent = nullptr) : SpinBoxTraits<T>::BaseType(parent) {
		this->setKeyboardTracking(false);
		this->setCorrectionMode(QAbstractSpinBox::CorrectionMode::CorrectToNearestValue);
	}

	virtual ~InternalSpinBox() {}
	QValidator::State validate(QString& input, int& pos) const override {
		return QValidator::Acceptable;
	}

	QString textFromValue(T value) const {
		return SpinBoxTraits<T>::BaseType::textFromValue(value);
	}

	T valueFromText(const QString& text) const {
		return tryGetValueFromText(text).value_or(SpinBoxTraits<T>::BaseType::value());
	}

protected:
	T focusInOldValue_;

	void focusInEvent(QFocusEvent* event) {
		this->selectAll();
		focusInOldValue_ = SpinBoxTraits<T>::BaseType::value();
		SpinBoxTraits<T>::BaseType::focusInEvent(event);
	}

	void keyPressEvent(QKeyEvent* event) {
		SpinBoxTraits<T>::BaseType::keyPressEvent(event);

		if (event->key() == Qt::Key_Escape) {
			SpinBoxTraits<T>::BaseType::setValue(focusInOldValue_);
			SpinBoxTraits<T>::BaseType::clearFocus();
		}
	}
	
	std::optional<T> tryGetValueFromText(const QString& text) const {
		return evaluateLuaExpression<T>(text);
	}
};

template <>
inline QString InternalSpinBox<double>::textFromValue(double value) const {
	return QLocale(QLocale::C).toString(value, 'f', QLocale::FloatingPointShortest);
};

template <typename T>
class SpinBox : public QWidget {
public:
	SpinBox(QWidget* parent = nullptr) : QWidget{parent} {
		valueChangedConnection_ = QObject::connect(&widget_, qOverload<T>(&SpinBoxTraits<T>::BaseType::valueChanged), this, [this]() { emitValueChanged(widget_.value()); });
		QObject::connect(&widget_, &SpinBoxTraits<T>::BaseType::editingFinished, this, [this]() { emitEditingFinished(); });
		layout_.addWidget(&widget_);
		widget_.setRange(min_, max_);
		// Disable QSpinBox sizing based on range
		widget_.setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
		setFocusPolicy(Qt::FocusPolicy::StrongFocus);
		setFocusProxy(&widget_);
	}

	void setValue(T v) {
		QObject::disconnect(valueChangedConnection_);
		widget_.setValue(v);
		valueChangedConnection_ = QObject::connect(&widget_, qOverload<T>(&SpinBoxTraits<T>::BaseType::valueChanged), this, [this]() { emitValueChanged(widget_.value()); });
	}

	int outOfRange() const noexcept {
		return false;
		// todo: disabled range check until we have full range handling
		//return value() < min_ ? -1 : (value() > max_ ? 1 : 0);
	}

	T value() const noexcept {
		return widget_.value();
	}

	void setRange(T min, T max) {
		min_ = min;
		max_ = max;
	}

	void keyPressEvent(QKeyEvent* event) {
		QWidget::keyPressEvent(event);

		if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
			widget_.clearFocus();
			emitFocusNextRequested();
		}
	}

protected:
	virtual void emitValueChanged(T value) = 0;
	virtual void emitEditingFinished() = 0;
	virtual void emitFocusNextRequested() = 0;

	InternalSpinBox<T> widget_{this};

private:
	QMetaObject::Connection valueChangedConnection_;
	PropertyBrowserGridLayout layout_{this};
	T min_{std::numeric_limits<T>::lowest()};
	T max_{std::numeric_limits<T>::max()};
};

class DoubleSpinBox final : public SpinBox<double> {
	Q_OBJECT
public:
	// Property used in RaCoStyle to draw error colors (careful we depend on hierarchy as the actual element which is drawen is SpinBox->QSpinBox->QLineEdit)
	Q_PROPERTY(int outOfRange READ outOfRange);
	DoubleSpinBox(QWidget* parent = nullptr);
Q_SIGNALS:
	void valueChanged(double val);
	void editingFinished();
	void focusNextRequested();

protected:
	void emitValueChanged(double value) override;
	void emitEditingFinished() override;
	void emitFocusNextRequested() override;
};

class IntSpinBox final : public SpinBox<int> {
	Q_OBJECT
public:
	// Property used in RaCoStyle to draw error colors (careful we depend on hierarchy as the actual element which is drawen is SpinBox->QSpinBox->QLineEdit)
	Q_PROPERTY(int outOfRange READ outOfRange);
	IntSpinBox(QWidget* parent = nullptr);
Q_SIGNALS:
	void valueChanged(int val);
	void editingFinished();
	void focusNextRequested();

protected:
	void emitValueChanged(int value) override;
	void emitEditingFinished() override;
	void emitFocusNextRequested() override;
};

}  // namespace raco::property_browser
