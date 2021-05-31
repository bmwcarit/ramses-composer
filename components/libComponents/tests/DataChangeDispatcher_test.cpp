/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include <core/Project.h>

#include "core/Errors.h"
#include <ramses_base/HeadlessEngineBackend.h>

#include "user_types/UserObjectFactory.h"
#include "user_types/Node.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <components/DataChangeDispatcher.h>

#include <memory>

using namespace raco::user_types;
using namespace raco::components;
using namespace raco::core;
TEST(DataChangeDispatcher, constructor) {
	DataChangeRecorder recorder {};
	DataChangeDispatcher underTest{};
}

TEST(DataChangeDispatcher, dispatchEmitsUpdatedOnValueHandleObservable) {
	DataChangeRecorder recorder {};
	Errors errors{&recorder};
	DataChangeDispatcher underTest{};
	raco::ramses_base::HeadlessEngineBackend backend{};
	BaseContext context{new Project{}, backend.coreInterface(), &UserObjectFactory::getInstance(), &recorder, &errors};
	SEditorObject node = std::make_shared<Node>();
	ValueHandle valueHandle{node, {"translation", "x"}};

	testing::MockFunction<void()> callback{};
	EXPECT_CALL(callback, Call()).Times(1);

	auto subscribtion = underTest.registerOn(valueHandle, callback.AsStdFunction());

	// TEST
	context.set(valueHandle, 1.0);
	underTest.dispatch(recorder.release());
	
	testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST(DataChangeDispatcher, dispatchDoesntEmitUpdatedOnUnrelatedValueHandleObservable) {
	DataChangeRecorder recorder{};
	Errors errors{&recorder};
	DataChangeDispatcher underTest{};
	raco::ramses_base::HeadlessEngineBackend backend{};
	BaseContext context{new Project{}, backend.coreInterface(), &UserObjectFactory::getInstance(), &recorder, &errors};
	SEditorObject node = std::make_shared<Node>();
	ValueHandle valueHandle{node, {"translation", "x"}};
	ValueHandle unrelatedHandle{node, {"translation", "y"}};

	testing::MockFunction<void()> callback{};
	EXPECT_CALL(callback, Call()).Times(0);

	auto subscribtion = underTest.registerOn(unrelatedHandle, callback.AsStdFunction());

	// TEST
	context.set(valueHandle, 1.0);
	underTest.dispatch(recorder.release());

	testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST(DataChangeDispatcher, dispatchDoesntEmitUpdatedForDestroyedSubscribtion) {
	DataChangeRecorder recorder{};
	Errors errors{&recorder};
	DataChangeDispatcher underTest{};
	raco::ramses_base::HeadlessEngineBackend backend{};
	BaseContext context{new Project{}, backend.coreInterface(), &UserObjectFactory::getInstance(), &recorder, &errors};
	SEditorObject node = std::make_shared<Node>();
	ValueHandle valueHandle{node, {"translation", "x"}};

	testing::MockFunction<void()> callback{};
	EXPECT_CALL(callback, Call()).Times(0);

	{
		auto subscribtion = underTest.registerOn(valueHandle, callback.AsStdFunction());
	}

	// TEST
	context.set(valueHandle, 1.0);
	underTest.dispatch(recorder.release());

	testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST(DataChangeDispatcher, dispatchEmitUpdatedMoreComplexCase) {
	DataChangeRecorder recorder{};
	Errors errors{&recorder};
	DataChangeDispatcher underTest{};
	raco::ramses_base::HeadlessEngineBackend backend{};
	BaseContext context{new Project{}, backend.coreInterface(), &UserObjectFactory::getInstance(), &recorder, &errors};
	SEditorObject node = std::make_shared<Node>();
	ValueHandle valueHandle{node, {"translation", "x"}};

	testing::MockFunction<void()> callback1{};
	EXPECT_CALL(callback1, Call()).Times(1);
	testing::MockFunction<void()> callback2{};
	EXPECT_CALL(callback2, Call()).Times(0);

	auto subscribtion1 = underTest.registerOn(valueHandle, callback1.AsStdFunction());
	{
		auto subscribtion2 = underTest.registerOn(valueHandle, callback2.AsStdFunction());
	}

	// TEST
	context.set(valueHandle, 1.0);
	underTest.dispatch(recorder.release());

	testing::Mock::VerifyAndClearExpectations(&callback1);
	testing::Mock::VerifyAndClearExpectations(&callback2);
}

TEST(DataChangeDispatcher, registerOnChildren_dispatchEmitUpdated) {
	DataChangeRecorder recorder{};
	Errors errors{&recorder};
	DataChangeDispatcher underTest{};
	raco::ramses_base::HeadlessEngineBackend backend{};
	BaseContext context{new Project{}, backend.coreInterface(), &UserObjectFactory::getInstance(), &recorder, &errors};
	SEditorObject node = std::make_shared<Node>();
	ValueHandle translation{node, {"translation"}};

	ValueHandle x { translation.get("x") };
	testing::MockFunction<void(ValueHandle)> callback1{};
	EXPECT_CALL(callback1, Call(x)).Times(1);
	testing::MockFunction<void(ValueHandle)> callback2{};
	EXPECT_CALL(callback2, Call(x)).Times(0);

	auto subscribtion1 = underTest.registerOnChildren(translation, callback1.AsStdFunction());
	{
		auto subscribtion2 = underTest.registerOnChildren(translation, callback2.AsStdFunction());
	}

	// TEST
	context.set(x, 1.0);
	underTest.dispatch(recorder.release());

	testing::Mock::VerifyAndClearExpectations(&callback1);
	testing::Mock::VerifyAndClearExpectations(&callback2);
}
