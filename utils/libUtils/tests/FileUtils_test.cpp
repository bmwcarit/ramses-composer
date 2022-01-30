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

class FileUtilsTest : public testing::Test {
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
		return (std::filesystem::current_path() / test_relative_path());
	}

	std::vector<u8path> testFileNames = {
		test_path() / "test",
		test_path() / u8"äöüÄÖÜáàÁÀÇß",
		test_path() / u8"滴滴启动纽交所退市",
		test_path() / u8"滴滴启动纽" / u8"äöüÄÖÜáàÁÀÇß"
	};

	std::string testFileContents = u8"test\täöüÄÖÜáàÁÀÇß 滴滴启动纽交所退市 🐌🌝👍🏼t";

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

TEST_F(FileUtilsTest, writeReadFileTest) {
	ASSERT_TRUE(test_path().exists());

	std::vector<unsigned char> testFileContentsBinary(testFileContents.begin(), testFileContents.end());

	for (const auto& testFileName : testFileNames) {
		file::write(testFileName, testFileContents);
		ASSERT_EQ(file::read(testFileName), testFileContents);

		auto readBinaryData = file::readBinary(testFileName);
		ASSERT_EQ(readBinaryData.size(), testFileContentsBinary.size());
		for (int i = 0; i < readBinaryData.size(); ++i) {
			ASSERT_EQ(readBinaryData[i], testFileContentsBinary[i]);
		}
	}
}