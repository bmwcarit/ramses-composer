/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "EditorTestFixture.h"
#include "property_browser/editors/URIEditor.h"

#include "core/EditorObject.h"
#include "core/PathManager.h"
#include "property_browser/PropertyBrowserItem.h"
#include "application/RaCoProject.h"
#include "testing/RacoBaseTest.h"
#include "user_types/Mesh.h"

#include <QApplication>
#include <gtest/gtest.h>

using namespace raco::core;
using namespace raco::user_types;

namespace raco::property_browser {


class ExposedURIEditor : public URIEditor {
public:
	ExposedURIEditor(PropertyBrowserItem *item) : URIEditor(item) {}
	QLineEdit *getLineEdit() { return lineEdit_; }
};

class URIEditorTest : public EditorTestFixture {
public:
	SEditorObject mesh = commandInterface.createObject("Mesh");
	ValueHandle propertyHandle{mesh, {"uri"}};
	PropertyBrowserItem propertyBrowserItem{propertyHandle, dataChangeDispatcher, &commandInterface, &model};
	ExposedURIEditor uriEditor{&propertyBrowserItem};
	std::string projectPath{(test_path() / "project" / "projectFileName").string()};
	std::string absoluteMeshPath{(test_path() / "meshes" / "Duck.glb").string()};
	std::string relativeMeshPath{raco::utils::u8path(absoluteMeshPath).normalizedRelativePath(raco::utils::u8path(projectPath).parent_path()).string()};

	URIEditorTest() {
		project.setCurrentPath(projectPath);
		dispatch();
	}

	void assertCorrectAbsolutePath() {
		ASSERT_EQ(uriEditor.getLineEdit()->text().toStdString(), absoluteMeshPath);
		ASSERT_TRUE(uriEditor.pathIsAbsolute());
		ASSERT_FALSE(propertyBrowserItem.hasError());
	}

	void assertCorrectRelativePath() {
		ASSERT_EQ(uriEditor.getLineEdit()->text().toStdString(), relativeMeshPath);
		ASSERT_FALSE(uriEditor.pathIsAbsolute());
		ASSERT_FALSE(propertyBrowserItem.hasError());
	}

	void setLineEditText(const std::string &newText) {
		uriEditor.getLineEdit()->setText(QString::fromStdString(newText));
		uriEditor.getLineEdit()->Q_EMIT editingFinished();
		dispatch();
	}
};

TEST_F(URIEditorTest, InstantiationShowWarning) {
	ASSERT_TRUE(propertyBrowserItem.hasError());
	ASSERT_EQ(propertyBrowserItem.error().level(), ErrorLevel::WARNING);
}

TEST_F(URIEditorTest, InstantiationIsDefaultRelative) {
	ASSERT_FALSE(uriEditor.pathIsAbsolute());
}

TEST_F(URIEditorTest, ModificationAddNonExistentPath) {
	propertyBrowserItem.set((test_path() / "THISSHOULDNOTEXIST.txt").string());
	
	ASSERT_TRUE(propertyBrowserItem.hasError());
	ASSERT_EQ(propertyBrowserItem.error().level(), ErrorLevel::ERROR);
}

TEST_F(URIEditorTest, ModificationAddExistentMeshPath) {
	propertyBrowserItem.set(absoluteMeshPath);
	
	ASSERT_FALSE(propertyBrowserItem.hasError());
}

TEST_F(URIEditorTest, ModificationSetEmptyPathAfterExistantPath) {
	propertyBrowserItem.set(absoluteMeshPath);
	propertyBrowserItem.set(std::string());
	
	ASSERT_TRUE(propertyBrowserItem.hasError());
	ASSERT_EQ(propertyBrowserItem.error().level(), ErrorLevel::WARNING);
	ASSERT_FALSE(uriEditor.pathIsAbsolute());
}

TEST_F(URIEditorTest, ModificationSetAbsolutePath) {
	setLineEditText(absoluteMeshPath);

	assertCorrectAbsolutePath();
}

TEST_F(URIEditorTest, ModificationSetRelativePath) {
	setLineEditText(relativeMeshPath);

	assertCorrectRelativePath();
}

TEST_F(URIEditorTest, ModificationChangeAbsoluteToRelativePathByText) {
	setLineEditText(absoluteMeshPath);
	setLineEditText(relativeMeshPath);

	assertCorrectRelativePath();
}

TEST_F(URIEditorTest, ModificationChangeRelativeToAbsolutePathByText) {
	setLineEditText(relativeMeshPath);
	setLineEditText(absoluteMeshPath);

	assertCorrectAbsolutePath();
}

TEST_F(URIEditorTest, ModificationChangeAbsoluteToRelativePathByUserAction) {
	setLineEditText(absoluteMeshPath);

	uriEditor.switchAbsoluteRelativePath();
	dispatch();

	assertCorrectRelativePath();
}

TEST_F(URIEditorTest, ModificationChangeRelativeToAbsolutePathByUserAction) {
	setLineEditText(relativeMeshPath);

	uriEditor.switchAbsoluteRelativePath();
	dispatch();

	assertCorrectAbsolutePath();
}

TEST_F(URIEditorTest, ModificationRerootRelativePath) {
	auto newProjectPath = test_path() / "project" / "projectSubFolder" / "projectFileName";
	auto newRelativeMeshPath = raco::utils::u8path(absoluteMeshPath).normalizedRelativePath(newProjectPath.parent_path());

	setLineEditText(relativeMeshPath);

	propertyBrowserItem.set(raco::utils::u8path(relativeMeshPath).rerootRelativePath(projectPath, newProjectPath).string());
	dispatch();

	ASSERT_EQ(uriEditor.getLineEdit()->text().toStdString(), newRelativeMeshPath);
}

}  // namespace raco::property_browser
