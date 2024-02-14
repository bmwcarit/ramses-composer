/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ErrorView_test.h"

#include "user_types/Node.h"

TEST_F(ErrorViewTest, ErrorViewWorksWithObjectValueHandle) {
	auto node = commandInterface.createObject(user_types::Node::typeDescription.typeName);
	commandInterface.errors().addError(core::ErrorCategory::GENERAL, core::ErrorLevel::ERROR, {node}, "Object Error");
	dataChangeDispatcher_->dispatch(recorder);
	// no crash
	ASSERT_EQ(commandInterface.errors().getAllErrors().size(), 1);

	commandInterface.errors().removeError({node});
	dataChangeDispatcher_->dispatch(recorder);
	ASSERT_TRUE(commandInterface.errors().getAllErrors().empty());
}

TEST_F(ErrorViewTest, ErrorViewWorksWithPropertyValueHandle) {
	auto node = commandInterface.createObject(user_types::Node::typeDescription.typeName);
	commandInterface.errors().addError(core::ErrorCategory::GENERAL, core::ErrorLevel::ERROR, core::ValueHandle{node}.get("rotation"), "Property Error");
	dataChangeDispatcher_->dispatch(recorder);
	// no crash
	ASSERT_EQ(commandInterface.errors().getAllErrors().size(), 1);

	commandInterface.errors().removeError(core::ValueHandle{node}.get("rotation"));
	dataChangeDispatcher_->dispatch(recorder);
	ASSERT_TRUE(commandInterface.errors().getAllErrors().empty());
}

TEST_F(ErrorViewTest, ErrorViewWorksWithProjectGlobalValueHandle) {
	commandInterface.errors().addError(core::ErrorCategory::GENERAL, core::ErrorLevel::ERROR, core::ValueHandle{}, "Project-Global Error");
	dataChangeDispatcher_->dispatch(recorder);
	// no crash
	ASSERT_EQ(commandInterface.errors().getAllErrors().size(), 1);

	commandInterface.errors().removeError(core::ValueHandle{});
	dataChangeDispatcher_->dispatch(recorder);
	ASSERT_TRUE(commandInterface.errors().getAllErrors().empty());
}