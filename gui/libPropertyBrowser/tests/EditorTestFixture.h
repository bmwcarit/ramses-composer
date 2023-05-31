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

#include "property_browser/PropertyBrowserModel.h"
#include "components/DataChangeDispatcher.h"
#include "core/UserObjectFactoryInterface.h"
#include "testing/TestEnvironmentCore.h"
#include <QApplication>

template <typename T>
class EditorTestFixtureT : public TestEnvironmentCoreT<T> {
public:
	using DataChangeDispatcher = raco::components::DataChangeDispatcher;
	int argc{0};
	QApplication application{argc, nullptr};
	std::shared_ptr<DataChangeDispatcher> dataChangeDispatcher;
	raco::property_browser::PropertyBrowserModel model;

	EditorTestFixtureT() : TestEnvironmentCoreT<T>{}, dataChangeDispatcher{std::make_shared<DataChangeDispatcher>()} {}
	EditorTestFixtureT(raco::core::UserObjectFactoryInterface* objectFactory) : TestEnvironmentCoreT<T>{objectFactory}, dataChangeDispatcher{std::make_shared<DataChangeDispatcher>()} {}

	void dispatch() {
		dataChangeDispatcher->dispatch(this->recorder.release());
		application.processEvents();
	}
};

using EditorTestFixture = EditorTestFixtureT<::testing::Test>;
