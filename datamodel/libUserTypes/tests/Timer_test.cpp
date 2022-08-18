/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "testing/TestEnvironmentCore.h"
#include "user_types/LuaScript.h"
#include "user_types/Timer.h"
#include <gtest/gtest.h>

using namespace raco::core;
using namespace raco::user_types;

class TimerTest : public TestEnvironmentCore {};

TEST_F(TimerTest, unlinkedTimerGetsDeletedByUnusedResourcesDelete) {
	auto timer{commandInterface.createObject(Timer::typeDescription.typeName)};

	ASSERT_EQ(commandInterface.deleteUnreferencedResources(), 1);
}

TEST_F(TimerTest, linkedTimerGetsDeletedByUnusedResourcesDelete_EndPoint) {
	auto timer{commandInterface.createObject(Timer::typeDescription.typeName)};
	auto script{commandInterface.createObject(LuaScript::typeDescription.typeName)};

	ValueHandle uriHandle{script, {"uri"}};
	commandInterface.set(uriHandle, test_path().append("scripts/types-scalar.lua").string());
	commandInterface.addLink({script, {"outputs", "ointeger64"}}, {timer, {"inputs", "ticker_us"}});

	ASSERT_EQ(commandInterface.deleteUnreferencedResources(), 1);
}

TEST_F(TimerTest, linkedTimerDoesNotGetDeletedByUnusedResourcesDelete_StartPoint) {
	auto timer{commandInterface.createObject(Timer::typeDescription.typeName)};
	auto script{commandInterface.createObject(LuaScript::typeDescription.typeName)};

	ValueHandle uriHandle{script, {"uri"}};
	commandInterface.set(uriHandle, test_path().append("scripts/types-scalar.lua").string());
	commandInterface.addLink({timer, {"outputs", "ticker_us"}}, {script, {"inputs", "integer64"}});

	ASSERT_EQ(commandInterface.deleteUnreferencedResources(), 0);
}
