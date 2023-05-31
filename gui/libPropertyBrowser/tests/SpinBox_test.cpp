/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <cmath>

#include "property_browser/PropertyBrowserModel.h"
#include "property_browser/controls/SpinBox.h"
#include "property_browser/editors/Int64Editor.h"
#include "testing/TestEnvironmentCore.h"
#include "user_types/Timer.h"

#include <QApplication>

class SpinBoxFixture : public TestEnvironmentCore {};

using namespace raco::property_browser;

TEST_F(SpinBoxFixture, evaluateLuaExpressionDouble) {
	ASSERT_DOUBLE_EQ(12.345, evaluateLuaExpression<double>("12.345").value_or(0));
	ASSERT_DOUBLE_EQ(-12.345, evaluateLuaExpression<double>("-12.345").value_or(0));
	ASSERT_DOUBLE_EQ(12.345, evaluateLuaExpression<double>("12,345").value_or(0));
	ASSERT_DOUBLE_EQ(-12.345, evaluateLuaExpression<double>("-12,345").value_or(0));
	ASSERT_DOUBLE_EQ(0.345, evaluateLuaExpression<double>(".345").value_or(0));
	ASSERT_DOUBLE_EQ(-0.345, evaluateLuaExpression<double>("-.345").value_or(0));
	ASSERT_DOUBLE_EQ(-0.345 * 2, evaluateLuaExpression<double>("-.345 * 2").value_or(0));
	ASSERT_DOUBLE_EQ(4 + -0.345 * 2, evaluateLuaExpression<double>("4 + -.345 * 2").value_or(0));
	ASSERT_DOUBLE_EQ(1 - -0.345 / 2, evaluateLuaExpression<double>("1 - -.345 / 2 ").value_or(0));
	ASSERT_DOUBLE_EQ(-0.345 * -0.345, evaluateLuaExpression<double>("(-.345) ^ 2").value_or(0));

	// Forbidden elements are thousand seperators, invalid numeric expressions and function calls to the standard library.
	ASSERT_FALSE(evaluateLuaExpression<double>("1.000,0").has_value());
	ASSERT_FALSE(evaluateLuaExpression<double>("1,000.0").has_value());
	ASSERT_FALSE(evaluateLuaExpression<double>("1abc").has_value());
	ASSERT_FALSE(evaluateLuaExpression<double>("print('TEST') or 15").has_value());
	ASSERT_FALSE(evaluateLuaExpression<double>("math.sin(1.2)").has_value());

	// Limitation: Lua is not type safe, so strings are valid numbers.
	ASSERT_DOUBLE_EQ(1.02, evaluateLuaExpression<double>("'1.0' .. '2'").value_or(0));

	// As an undocumented feature, it is still possible to use lambda calculus to compute arbitary expressions. Since "," is always replaced by ".", function currying (e.g. max 1 argument per function) is required. 
	// To demonstrate computation capabilities, here we compute PI using the Nilakantha series.	
	ASSERT_NEAR(M_PI, evaluateLuaExpression<double>("(function(f) return 3 + f(f)(2) end)(function (f) return function(n) if (n < 100) then return 4 / (n * (n + 1) * (n + 2)) * -((-1)^(n/2)) + f(f)(n + 2) else return 0 end end end)").value_or(0), 0.0001);

	// If the user enters an infinite loop, program excecution should be aborted without a result after some time.
	ASSERT_FALSE(evaluateLuaExpression<double>("(function() while(true)do end end)()").has_value());
}

TEST_F(SpinBoxFixture, evaluateLuaExpressionInteger) {
	ASSERT_EQ(12, evaluateLuaExpression<int>("12").value_or(0));
	ASSERT_EQ(-12, evaluateLuaExpression<int>("-12").value_or(0));
	ASSERT_EQ(3 + 4 * 5, evaluateLuaExpression<int>("3 + 4 * 5").value_or(0));
	ASSERT_EQ(3 - 45 % 6, evaluateLuaExpression<int>("3 - 45 % 6").value_or(0));

	// Rounding will aways happen by truncation to integers.
	ASSERT_EQ(12, evaluateLuaExpression<int>("12.345").value_or(0));
	ASSERT_EQ(-12, evaluateLuaExpression<int>("-12.345").value_or(0));
	ASSERT_EQ(12, evaluateLuaExpression<int>("12.567").value_or(0));
	ASSERT_EQ(-12, evaluateLuaExpression<int>("-12.567").value_or(0));

	// Rounding happens after computation, integer division needs to happen explicitly.
	ASSERT_EQ((int)(1.0 / 2.0 * 4.0), evaluateLuaExpression<int>("1 / 2 * 4").value_or(0));
	ASSERT_EQ(1 / 2 * 4, evaluateLuaExpression<int>("1 // 2 * 4").value_or(0));
		
	// Forbidden elements are thousand seperators, invalid numeric expressions and function calls to the standard library.
	ASSERT_FALSE(evaluateLuaExpression<int>("1.000,0").has_value());
	ASSERT_FALSE(evaluateLuaExpression<int>("1,000.0").has_value());
	ASSERT_FALSE(evaluateLuaExpression<int>("1abc").has_value());
	ASSERT_FALSE(evaluateLuaExpression<int>("print('TEST') or 15").has_value());
	ASSERT_FALSE(evaluateLuaExpression<int>("math.sin(1.2)").has_value());

	// Limitation: Lua is not type safe, so strings are valid numbers.
	ASSERT_EQ(12, evaluateLuaExpression<int>("'1' .. '2'").value_or(0));

	// If the user enters an infinite loop, program excecution should be aborted without a result after some time.
	ASSERT_FALSE(evaluateLuaExpression<int>("(function() while(true)do end end)()").has_value());
}
