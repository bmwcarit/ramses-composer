/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "gtest/gtest.h"

#include "utils/FileUtils.h"
#include "utils/u8path.h"
#include "utils/stdfilesystem.h"

#include <fstream>

using namespace raco::utils;

class u8pathTest : public testing::Test {
public:
	virtual std::string test_case_name() const {
		return ::testing::UnitTest::GetInstance()->current_test_info()->name();
	}

	virtual std::string test_suite_name() const {
		return ::testing::UnitTest::GetInstance()->current_test_info()->test_suite_name();
	}

	virtual u8path test_relative_path() const {
		return u8path{test_suite_name()} / test_case_name();
	}

	virtual u8path test_path() const {
		return std::filesystem::current_path() / test_relative_path();
	}

	std::vector<u8path> testFileNames = {
		test_path() / "test",
		test_path() / u8"äöüÄÖÜáàÁÀÇß",
		test_path() / u8"滴滴启动纽交所退市"
	};

protected:
	virtual void SetUp() override {
		if (std::filesystem::exists(test_path())) {
			// Debugging case: if we debug and kill the test before complition the test directory will not be cleaned by TearDown
			std::filesystem::remove_all(test_path());
		}
		std::filesystem::create_directories(test_path());
	}

	virtual void TearDown() override {
		std::filesystem::remove_all(test_path());
	}
};

TEST_F(u8pathTest, existsFileTest) {
	ASSERT_TRUE(test_path().exists());

	for (const auto& testFileName : testFileNames) {
		ASSERT_FALSE(testFileName.exists());
		ASSERT_FALSE(testFileName.userHasReadAccess());
		ASSERT_FALSE(testFileName.existsFile());
		ASSERT_FALSE(testFileName.existsDirectory());

		file::write(testFileName.string(), "");

		ASSERT_TRUE(testFileName.exists());
		ASSERT_TRUE(testFileName.userHasReadAccess());
		ASSERT_TRUE(testFileName.existsFile());
		ASSERT_FALSE(testFileName.existsDirectory());
	}
}

TEST_F(u8pathTest, existsDirTest) {
	ASSERT_TRUE(test_path().exists());

	for (const auto& testDirName : testFileNames) {
		auto testDirSubfileName = testDirName / "test";

		ASSERT_FALSE(testDirName.exists());
		ASSERT_FALSE(testDirSubfileName.exists());
		ASSERT_FALSE(testDirSubfileName.userHasReadAccess());
		ASSERT_FALSE(testDirName.existsFile());
		ASSERT_FALSE(testDirName.existsDirectory());
		ASSERT_FALSE(testDirSubfileName.existsFile());
		ASSERT_FALSE(testDirSubfileName.existsDirectory());

		file::write(testDirSubfileName.string(), "");

		ASSERT_TRUE(testDirName.exists());
		ASSERT_TRUE(testDirSubfileName.exists());
		ASSERT_TRUE(testDirSubfileName.userHasReadAccess());
		ASSERT_FALSE(testDirName.existsFile());
		ASSERT_TRUE(testDirName.existsDirectory());
		ASSERT_TRUE(testDirSubfileName.existsFile());
		ASSERT_FALSE(testDirSubfileName.existsDirectory());
	}
}