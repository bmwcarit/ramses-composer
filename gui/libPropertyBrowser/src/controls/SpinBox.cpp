/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "property_browser/controls/SpinBox.h"

#include "log_system/log.h"

#include "lualib.h"
#include "lauxlib.h"
#include "lapi.h"

namespace raco::property_browser {

const int EVALUATE_LUA_EXECUTION_LIMIT = 1000;

raco::property_browser::IntSpinBox::IntSpinBox(QWidget* parent) : SpinBox<int>{parent} {}

void raco::property_browser::IntSpinBox::emitValueChanged(int value) {
	Q_EMIT valueChanged(value);
}

void raco::property_browser::IntSpinBox::emitEditingFinished() {
	Q_EMIT editingFinished();
}

void raco::property_browser::IntSpinBox::emitFocusNextRequested() {
	Q_EMIT focusNextRequested();
}

raco::property_browser::DoubleSpinBox::DoubleSpinBox(QWidget* parent) : SpinBox<double>{parent} {
	widget_.setDecimals(5);
}

void raco::property_browser::DoubleSpinBox::emitValueChanged(double value) {
	Q_EMIT valueChanged(value);
}

void raco::property_browser::DoubleSpinBox::emitEditingFinished() {
	Q_EMIT editingFinished();
}

void raco::property_browser::DoubleSpinBox::emitFocusNextRequested() {
	Q_EMIT focusNextRequested();
}


template <typename T>
std::optional<T> evaluateLuaExpression(QString expression) {
	// Support german language decimal seperators by just replacing them with dots.
	// This can invalidate fancy lua expressions, but these should not be used anyway by users.
	expression.replace(',', '.');

	lua_State* l = lua_open();

	// Limit number of instructions executed, to avoid infinite loops hanging the program.
	lua_sethook(
		l, [](lua_State* l, lua_Debug* d) { luaL_error(l, "Maximum instruction excecution limit exceeded."); }, LUA_MASKCOUNT, EVALUATE_LUA_EXECUTION_LIMIT);

	if (!luaL_dostring(l, ("return " + expression.toStdString()).c_str())) {
		T result;
		if constexpr (std::is_same<T, double>::value) {
			result = lua_tonumber(l, -1);
		} else {
			result = lua_tointeger(l, -1);
		}
		lua_close(l);
		return result;
	} else {
		LOG_INFO(raco::log_system::PROPERTY_BROWSER, "Could not evaluate Lua Expression: {}", lua_tostring(l, -1));
		lua_close(l);
		return {};
	}
};

template std::optional<int> evaluateLuaExpression<int>(QString expression);

template std::optional<double> evaluateLuaExpression<double>(QString expression);

}  // namespace raco::property_browser