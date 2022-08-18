/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include <core/Project.h>

#include "core/Errors.h"

#include "user_types/UserObjectFactory.h"
#include "user_types/Node.h"
#include "testing/TestEnvironmentCore.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <components/DataChangeDispatcher.h>
#include <ramses_base/HeadlessEngineBackend.h>

#include <memory>

using namespace raco::user_types;
using namespace raco::components;
using namespace raco::core;

class DataChangeDispatcherTest : public TestEnvironmentCore {
protected:
	DataChangeDispatcher underTest{};
};


TEST_F(DataChangeDispatcherTest, constructor) {
	DataChangeRecorder recorder {};
	DataChangeDispatcher underTest{};
}

TEST_F(DataChangeDispatcherTest, dispatchEmitsUpdatedOnValueHandleObservable) {
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

TEST_F(DataChangeDispatcherTest, dispatchDoesntEmitUpdatedOnUnrelatedValueHandleObservable) {
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

TEST_F(DataChangeDispatcherTest, dispatchDoesntEmitUpdatedForDestroyedSubscribtion) {
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

TEST_F(DataChangeDispatcherTest, dispatchEmitUpdatedMoreComplexCase) {
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

TEST_F(DataChangeDispatcherTest, registerOnChildren_dispatchEmitUpdated) {
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

class LifeCycleListenertest : public DataChangeDispatcherTest {
public:
	LifeCycleListenertest() {
		node_start = std::make_shared<Node>();
		node_end = std::make_shared<Node>();
	}

	void test_add_remove(const LinkDescriptor& linkDesc, int expected_count) {
		// test recordAddLink
		EXPECT_CALL(creationCallback, Call(linkDesc)).Times(expected_count);
		EXPECT_CALL(deletionCallback, Call(linkDesc)).Times(0);

		recorder.recordAddLink(linkDesc);
		underTest.dispatch(recorder.release());

		testing::Mock::VerifyAndClearExpectations(&creationCallback);
		testing::Mock::VerifyAndClearExpectations(&deletionCallback);

		// test recordRemoveLInk
		EXPECT_CALL(creationCallback, Call(linkDesc)).Times(0);
		EXPECT_CALL(deletionCallback, Call(linkDesc)).Times(expected_count);

		recorder.recordRemoveLink(linkDesc);
		underTest.dispatch(recorder.release());

		testing::Mock::VerifyAndClearExpectations(&creationCallback);
		testing::Mock::VerifyAndClearExpectations(&deletionCallback);
	}

	SEditorObject node_start;
	SEditorObject node_end;

	testing::MockFunction<void(const raco::core::LinkDescriptor&)> creationCallback{};
	testing::MockFunction<void(const raco::core::LinkDescriptor&)> deletionCallback{};
};

TEST_F(LifeCycleListenertest, generic) {
	LinkDescriptor linkDesc{{node_start, {"translation"}}, {node_end, {"scaling"}}, true};
	auto subscribtion = underTest.registerOnLinksLifeCycle(creationCallback.AsStdFunction(), deletionCallback.AsStdFunction());
	test_add_remove(linkDesc, 1);
}

TEST_F(LifeCycleListenertest, for_start_link_same_start) {
	LinkDescriptor linkDesc{{node_start, {"translation"}}, {node_end, {"scaling"}}, true};
	auto subscribtion = underTest.registerOnLinksLifeCycleForStart(node_start, creationCallback.AsStdFunction(), deletionCallback.AsStdFunction());
	test_add_remove(linkDesc, 1);
}

TEST_F(LifeCycleListenertest, for_start_link_different_start) {
	LinkDescriptor linkDesc{{node_start, {"translation"}}, {node_end, {"scaling"}}, true};
	auto subscribtion = underTest.registerOnLinksLifeCycleForStart(node_end, creationCallback.AsStdFunction(), deletionCallback.AsStdFunction());
	test_add_remove(linkDesc, 0);
}

TEST_F(LifeCycleListenertest, for_end_link_same_end) {
	LinkDescriptor linkDesc{{node_start, {"translation"}}, {node_end, {"scaling"}}, true};
	auto subscribtion = underTest.registerOnLinksLifeCycleForEnd(node_end, creationCallback.AsStdFunction(), deletionCallback.AsStdFunction());
	test_add_remove(linkDesc, 1);
}

TEST_F(LifeCycleListenertest, for_end_link_different_end) {
	LinkDescriptor linkDesc{{node_start, {"translation"}}, {node_end, {"scaling"}}, true};
	auto subscribtion = underTest.registerOnLinksLifeCycleForEnd(node_start, creationCallback.AsStdFunction(), deletionCallback.AsStdFunction());
	test_add_remove(linkDesc, 0);
}
