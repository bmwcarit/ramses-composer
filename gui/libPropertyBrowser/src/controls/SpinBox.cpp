/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "property_browser/controls/SpinBox.h"

#include "property_browser/PropertyBrowserItem.h"

#include "log_system/log.h"

#include "lapi.h"
#include "lauxlib.h"
#include "lualib.h"
#include "style/RaCoStyle.h"
#include <QApplication>
#include <QLineEdit>

namespace raco::property_browser {

const int EVALUATE_LUA_EXECUTION_LIMIT = 1000;

template <typename T>
InternalSpinBox<T>::InternalSpinBox(QWidget* parent, std::function<void(T)> valueEdited, std::function<void()> saveFocusInValue, std::function<void()> restoreFocusInValue)
	: QAbstractSpinBox(parent),
	  value_(),
	  min_(data_storage::numericalLimitMin<T>()),
	  max_(data_storage::numericalLimitMax<T>()),
	  valueEditedFunc_(valueEdited),
	  saveFocusInValueFunc_(saveFocusInValue),
	  restoreFocusInValueFunc_(restoreFocusInValue) {
	this->setKeyboardTracking(false);
	this->setCorrectionMode(QAbstractSpinBox::CorrectionMode::CorrectToNearestValue);
	connect(lineEdit(), &QLineEdit::editingFinished, this, [this]() {
		// Emit change event only if the value obtained from the current text
		// - is neither multiple value nor an invalid lua expression
		// and
		// - is different from the current value
		// This is ensured implicitly by the valueFromText function which uses the current value_
		// if either of the conditions above is false.
		auto newValue = valueFromText(lineEdit()->text());
		if (value_ != newValue) {
			setValue(newValue);
			valueEditedFunc_(newValue);
		}
	});

	// setting RaCoStyle to set Shift modifier for extended value step
	this->setStyle(QApplication::style());
}

template <typename T>
QString InternalSpinBox<T>::textFromValue(T value) const {
	return QLocale(QLocale::C).toString(value);
}

template <typename T>
T InternalSpinBox<T>::valueFromText(const QString& text) const {
	return evaluateLuaExpression<T>(text, min_, max_).value_or(value());
}

template <typename T>
T InternalSpinBox<T>::value() const {
	return value_;
}

template <typename T>
void InternalSpinBox<T>::setValue(T newValue) {
	newValue = std::clamp(newValue, min_, max_);

	lineEdit()->setText(textFromValue(newValue));
	value_ = newValue;
}

template <typename T>
void InternalSpinBox<T>::setMultipleValues() const {
	lineEdit()->setText(PropertyBrowserItem::MultipleValueText);
}

template <typename T>
bool InternalSpinBox<T>::hasMultipleValues() const {
	return lineEdit()->text() == PropertyBrowserItem::MultipleValueText;
}

template <typename T>
void InternalSpinBox<T>::setRange(T min, T max) {
	min_ = min;
	max_ = max;
}

template <typename T>
void InternalSpinBox<T>::stepBy(int steps) {
	auto newValue = valueFromText(lineEdit()->text()) + steps;
	setValue(newValue);
	valueEditedFunc_(newValue);
}

template <typename T>
QAbstractSpinBox::StepEnabled InternalSpinBox<T>::stepEnabled() const {
	QAbstractSpinBox::StepEnabled flags;
	// TODO wrong:
	// - ignores lua expression value
	// - should really compare: value_ + step < max to ensure we can't leave the range by stepping
	// currently irrelevant though since setting the range is still disabled in the SpinBox class below
	if (value_ != max_) {
		flags |= StepUpEnabled;
	}
	if (value_ != min_) {
		flags |= StepDownEnabled;
	}
	return flags;
}

template <typename T>
void InternalSpinBox<T>::focusInEvent(QFocusEvent* event) {
	this->selectAll();
	saveFocusInValueFunc_();
	QAbstractSpinBox::focusInEvent(event);
}

template <typename T>
void InternalSpinBox<T>::keyPressEvent(QKeyEvent* event) {
	QAbstractSpinBox::keyPressEvent(event);

	if (event->key() == Qt::Key_Escape) {
		restoreFocusInValueFunc_();
		QAbstractSpinBox::clearFocus();
	}
}
template <typename T>
SpinBox<T>::SpinBox(QWidget* parent)
	: QWidget(parent),
	  widget_(
		  this,
		  [this](T newValue) { emitvalueEdited(newValue); },
		  [this]() { emitSaveFocusInValue(); },
		  [this]() { emitRestoreFocusInValue(); }),
	  layout_(this) {
	QObject::connect(&widget_, &QAbstractSpinBox::editingFinished, this, [this]() { emitEditingFinished(); });
	layout_.addWidget(&widget_);

	// Disable QSpinBox sizing based on range
	widget_.setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	setFocusPolicy(Qt::FocusPolicy::StrongFocus);
	setFocusProxy(&widget_);
}

template <typename T>
void SpinBox<T>::setValue(T v) {
	widget_.setValue(v);
}

template <typename T>
void SpinBox<T>::setMultipleValues() {
	widget_.setMultipleValues();
}

template <typename T>
bool SpinBox<T>::hasMultipleValues() {
	return widget_.hasMultipleValues();
}

template <typename T>
int SpinBox<T>::outOfRange() const noexcept {
	return false;
	// TODO: disabled range check until we have full range handling
	// return value() < min_ ? -1 : (value() > max_ ? 1 : 0);
}

template <typename T>
void SpinBox<T>::setSoftRange(T min, T max) {
	// TODO: disabled range check until we have full range handling
	// widget_.setRange(min, max);
}

template <typename T>
T SpinBox<T>::value() const noexcept {
	return widget_.value();
}

template <typename T>
void SpinBox<T>::keyPressEvent(QKeyEvent* event) {
	QWidget::keyPressEvent(event);

	if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
		widget_.clearFocus();
		emitFocusNextRequested();
	}
}

template <>
QString InternalSpinBox<double>::textFromValue(double value) const {
	auto DECIMALS = 5;
	return QLocale(QLocale::C).toString(value, 'f', DECIMALS);
};

DoubleSpinBox::DoubleSpinBox(QWidget* parent) : SpinBox<double>(parent) {
}

void DoubleSpinBox::emitvalueEdited(double value) {
	Q_EMIT valueEdited(value);
}

void DoubleSpinBox::emitEditingFinished() {
	Q_EMIT editingFinished();
}

void DoubleSpinBox::emitSaveFocusInValue() {
	Q_EMIT saveFocusInValue();
}

void DoubleSpinBox::emitRestoreFocusInValue() {
	Q_EMIT restoreFocusInValue();
}

void DoubleSpinBox::emitFocusNextRequested() {
	Q_EMIT focusNextRequested();
}

IntSpinBox::IntSpinBox(QWidget* parent) : SpinBox<int>(parent) {}

void IntSpinBox::emitvalueEdited(int value) {
	Q_EMIT valueEdited(value);
}

void IntSpinBox::emitEditingFinished() {
	Q_EMIT editingFinished();
}

void IntSpinBox::emitSaveFocusInValue() {
	Q_EMIT saveFocusInValue();
}

void IntSpinBox::emitRestoreFocusInValue() {
	Q_EMIT restoreFocusInValue();
}

void IntSpinBox::emitFocusNextRequested() {
	Q_EMIT focusNextRequested();
}

Int64SpinBox::Int64SpinBox(QWidget* parent) : SpinBox<int64_t>(parent) {}

void Int64SpinBox::emitvalueEdited(int64_t value) {
	Q_EMIT valueEdited(value);
}

void Int64SpinBox::emitSaveFocusInValue() {
	Q_EMIT saveFocusInValue();
}

void Int64SpinBox::emitRestoreFocusInValue() {
	Q_EMIT restoreFocusInValue();
}

void Int64SpinBox::emitEditingFinished() {
	Q_EMIT editingFinished();
}

void Int64SpinBox::emitFocusNextRequested() {
	Q_EMIT focusNextRequested();
}

template <typename T>
std::optional<T> evaluateLuaExpression(QString expression, T min, T max) {
	// Support german language decimal seperators by just replacing them with dots.
	// This can invalidate fancy lua expressions, but these should not be used anyway by users.
	expression.replace(',', '.');

	lua_State* l = lua_open();

	// Limit number of instructions executed, to avoid infinite loops hanging the program.
	lua_sethook(
		l, [](lua_State* l, lua_Debug* d) { luaL_error(l, "Maximum instruction excecution limit exceeded."); }, LUA_MASKCOUNT, EVALUATE_LUA_EXECUTION_LIMIT);

	if (!luaL_dostring(l, ("return " + expression.toStdString()).c_str())) {
		auto result = lua_tonumber(l, -1);
		if (result < min) {
			LOG_INFO(log_system::PROPERTY_BROWSER, "Lua result {} is under numerical limit {} and will be clamped.", result, min);
			result = min;
		} else if (result > max) {
			LOG_INFO(log_system::PROPERTY_BROWSER, "Lua result {} is over numerical limit {} and will be clamped.", result, max);
			result = max;
		}

		lua_close(l);
		return (T)result;
	} else {
		LOG_INFO(log_system::PROPERTY_BROWSER, "Could not evaluate Lua Expression: {}", lua_tostring(l, -1));
		lua_close(l);
		return {};
	}
};

template class SpinBox<double>;
template class SpinBox<int>;
template class SpinBox<int64_t>;

template std::optional<double> evaluateLuaExpression<double>(QString expression, double min, double max);
template std::optional<int> evaluateLuaExpression<int>(QString expression, int min, int max);
template std::optional<int64_t> evaluateLuaExpression<int64_t>(QString expression, int64_t min, int64_t max);

}  // namespace raco::property_browser