/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "EditorTestFixture.h"
#include "property_browser/editors/URIEditor.h"

#include "PropertyBrowserItemTestHelper.h"
#include "core/EditorObject.h"
#include "core/PathManager.h"
#include "property_browser/PropertyBrowserItem.h"
#include "application/RaCoProject.h"
#include "testing/RacoBaseTest.h"
#include "user_types/Mesh.h"

#include <QApplication>
#include <gtest/gtest.h>
#include <testing/TestEnvironmentCore.h>

using namespace raco::core;
using namespace raco::user_types;

namespace raco::property_browser {

class MockURI : public core::EditorObject {
public:
	static inline const TypeDescriptor typeDescription = {"MockURI", true};
	TypeDescriptor const &getTypeDescription() const override {
		return typeDescription;
	}
	MockURI(MockURI const &) = delete;
	MockURI(std::string name = std::string(), std::string id = std::string()) : EditorObject(name, id) {
		fillPropertyDescription();
	}
	virtual ~MockURI() {}

	void fillPropertyDescription() {
		properties_.emplace_back("uri", &uri_);
	}

	data_storage::Property<std::string, data_storage::URIAnnotation> uri_{std::string(), {}};
};

class ExposedURIEditor : public URIEditor {
public:
	ExposedURIEditor(PropertyBrowserItem *item) : URIEditor(item) {}
	QLineEdit *getLineEdit() { return lineEdit_; }
};

class URIEditorTest : public EditorTestFixture {
public:
	PropertyBrowserItemTestHelper<Mesh> data{};
	ValueHandle propertyHandle{data.valueHandle.get("uri")};
	PropertyBrowserItem propertyBrowserItem{propertyHandle, dataChangeDispatcher, &commandInterface, &model};
	ExposedURIEditor uriEditor{&propertyBrowserItem};
	std::string projectPath{(cwd_path() / "project" / "projectFileName").generic_string()};
	std::string absoluteMeshPath{(cwd_path() / "meshes" / "Duck.glb").generic_string()};
	std::string relativeMeshPath{PathManager::constructRelativePath(absoluteMeshPath, std::filesystem::path(projectPath).parent_path().string())};

	URIEditorTest() {
		project.setCurrentPath(projectPath);
		project.addInstance(data.editorObject);
		valueChanged();
	}

	void valueChanged() {
		dataChangeDispatcher->dispatch(recorder.release());
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
		valueChanged();
	}
};

TEST_F(URIEditorTest, InstantiationShowWarning) {
	data.editorObject->onAfterValueChanged(context, propertyHandle);

	ASSERT_TRUE(propertyBrowserItem.hasError());
	ASSERT_EQ(propertyBrowserItem.error().level(), ErrorLevel::WARNING);
}

TEST_F(URIEditorTest, InstantiationIsDefaultRelative) {
	ASSERT_FALSE(uriEditor.pathIsAbsolute());
}

TEST_F(URIEditorTest, ModificationAddNonExistentPath) {
	propertyBrowserItem.set((cwd_path() / "THISSHOULDNOTEXIST.txt").string());
	
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
	valueChanged();

	assertCorrectRelativePath();
}

TEST_F(URIEditorTest, ModificationChangeRelativeToAbsolutePathByUserAction) {
	setLineEditText(relativeMeshPath);

	uriEditor.switchAbsoluteRelativePath();
	valueChanged();

	assertCorrectAbsolutePath();
}

TEST_F(URIEditorTest, ModificationRerootRelativePath) {
	std::string newProjectPath{(cwd_path() / "project" / "projectSubFolder" / "projectFileName").generic_string()};
	std::string newRelativeMeshPath{PathManager::constructRelativePath(absoluteMeshPath, std::filesystem::path(newProjectPath).parent_path().string())};

	setLineEditText(relativeMeshPath);

	propertyBrowserItem.set(PathManager::rerootRelativePath(relativeMeshPath, projectPath, newProjectPath));
	valueChanged();

	ASSERT_EQ(uriEditor.getLineEdit()->text().toStdString(), newRelativeMeshPath);
}

}  // namespace raco::property_browser
