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
#include "gtest/gtest.h"

#include "common_widgets/ErrorView.h"
#include "testing/TestEnvironmentCore.h"

#include <QApplication>

class ErrorViewTest : public TestEnvironmentCoreT<::testing::Test, QApplication> {
	
protected:
	components::SDataChangeDispatcher dataChangeDispatcher_{std::make_shared<components::DataChangeDispatcher>()};
	common_widgets::ErrorView errorView_{&commandInterface, dataChangeDispatcher_, true, nullptr};
};
