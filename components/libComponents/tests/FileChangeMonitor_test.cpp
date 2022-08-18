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

#include <fstream>

using namespace raco::core;

class FileChangeMonitorTest : public TestEnvironmentCore {
protected:
	static constexpr const char* TEST_RESOURCES_FOLDER_NAME = "testresources";
	static constexpr const char* TEST_FILE_NAME = "test.txt";

	void SetUp() override {
		TestEnvironmentCore::SetUp();
		std::filesystem::create_directory(testFolderPath_);

		testFileOutputStream_ = std::ofstream(testFilePath_.string(), std::ios_base::out);
		testFileOutputStream_.close();

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

	int waitForFileChangeCounterGEq(int count, int timeOutInMS = 2000) {
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
		
		return fileChangeCounter_;
	}

	void runRenamingRoutine(raco::utils::u8path& originPath, const char* firstRename, const char* secondRename) {
		auto newFilePath = originPath;
		newFilePath.replace_filename(firstRename);

		auto evenNewerFilePath = testFilePath_;
		evenNewerFilePath.replace_filename(secondRename);

		std::filesystem::rename(testFilePath_, newFilePath);
		ASSERT_EQ(waitForFileChangeCounterGEq(1), 1);

		std::filesystem::rename(newFilePath, evenNewerFilePath);
		ASSERT_EQ(waitForFileChangeCounterGEq(1), 1);

		std::filesystem::rename(evenNewerFilePath, testFilePath_);
		ASSERT_EQ(waitForFileChangeCounterGEq(2), 2);

		std::filesystem::rename(testFilePath_, newFilePath);
		ASSERT_EQ(waitForFileChangeCounterGEq(3), 3);
	}

	std::ofstream testFileOutputStream_;
	int fileChangeCounter_{0};
	raco::utils::u8path testFolderPath_{test_path().append(TEST_RESOURCES_FOLDER_NAME)};
	raco::utils::u8path testFilePath_{raco::utils::u8path(testFolderPath_).append(TEST_FILE_NAME)};
	FileChangeCallback testCallback_ = {&context, nullptr, [this]() { ++fileChangeCounter_; }};
	std::unique_ptr<FileChangeMonitorImpl> testFileChangeMonitor_ = std::make_unique<FileChangeMonitorImpl>();
	std::vector<FileChangeMonitor::UniqueListener> createdFileListeners_;
	int argc = 0;
	QCoreApplication eventLoop_{argc, nullptr};
};


TEST_F(FileChangeMonitorTest, InstantiationNoFileChange) {
	ASSERT_EQ(fileChangeCounter_, 0);
}


// TODO Make this not crash anymore?
/*
TEST_F(FileChangeMonitorTest, DeInstantiationNoCrash) {
	auto secondFileChangeMonitor = std::make_unique<FileChangeMonitorImpl>(context);
	auto secondFileListener_ = secondFileChangeMonitor->registerFileChangedHandler(testFilePath_.string(), testCallback_);
	secondFileChangeMonitor.reset();
}
*/

TEST_F(FileChangeMonitorTest, FileModificationCreation) {
	testFilePath_ = raco::utils::u8path(testFolderPath_).append("differentFile.txt");
	createdFileListeners_.emplace_back(testFileChangeMonitor_->registerFileChangedHandler(testFilePath_.string(), testCallback_));
	ASSERT_EQ(waitForFileChangeCounterGEq(0), 0);

	testFileOutputStream_ = std::ofstream(testFilePath_.string(), std::ios_base::out);
	testFileOutputStream_.close();

	ASSERT_EQ(waitForFileChangeCounterGEq(1), 1);
}


TEST_F(FileChangeMonitorTest, FileModificationEditing) {
	testFileOutputStream_.open(testFilePath_.string(), std::ios_base::out);

	testFileOutputStream_ << "Test";
	testFileOutputStream_.flush();
	testFileOutputStream_.close();

	ASSERT_EQ(waitForFileChangeCounterGEq(1), 1);
}


TEST_F(FileChangeMonitorTest, FileModificationDeletion) {
	std::filesystem::remove(testFilePath_);
	ASSERT_EQ(waitForFileChangeCounterGEq(1), 1);
}


TEST_F(FileChangeMonitorTest, FileModificationRenaming) {
	runRenamingRoutine(testFilePath_, "new.txt", "evenNewer.txt");	
}


TEST_F(FileChangeMonitorTest, FileModificationMultipleModificationsAtTheSameTime) {
	testFileOutputStream_.open(testFilePath_.string(), std::ios_base::out);
	testFileOutputStream_ << "Test";

	std::ofstream otherOutputStream = std::ofstream(testFilePath_.string(), std::ios_base::app);
	otherOutputStream << "Other";
	otherOutputStream.close();

	testFileOutputStream_.close();

	ASSERT_GE(waitForFileChangeCounterGEq(1), 1); // Linux gives sometimes one, sometimes two events.
}

#if (!defined(__linux))
// For some reason in Linux we can still change the file even after we set the permissions to "owner_read" only.
// So skip this test in Linux for now.
TEST_F(FileChangeMonitorTest, FileModificationSetAndTryToEditReadOnly) {
	std::error_code ec;
	std::filesystem::permissions(testFilePath_, std::filesystem::perms::owner_read, ec);
	ASSERT_TRUE(!ec) << "Failed to set permissons. Error code: " << ec.value() << " Error message: '" << ec.message() << "'";

	// fileChangeCounter_ will be 0 in WSL, but the proper value in Linux container
	ASSERT_EQ(waitForFileChangeCounterGEq(1), 1);

	testFileOutputStream_.open(testFilePath_.string(), std::ios_base::out);
	ASSERT_FALSE(testFileOutputStream_.is_open());

	ASSERT_EQ(waitForFileChangeCounterGEq(1), 1);
}
#endif


TEST_F(FileChangeMonitorTest, FolderModificationDeletion) {
	std::filesystem::remove_all(test_path().append(TEST_RESOURCES_FOLDER_NAME));
	ASSERT_EQ(waitForFileChangeCounterGEq(1), 1); 
}


TEST_F(FileChangeMonitorTest, FolderModificationRenaming) {
	runRenamingRoutine(testFolderPath_, "newFolder", "evenNewerFolder");
}
