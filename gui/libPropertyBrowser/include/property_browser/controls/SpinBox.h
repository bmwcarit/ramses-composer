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

#include "data_storage/Value.h"
#include "property_browser/PropertyBrowserLayouts.h"

#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QKeyEvent>
#include <optional>

namespace raco::property_browser {

template <typename T>
std::optional<T> evaluateLuaExpression(QString expression, T min = raco::data_storage::numericalLimitMin<T>(), T max = raco::data_storage::numericalLimitMax<T>());

template <typename T>
class InternalSpinBox : public QAbstractSpinBox {
public:
	InternalSpinBox(QWidget* parent, std::function<void(T)> valueEdited);

	QString textFromValue(T value) const;
	T valueFromText(const QString& text) const;

	T value() const;
	void setValue(T newValue);

	void setRange(T min, T max);

	void stepBy(int steps) override;
	QAbstractSpinBox::StepEnabled stepEnabled() const override;

protected:
	T value_;
	T min_;
	T max_;
	T focusInOldValue_;
	std::function<void(T)> valueEdited_;

	void focusInEvent(QFocusEvent* event);
	void keyPressEvent(QKeyEvent* event);
};

template <typename T>
class SpinBox : public QWidget {
public:
	SpinBox(QWidget* parent = nullptr);

	void setValue(T v);
	T value() const noexcept;

	int outOfRange() const noexcept;
	void setSoftRange(T min, T max);

	void keyPressEvent(QKeyEvent* event);

protected:
	virtual void emitvalueEdited(T value) = 0;
	virtual void emitEditingFinished() = 0;
	virtual void emitFocusNextRequested() = 0;

	InternalSpinBox<T> widget_;

private:
	PropertyBrowserGridLayout layout_;
};

class DoubleSpinBox final : public SpinBox<double> {
	Q_OBJECT
public:
	// Property used in RaCoStyle to draw error colors (careful we depend on hierarchy as the actual element which is drawen is SpinBox->QSpinBox->QLineEdit)
	Q_PROPERTY(int outOfRange READ outOfRange);
	DoubleSpinBox(QWidget* parent = nullptr);
Q_SIGNALS:
	void valueEdited(double val);
	void editingFinished();
	void focusNextRequested();

protected:
	void emitvalueEdited(double value) override;
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
	void valueEdited(int val);
	void editingFinished();
	void focusNextRequested();

protected:
	void emitvalueEdited(int value) override;
	void emitEditingFinished() override;
	void emitFocusNextRequested() override;
};

class Int64SpinBox final : public SpinBox<int64_t> {
	Q_OBJECT
public:
	// Property used in RaCoStyle to draw error colors (careful we depend on hierarchy as the actual element which is drawen is SpinBox->QSpinBox->QLineEdit)
	Q_PROPERTY(int outOfRange READ outOfRange);
	Int64SpinBox(QWidget* parent = nullptr);

Q_SIGNALS:
	void valueEdited(int64_t val);
	void editingFinished();
	void focusNextRequested();

protected:
	void emitvalueEdited(int64_t value) override;
	void emitEditingFinished() override;
	void emitFocusNextRequested() override;
};

}  // namespace raco::property_browser
