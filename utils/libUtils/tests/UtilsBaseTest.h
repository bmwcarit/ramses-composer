/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "utils/FileUtils.h"

using raco::utils::u8path;

class UtilsBaseTest : public testing::Test {
public:
	std::string test_case_name() const {
		return ::testing::UnitTest::GetInstance()->current_test_info()->name();
	}

	std::string test_suite_name() const {
		return ::testing::UnitTest::GetInstance()->current_test_info()->test_suite_name();
	}

	u8path test_relative_path() const {
		return u8path{test_suite_name()} / test_case_name();
	}

	u8path test_path() const {
		return std::filesystem::current_path() / test_relative_path();
	}

protected:
	void SetUp() override {
		if (std::filesystem::exists(test_path())) {
			// Debugging case: if we debug and kill the test before completion the test directory will not be cleaned by TearDown
			std::filesystem::remove_all(test_path());
		}
		std::filesystem::create_directories(test_path());
	}

	void TearDown() override {
		std::filesystem::remove_all(test_path());
	}
};
