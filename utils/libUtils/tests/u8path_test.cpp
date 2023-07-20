/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "gtest/gtest.h"

#include "utils/FileUtils.h"
#include "utils/u8path.h"
#include "UtilsBaseTest.h"

#include <filesystem>
#include <fstream>

using namespace raco::utils;

class u8pathTest : public UtilsBaseTest {
public:
	std::vector<u8path> testFileNames = {
		test_path() / "test",
		test_path() / u8"äöüÄÖÜáàÁÀÇß",
		test_path() / u8"滴滴启动纽交所退市"
	};
};

TEST_F(u8pathTest, existsFileTest) {
	ASSERT_TRUE(test_path().exists());

	for (const auto& testFileName : testFileNames) {
		auto testFileNameUppercase = testFileName.string();
		std::transform(testFileNameUppercase.begin(), testFileNameUppercase.end(), testFileNameUppercase.begin(), ::toupper);

		ASSERT_FALSE(testFileName.exists());
		ASSERT_FALSE(u8path(testFileNameUppercase).exists());
		ASSERT_FALSE(u8path(testFileNameUppercase).exists(true));
		ASSERT_FALSE(testFileName.userHasReadAccess());
		ASSERT_FALSE(testFileName.existsFile());
		ASSERT_FALSE(testFileName.existsDirectory());

		file::write(testFileName.string(), "");

		ASSERT_TRUE(testFileName.exists());
#if (!defined (__linux__))
		// Platform-dependent: on Windows the check is case-insensitive and returns true even for upper-case name.
		ASSERT_TRUE(u8path(testFileNameUppercase).exists());
#endif
		// Platform-independent case-sensitive check.
		ASSERT_FALSE(u8path(testFileNameUppercase).exists(true));
		ASSERT_TRUE(testFileName.userHasReadAccess());
		ASSERT_TRUE(testFileName.existsFile());
		ASSERT_FALSE(testFileName.existsDirectory());
	}
}

TEST_F(u8pathTest, existsDirTest) {
	ASSERT_TRUE(test_path().exists());

	for (const auto& testDirName : testFileNames) {
		auto testDirNameUppercase = testDirName.string();
		std::transform(testDirNameUppercase.begin(), testDirNameUppercase.end(), testDirNameUppercase.begin(), ::toupper);

		auto testDirSubfileName = testDirName / "test";

		auto testDirSubfileNameUppercase = testDirSubfileName.string();
		std::transform(testDirSubfileNameUppercase.begin(), testDirSubfileNameUppercase.end(), testDirSubfileNameUppercase.begin(), ::toupper);

		ASSERT_FALSE(testDirName.exists());
		ASSERT_FALSE(u8path(testDirNameUppercase).exists());
		ASSERT_FALSE(u8path(testDirNameUppercase).exists(true));
		ASSERT_FALSE(testDirSubfileName.exists());
		ASSERT_FALSE(u8path(testDirSubfileNameUppercase).exists());
		ASSERT_FALSE(u8path(testDirSubfileNameUppercase).exists(true));
		ASSERT_FALSE(testDirSubfileName.userHasReadAccess());
		ASSERT_FALSE(testDirName.existsFile());
		ASSERT_FALSE(testDirName.existsDirectory());
		ASSERT_FALSE(testDirSubfileName.existsFile());
		ASSERT_FALSE(testDirSubfileName.existsDirectory());

		file::write(testDirSubfileName.string(), "");

		ASSERT_TRUE(testDirName.exists());
#if (!defined (__linux__))
		// Platform-dependent: on Windows the check is case-insensitive and returns true even for upper-case name.
		ASSERT_TRUE(u8path(testDirNameUppercase).exists());
		ASSERT_TRUE(u8path(testDirSubfileNameUppercase).exists());
#endif
		// Platform-independent case-sensitive check.
		ASSERT_FALSE(u8path(testDirNameUppercase).exists(true));
		ASSERT_TRUE(testDirSubfileName.exists());
		// Platform-independent case-sensitive check.
		ASSERT_FALSE(u8path(testDirSubfileNameUppercase).exists(true));
		ASSERT_TRUE(testDirSubfileName.userHasReadAccess());
		ASSERT_FALSE(testDirName.existsFile());
		ASSERT_TRUE(testDirName.existsDirectory());
		ASSERT_TRUE(testDirSubfileName.existsFile());
		ASSERT_FALSE(testDirSubfileName.existsDirectory());
	}
}