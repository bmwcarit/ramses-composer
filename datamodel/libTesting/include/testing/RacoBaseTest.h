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
#include "core/Context.h"
#include "core/CommandInterface.h"
#include "core/Undo.h"

#include <gtest/gtest.h>

template <class BaseClass = ::testing::Test>
class RacoBaseTest : public BaseClass {
public:
	virtual std::string cwd() const {
		return cwd_path().u8string();
	}


	virtual std::string test_case_name() const {
		return ::testing::UnitTest::GetInstance()->current_test_info()->name();
	}

	virtual std::string test_suite_name() const {
		return ::testing::UnitTest::GetInstance()->current_test_info()->test_suite_name();
	}

	virtual std::filesystem::path cwd_path_relative() const {
		return std::filesystem::path{test_suite_name()} / test_case_name();
	}

	virtual std::filesystem::path cwd_path() const {
		return (std::filesystem::current_path() / cwd_path_relative());
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


protected:
	virtual void SetUp() override {
		if (std::filesystem::exists(cwd())) {
			// Debugging case: if we debug and kill the test before complition the test directory will not be cleaned by TearDown
			std::filesystem::remove_all(cwd());
		}
		std::filesystem::create_directories(cwd());
#ifdef RACO_LOCAL_TEST_RESOURCES_FILE_LIST
		std::string input{RACO_LOCAL_TEST_RESOURCES_FILE_LIST};
		std::stringstream ss{input};
		std::string fileName{};

		// Convention Fileseparator: '!'
		while (std::getline(ss, fileName, '!')) {
			const std::filesystem::path from{std::filesystem::path{RACO_LOCAL_TEST_RESOURCES_SOURCE_DIRECTORY}.append(fileName)};
			const std::filesystem::path to{cwd_path().append(fileName)};
			std::filesystem::path toWithoutFilename{to};
			toWithoutFilename.remove_filename();
			std::filesystem::create_directories(toWithoutFilename);
			std::filesystem::copy(from, to);
		}
#endif
	}

	virtual void TearDown() override {
		std::filesystem::remove_all(cwd());
	}

	struct TextFile {
		TextFile(RacoBaseTest& test, std::string fileName, std::string contents) {
			path = test.cwd_path() / fileName;
			raco::utils::file::write(path.string(), contents);
		}

		operator std::string() const {
			return path.string();
		}

		std::filesystem::path path;
	};

	TextFile makeFile(std::string fileName, std::string contents) {
		return TextFile(*this, fileName, contents);
	}
};
