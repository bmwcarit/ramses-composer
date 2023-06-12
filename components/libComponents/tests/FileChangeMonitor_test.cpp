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

#include "gtest/gtest.h"

#include "components/FileChangeMonitorImpl.h"
#include "testing/TestEnvironmentCore.h"
#include "utils/u8path.h"
#include "utils/FileUtils.h"

#include <fstream>

using namespace raco::core;

class BasicFileChangeMonitorTest : public TestEnvironmentCore {
protected:
	bool waitForFileChangeCounterGEq(int count, int timeOutInMS = 2000) {
		auto start = std::chrono::steady_clock::now();
		do {
			// Programmatic file/directory modifications need to be explicitly processed in a Qt event loop (QCoreApplication).

			// Briefly yield, so the QFileSystemWatcher has time to react to our file changes and
			// add its event to the event loop.
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			// Process the event the QFileSystemWatcher queued. This will cause the FileChangeListener
			// to create a timer.
			QCoreApplication::processEvents();

			// Wait for the timer to queue its event, and then process the timer event, which eventually
			// leads to the callbacks registered with FileMonitor::registerFileChangedHandler to be called.
			std::this_thread::sleep_for(std::chrono::milliseconds(raco::components::FileChangeListenerImpl::DELAYED_FILE_LOAD_TIME_MSEC + 100));
			QCoreApplication::processEvents();			
		}
		while (fileChangeCounter_ < count && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() <= timeOutInMS);
		
		return fileChangeCounter_ == count;
	}

	int argc = 0;
	// Apparently QCoreApplication needs to be initialized before the ProjectFileChangeMonitor is created, since that 
	// will create signal connections which don't work on Linux otherwise (but they do work on Windows).
	QCoreApplication eventLoop_{argc, nullptr};

	int fileChangeCounter_{0};
	std::function<void(void)> testCallback_ = [this]() { ++fileChangeCounter_; };
	std::unique_ptr<raco::components::ProjectFileChangeMonitor> testFileChangeMonitor_ = std::make_unique<raco::components::ProjectFileChangeMonitor>();
	std::vector<raco::components::ProjectFileChangeMonitor::UniqueListener> createdFileListeners_;
};

class FileChangeMonitorTest : public BasicFileChangeMonitorTest {
protected:
	static constexpr const char* TEST_RESOURCES_FOLDER_NAME = "testresources";
	static constexpr const char* TEST_FILE_NAME = "test.txt";

	void SetUp() override {
		TestEnvironmentCore::SetUp();
		std::filesystem::create_directory(testFolderPath_);

		raco::utils::file::write(testFilePath_, {});

		createdFileListeners_.emplace_back(testFileChangeMonitor_->registerFileChangedHandler(testFilePath_.string(), testCallback_));
	}

	void TearDown() override {
		if (testFilePath_.exists()) {
			std::filesystem::permissions(testFilePath_, std::filesystem::perms::all);
		}
		if (testFolderPath_.exists()) {
			std::filesystem::permissions(testFolderPath_, std::filesystem::perms::all);
		}
		if (test_path().exists()) {
			std::filesystem::permissions(test_path(), std::filesystem::perms::all);
		}
		TestEnvironmentCore::TearDown();
	}

	void runRenamingRoutine(raco::utils::u8path& originPath, const char* firstRename, const char* secondRename) {
		auto newFilePath = originPath;
		newFilePath.replace_filename(firstRename);

		auto evenNewerFilePath = testFilePath_;
		evenNewerFilePath.replace_filename(secondRename);

		std::filesystem::rename(testFilePath_, newFilePath);
		ASSERT_TRUE(waitForFileChangeCounterGEq(1));

		std::filesystem::rename(newFilePath, evenNewerFilePath);
		ASSERT_TRUE(waitForFileChangeCounterGEq(1));

		std::filesystem::rename(evenNewerFilePath, testFilePath_);
		ASSERT_TRUE(waitForFileChangeCounterGEq(2));

		std::filesystem::rename(testFilePath_, newFilePath);
		ASSERT_TRUE(waitForFileChangeCounterGEq(3));
	}

	raco::utils::u8path testFolderPath_{test_path().append(TEST_RESOURCES_FOLDER_NAME)};
	raco::utils::u8path testFilePath_{raco::utils::u8path(testFolderPath_).append(TEST_FILE_NAME)};
};

TEST_F(FileChangeMonitorTest, InstantiationNoFileChange) {
	ASSERT_EQ(fileChangeCounter_, 0);
}

TEST_F(FileChangeMonitorTest, FileModificationCreation) {
	testFilePath_ = raco::utils::u8path(testFolderPath_).append("differentFile.txt");
	createdFileListeners_.emplace_back(testFileChangeMonitor_->registerFileChangedHandler(testFilePath_.string(), testCallback_));
	ASSERT_TRUE(waitForFileChangeCounterGEq(0));

	raco::utils::file::write(testFilePath_, {});

	ASSERT_TRUE(waitForFileChangeCounterGEq(1));
}


TEST_F(FileChangeMonitorTest, FileModificationEditing) {
	raco::utils::file::write(testFilePath_, "Test");

	ASSERT_TRUE(waitForFileChangeCounterGEq(1));
}


TEST_F(FileChangeMonitorTest, FileModificationDeletion) {
	std::filesystem::remove(testFilePath_);
	ASSERT_TRUE(waitForFileChangeCounterGEq(1));
}


TEST_F(FileChangeMonitorTest, FileModificationRenaming) {
	runRenamingRoutine(testFilePath_, "new.txt", "evenNewer.txt");	
}


TEST_F(FileChangeMonitorTest, FileModificationMultipleModificationsAtTheSameTime) {
	std::ofstream testFileOutputStream(testFilePath_.string(), std::ios_base::out);
	testFileOutputStream << "Test";

	std::ofstream otherOutputStream = std::ofstream(testFilePath_.string(), std::ios_base::app);
	otherOutputStream << "Other";
	otherOutputStream.close();

	testFileOutputStream.close();

	// Linux gives sometimes one, sometimes two events.
	ASSERT_TRUE(waitForFileChangeCounterGEq(1));
}

#if (!defined(__linux))
// For some reason in Linux we can still change the file even after we set the permissions to "owner_read" only.
// So skip this test in Linux for now.
TEST_F(FileChangeMonitorTest, FileModificationSetAndTryToEditReadOnly) {
	std::error_code ec;
	std::filesystem::permissions(testFilePath_, std::filesystem::perms::owner_read, ec);
	ASSERT_TRUE(!ec) << "Failed to set permissons. Error code: " << ec.value() << " Error message: '" << ec.message() << "'";

	// fileChangeCounter_ will be 0 in WSL, but the proper value in Linux container
	ASSERT_TRUE(waitForFileChangeCounterGEq(1));

	std::ofstream testFileOutputStream(testFilePath_.string(), std::ios_base::out);
	ASSERT_FALSE(testFileOutputStream.is_open());

	ASSERT_TRUE(waitForFileChangeCounterGEq(1));
}
#endif


TEST_F(FileChangeMonitorTest, FolderModificationDeletion) {
	std::filesystem::remove_all(test_path().append(TEST_RESOURCES_FOLDER_NAME));
	ASSERT_TRUE(waitForFileChangeCounterGEq(1));
}


TEST_F(FileChangeMonitorTest, FolderModificationRenaming) {
	runRenamingRoutine(testFolderPath_, "newFolder", "evenNewerFolder");
}

TEST_F(BasicFileChangeMonitorTest, create_file_existing_directory) {
	auto testFolderPath = test_path() / "test-folder";
	std::string test_file_name = "test.txt";

	std::filesystem::create_directory(testFolderPath);

	createdFileListeners_.emplace_back(testFileChangeMonitor_->registerFileChangedHandler((testFolderPath / test_file_name).string(), testCallback_));
	EXPECT_EQ(fileChangeCounter_, 0);

	raco::utils::file::write(testFolderPath / test_file_name, {});
	ASSERT_TRUE(waitForFileChangeCounterGEq(1));
}

TEST_F(BasicFileChangeMonitorTest, create_file_no_directory) {
	auto testFolderPath = test_path() / "test-folder";
	std::string test_file_name = "test.txt";

	createdFileListeners_.emplace_back(testFileChangeMonitor_->registerFileChangedHandler((testFolderPath / test_file_name).string(), testCallback_));
	EXPECT_EQ(fileChangeCounter_, 0);

	std::filesystem::create_directory(testFolderPath);
	ASSERT_TRUE(waitForFileChangeCounterGEq(0));

	raco::utils::file::write(testFolderPath / test_file_name, {});
	ASSERT_TRUE(waitForFileChangeCounterGEq(1));
}

TEST_F(FileChangeMonitorTest, directory_rename_disappearance) {
	auto movedFolderPath = test_path() / "moved-directory";
	std::filesystem::rename(testFolderPath_, movedFolderPath);
	ASSERT_TRUE(waitForFileChangeCounterGEq(1));

	std::filesystem::rename(movedFolderPath, testFolderPath_);
	ASSERT_TRUE(waitForFileChangeCounterGEq(2));
}

TEST_F(BasicFileChangeMonitorTest, directory_rename_appearance) {
	auto initialFolderPath = test_path() / "initial-folder";
	auto testFolderPath = test_path() / "test-folder";
	std::string test_file_name = "test.txt";

	std::filesystem::create_directory(initialFolderPath);
	raco::utils::file::write(initialFolderPath / test_file_name, {});

	createdFileListeners_.emplace_back(testFileChangeMonitor_->registerFileChangedHandler((testFolderPath / test_file_name).string(), testCallback_));
	EXPECT_EQ(fileChangeCounter_, 0);

	std::filesystem::rename(initialFolderPath, testFolderPath);
	ASSERT_TRUE(waitForFileChangeCounterGEq(1));
}

TEST_F(BasicFileChangeMonitorTest, directory_appear_file_change) {
	auto initialFolderPath = test_path() / "initial-folder";
	auto testFolderPath = test_path() / "test-folder";
	std::string test_file_name = "test.txt";

	std::filesystem::create_directory(initialFolderPath);
	raco::utils::file::write(initialFolderPath / test_file_name, {});

	createdFileListeners_.emplace_back(testFileChangeMonitor_->registerFileChangedHandler((testFolderPath / test_file_name).string(), testCallback_));
	EXPECT_EQ(fileChangeCounter_, 0);

	std::filesystem::rename(initialFolderPath, testFolderPath);
	ASSERT_TRUE(waitForFileChangeCounterGEq(1));

	raco::utils::file::write(testFolderPath / test_file_name, "Test");
	ASSERT_TRUE(waitForFileChangeCounterGEq(2));
}

TEST_F(BasicFileChangeMonitorTest, directory_appear_file_disappear) {
	auto initialFolderPath = test_path() / "initial-folder";
	auto testFolderPath = test_path() / "test-folder";
	std::string test_file_name = "test.txt";

	std::filesystem::create_directory(initialFolderPath);
	raco::utils::file::write(initialFolderPath / test_file_name, {});

	createdFileListeners_.emplace_back(testFileChangeMonitor_->registerFileChangedHandler((testFolderPath / test_file_name).string(), testCallback_));
	EXPECT_EQ(fileChangeCounter_, 0);

	std::filesystem::rename(initialFolderPath, testFolderPath);
	ASSERT_TRUE(waitForFileChangeCounterGEq(1));

	std::filesystem::rename(testFolderPath / test_file_name, testFolderPath / "new-file-name.txt");
	ASSERT_TRUE(waitForFileChangeCounterGEq(2));
}

TEST_F(BasicFileChangeMonitorTest, directory_appear_file_appear) {
	auto initialFolderPath = test_path() / "initial-folder";
	auto testFolderPath = test_path() / "test-folder";
	std::string initial_test_file_name = "initial.txt";
	std::string test_file_name = "test.txt";

	std::filesystem::create_directory(initialFolderPath);
	raco::utils::file::write(initialFolderPath / initial_test_file_name, {});

	createdFileListeners_.emplace_back(testFileChangeMonitor_->registerFileChangedHandler((testFolderPath / test_file_name).string(), testCallback_));
	EXPECT_EQ(fileChangeCounter_, 0);

	std::filesystem::rename(initialFolderPath, testFolderPath);
	ASSERT_TRUE(waitForFileChangeCounterGEq(0));

	std::filesystem::rename(testFolderPath / initial_test_file_name, testFolderPath / test_file_name);
	ASSERT_TRUE(waitForFileChangeCounterGEq(1));
}
