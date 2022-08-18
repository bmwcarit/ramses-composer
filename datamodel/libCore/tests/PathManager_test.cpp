/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/PathManager.h"

#include "testing/RacoBaseTest.h"

#include "gtest/gtest.h"

using namespace raco::core;

class PathManagerTest : public RacoBaseTest<> {
public:
	raco::utils::u8path test_relative_path() const override {
		return raco::utils::u8path{test_suite_name()} / test_case_name() / u8"滴滴启动纽交所退市 äüöß";
	}
};

TEST_F(PathManagerTest, MigrationOldConfigfilesDoIfNewNotPresent) {
	auto programPath = test_path() / "App" / "bin";
	auto appDataPath = test_path() / "AppData";
	auto legacyConfigFiles = test_path() / "configfiles";

	std::filesystem::remove_all(test_path());
	std::filesystem::create_directories(programPath);
	std::filesystem::create_directories(legacyConfigFiles);

	auto layout = raco::utils::u8path(makeFile("configfiles/layout.ini", ""));
	auto preferences = raco::utils::u8path(makeFile("configfiles/preferences.ini", ""));
	auto log1 = raco::utils::u8path(makeFile("configfiles/log1.log", ""));
	auto log2 = raco::utils::u8path(makeFile("configfiles/log2.log", ""));

	ASSERT_FALSE(appDataPath.existsDirectory());
	ASSERT_TRUE(legacyConfigFiles.existsDirectory());
	ASSERT_TRUE(layout.existsFile());
	ASSERT_TRUE(preferences.existsFile());
	ASSERT_TRUE(log1.existsFile());
	ASSERT_TRUE(log2.existsFile());

	PathManager::init(programPath.string(), appDataPath.string());

	ASSERT_FALSE(legacyConfigFiles.existsDirectory());
	ASSERT_FALSE(layout.existsFile());
	ASSERT_FALSE(preferences.existsFile());
	ASSERT_FALSE(log1.existsFile());
	ASSERT_FALSE(log2.existsFile());
	ASSERT_TRUE(appDataPath.existsDirectory());
	ASSERT_TRUE((appDataPath / "layout.ini").existsFile());
	ASSERT_TRUE((appDataPath / "preferences.ini").existsFile());
	ASSERT_TRUE((appDataPath / "logs/log1.log").existsFile());
	ASSERT_TRUE((appDataPath / "logs/log2.log").existsFile());
}

TEST_F(PathManagerTest, MigrationOldConfigfilesDoIfNewPresentEmptyFolder) {
	auto programPath = test_path() / "App" / "bin";
	auto appDataPath = test_path() / "AppData";
	auto legacyConfigFiles = test_path() / "configfiles";

	std::filesystem::remove_all(test_path());
	std::filesystem::create_directories(appDataPath);
	std::filesystem::create_directories(programPath);
	std::filesystem::create_directories(legacyConfigFiles);

	auto layout = raco::utils::u8path(makeFile("configfiles/layout.ini", ""));
	auto preferences = raco::utils::u8path(makeFile("configfiles/preferences.ini", ""));
	auto log1 = raco::utils::u8path(makeFile("configfiles/log1.log", ""));
	auto log2 = raco::utils::u8path(makeFile("configfiles/log2.log", ""));

	ASSERT_TRUE(appDataPath.existsDirectory());
	ASSERT_TRUE(legacyConfigFiles.existsDirectory());
	ASSERT_TRUE(layout.existsFile());
	ASSERT_TRUE(preferences.existsFile());
	ASSERT_TRUE(log1.existsFile());
	ASSERT_TRUE(log2.existsFile());

	PathManager::init(programPath.string(), appDataPath.string());

	ASSERT_FALSE(legacyConfigFiles.existsDirectory());
	ASSERT_FALSE(layout.existsFile());
	ASSERT_FALSE(preferences.existsFile());
	ASSERT_FALSE(log1.existsFile());
	ASSERT_FALSE(log2.existsFile());

	ASSERT_TRUE(appDataPath.existsDirectory());
	ASSERT_TRUE((appDataPath / "layout.ini").existsFile());
	ASSERT_TRUE((appDataPath / "preferences.ini").existsFile());
	ASSERT_TRUE((appDataPath / "logs/log1.log").existsFile());
	ASSERT_TRUE((appDataPath / "logs/log2.log").existsFile());
}

TEST_F(PathManagerTest, MigrationOldConfigfilesDoNotIfNewAlreadyPresent) {
	auto programPath = test_path() / "App" / "bin";
	auto appDataPath = test_path() / "AppData";
	auto legacyConfigFiles = test_path() / "configfiles";

	std::filesystem::remove_all(test_path());
	std::filesystem::create_directories(appDataPath);
	std::filesystem::create_directories(programPath);
	std::filesystem::create_directories(legacyConfigFiles);

	makeFile("AppData/custom.ini", "");

	auto layout = raco::utils::u8path(makeFile("configfiles/layout.ini", ""));
	auto preferences = raco::utils::u8path(makeFile("configfiles/preferences.ini", ""));
	auto log1 = raco::utils::u8path(makeFile("configfiles/log1.log", ""));
	auto log2 = raco::utils::u8path(makeFile("configfiles/log2.log", ""));

	ASSERT_TRUE(appDataPath.existsDirectory());
	ASSERT_TRUE(legacyConfigFiles.existsDirectory());
	ASSERT_TRUE(layout.existsFile());
	ASSERT_TRUE(preferences.existsFile());
	ASSERT_TRUE(log1.existsFile());
	ASSERT_TRUE(log2.existsFile());

	PathManager::init(programPath.string(), appDataPath.string());

	ASSERT_TRUE(appDataPath.existsDirectory());
	ASSERT_TRUE(legacyConfigFiles.existsDirectory());
	ASSERT_TRUE(layout.existsFile());
	ASSERT_TRUE(preferences.existsFile());
	ASSERT_TRUE(log1.existsFile());
	ASSERT_TRUE(log2.existsFile());

	ASSERT_TRUE((appDataPath / "custom.ini").existsFile());
	ASSERT_FALSE((appDataPath / "layout.ini").existsFile());
	ASSERT_FALSE((appDataPath / "preferences.ini").existsFile());
	ASSERT_FALSE((appDataPath / "logs/log1.log").existsFile());
	ASSERT_FALSE((appDataPath / "logs/log2.log").existsFile());
}