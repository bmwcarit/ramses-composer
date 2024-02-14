/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include <gtest/gtest.h>

#include "RamsesBaseFixture.h"
#include "ramses_adaptor/TimerAdaptor.h"

using namespace raco::user_types;

class TimerAdaptorTest : public RamsesBaseFixture<> {};

TEST_F(TimerAdaptorTest, defaultConstruction) {
	auto timer = context.createObject(Timer::typeDescription.typeName, "Timer");

	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<ramses::TimerNode>().size(), 1);
	ASSERT_EQ(sceneContext.logicEngine().getCollection<ramses::TimerNode>().begin()->getUserId(), timer->objectIDAsRamsesLogicID());
}

TEST_F(TimerAdaptorTest, renaming) {
	auto timer = context.createObject(Timer::typeDescription.typeName, "Timer");

	dispatch();

	context.set({timer, {"objectName"}}, std::string("Changed"));
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<ramses::TimerNode>().size(), 1);
	ASSERT_EQ(select<ramses::TimerNode>(sceneContext.logicEngine(), "Timer"), nullptr);
	ASSERT_NE(select<ramses::TimerNode>(sceneContext.logicEngine(), "Changed"), nullptr);
	ASSERT_EQ(select<ramses::TimerNode>(sceneContext.logicEngine(), "Changed")->getUserId(), timer->objectIDAsRamsesLogicID());
}

TEST_F(TimerAdaptorTest, multipleTimers) {
	auto timer = context.createObject(Timer::typeDescription.typeName, "Timer");
	dispatch();

	auto timer2 = context.createObject(Timer::typeDescription.typeName, "Timer2");
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<ramses::TimerNode>().size(), 2);
	ASSERT_NE(select<ramses::TimerNode>(sceneContext.logicEngine(), "Timer"), nullptr);
	ASSERT_EQ(select<ramses::TimerNode>(sceneContext.logicEngine(), "Timer")->getUserId(), timer->objectIDAsRamsesLogicID());

	ASSERT_NE(select<ramses::TimerNode>(sceneContext.logicEngine(), "Timer2"), nullptr);
	ASSERT_EQ(select<ramses::TimerNode>(sceneContext.logicEngine(), "Timer2")->getUserId(), timer2->objectIDAsRamsesLogicID());

	context.deleteObjects({timer2});
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<ramses::TimerNode>().size(), 1);
	ASSERT_NE(select<ramses::TimerNode>(sceneContext.logicEngine(), "Timer"), nullptr);
	ASSERT_EQ(select<ramses::TimerNode>(sceneContext.logicEngine(), "Timer2"), nullptr);
}

TEST_F(TimerAdaptorTest, InputZeroOutputNotZero) {
	auto timer{commandInterface.createObject(Timer::typeDescription.typeName)};

	dispatch();
	dispatch();
	dispatch();
	dispatch();

	ASSERT_EQ((ValueHandle{timer, {"inputs", "ticker_us"}}.asInt64()), 0);
	ASSERT_NE((ValueHandle{timer, {"outputs", "ticker_us"}}.asInt64()), 0);
}

TEST_F(TimerAdaptorTest, InputNotZeroGetsPropagated) {
	auto timer{commandInterface.createObject(Timer::typeDescription.typeName)};
	int64_t val = 233212;

	commandInterface.set({timer, {"inputs", "ticker_us"}}, val);
	dispatch();

	ASSERT_EQ((ValueHandle{timer, {"outputs", "ticker_us"}}.asInt64()), val);
}

TEST_F(TimerAdaptorTest, InputNotZeroBackToZeroOutputNotZero) {
	auto timer{commandInterface.createObject(Timer::typeDescription.typeName)};

	commandInterface.set({timer, {"inputs", "ticker_us"}}, int64_t{233212});
	dispatch();

	commandInterface.set({timer, {"inputs", "ticker_us"}}, int64_t{0});
	dispatch();

	ASSERT_NE((ValueHandle{timer, {"outputs", "ticker_us"}}.asInt64()), int64_t{0});
}

TEST_F(TimerAdaptorTest, InputBelowZeroNoError) {
	auto timer{commandInterface.createObject(Timer::typeDescription.typeName)};

	commandInterface.set({timer, {"inputs", "ticker_us"}}, int64_t{-1});
	dispatch();

	ASSERT_FALSE(commandInterface.errors().hasError(timer));
	ASSERT_EQ((ValueHandle{timer, {"outputs", "ticker_us"}}.asInt64()), int64_t{-1});
}

TEST_F(TimerAdaptorTest, InputNotIncreasingNoError) {
	auto timer{commandInterface.createObject(Timer::typeDescription.typeName)};

	commandInterface.set({timer, {"inputs", "ticker_us"}}, int64_t{2});
	dispatch();

	commandInterface.set({timer, {"inputs", "ticker_us"}}, int64_t{1});
	dispatch();

	ASSERT_FALSE(commandInterface.errors().hasError(timer));

	commandInterface.set({timer, {"inputs", "ticker_us"}}, int64_t{0});
	dispatch();
	ASSERT_FALSE(commandInterface.errors().hasError(timer));
}
