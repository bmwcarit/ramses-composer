/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "utils/stdfilesystem.h"
#include "utils/FileUtils.h"
#include "utils/u8path.h"
#include "core/Context.h"
#include "core/CommandInterface.h"
#include "core/PathManager.h"
#include "core/Project.h"
#include "core/Queries.h"
#include "core/Undo.h"

#include <gtest/gtest.h>

template <class BaseClass = ::testing::Test>
class RacoBaseTest : public BaseClass {
public:
	virtual std::string test_case_name() const {
		return ::testing::UnitTest::GetInstance()->current_test_info()->name();
	}

	virtual std::string test_suite_name() const {
		return ::testing::UnitTest::GetInstance()->current_test_info()->test_suite_name();
	}

	virtual raco::utils::u8path test_relative_path() const {
		return raco::utils::u8path{test_suite_name()} / test_case_name();
	}

	virtual raco::utils::u8path test_path() const {
		return raco::utils::u8path::current() / test_relative_path();
	}

	void checkUndoRedo(raco::core::CommandInterface& cmd, std::function<void()> operation, std::function<void()> preCheck, std::function<void()> postCheck) {
		preCheck();
		operation();
		postCheck();
		cmd.undoStack().undo();
		preCheck();
		cmd.undoStack().redo();
		postCheck();
	}

	template <std::size_t N>
	void checkUndoRedoMultiStep(raco::core::CommandInterface& cmd, std::array<std::function<void()>, N> operations, std::array<std::function<void()>, N + 1> checks) {
		// forward pass
		for (std::size_t i = 0; i < N; i++) {
			checks[i]();
			operations[i]();
		}
		checks.back()();

		// stepwise undo
		for (std::size_t i = 0; i < N; i++) {
			cmd.undoStack().undo();
			checks[N - i - 1]();
		}

		// stepwise redo
		for (std::size_t i = 0; i < N; i++) {
			cmd.undoStack().redo();
			checks[i + 1]();
		}
	}

	static void checkLinks(raco::core::Project& project, const std::vector<raco::core::Link>& refLinks) {
		EXPECT_EQ(refLinks.size(), project.links().size());
		for (const auto& refLink : refLinks) {
			auto projectLink = raco::core::Queries::getLink(project, refLink.endProp());
			EXPECT_TRUE(projectLink && projectLink->startProp() == refLink.startProp() && projectLink->isValid() == refLink.isValid());
		}
	}

protected:

	std::vector<std::string> split(const std::string& s, char delim) {
		std::stringstream sstream{s};
		std::vector<std::string> result;
		std::string word;
		while (std::getline(sstream, word, delim)) {
			result.emplace_back(word);
		}
		return result;
	}

	virtual void SetUp() override {
		if (std::filesystem::exists(test_path())) {
			// Debugging case: if we debug and kill the test before complition the test directory will not be cleaned by TearDown
			std::filesystem::remove_all(test_path());
		}
		std::filesystem::create_directories(test_path());

		// Setup fake directory hierachy to simulate different directories configfiles can live in.
		auto programPath = test_path() / "App" / "bin";
		auto appDataPath = test_path() / "AppData";
		raco::core::PathManager::init(programPath.string(), appDataPath.string());

#ifdef RACO_LOCAL_TEST_RESOURCES_FILE_LIST
		auto fileNames{split(RACO_LOCAL_TEST_RESOURCES_FILE_LIST, '!')};
		auto dirNames{split(RACO_LOCAL_TEST_RESOURCES_DIRECTORY_LIST, '!')};

		for (size_t index = 0; index < fileNames.size(); index++) {
			auto from{raco::utils::u8path(dirNames[index]) / fileNames[index]};
			auto to{test_path() / fileNames[index]};
			raco::utils::u8path toWithoutFilename{to};
			toWithoutFilename.remove_filename();
			std::filesystem::create_directories(toWithoutFilename);
			std::filesystem::copy(from, to);
		}
#endif
	}

	virtual void TearDown() override {
		std::filesystem::remove_all(test_path());
	}

	struct TextFile {
		TextFile(RacoBaseTest& test, std::string fileName, std::string contents) {
			path = test.test_path() / fileName;
			raco::utils::file::write(path.string(), contents);
		}

		operator std::string() const {
			return path.string();
		}

		raco::utils::u8path path;
	};

	TextFile makeFile(std::string fileName, std::string contents) {
		return TextFile(*this, fileName, contents);
	}
};
