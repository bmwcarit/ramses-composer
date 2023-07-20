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
	StringEditorLineEdit *getLineEdit() { return lineEdit_; }
	void setFromFileDialog(const QString& file) {
		URIEditor::setFromFileDialog(file);
	}
	void switchToAbsolutePath() {
		URIEditor::switchToAbsolutePath();
	}
	void switchToRelativePath() {
		URIEditor::switchToRelativePath();
	}
	bool switchToAbsolutePathEnabled() {
		return URIEditor::switchToAbsolutePathEnabled();
	}
	bool switchToRelativePathEnabled() {
		return URIEditor::switchToRelativePathEnabled();
	}
};

class URIEditorTest : public EditorTestFixture {
public:
	SMesh mesh_1 = this->create<Mesh>("mesh_1");
	ValueHandle uriHandle_1{mesh_1, {"uri"}};

	SMesh mesh_2 = this->create<Mesh>("mesh_2");
	ValueHandle uriHandle_2{mesh_2, {"uri"}};
	
	PropertyBrowserItem propertyBrowserItem{{uriHandle_1, uriHandle_2}, this->dataChangeDispatcher, &this->commandInterface, &this->model};
	ExposedURIEditor uriEditor{&propertyBrowserItem};
	std::string projectPath{(this->test_path() / "project" / "projectFileName").string()};

	std::string absoluteMeshPath_1{(this->test_path() / "meshes" / "Duck.glb").string()};
	std::string relativeMeshPath_1{raco::utils::u8path(absoluteMeshPath_1).normalizedRelativePath(raco::utils::u8path(projectPath).parent_path()).string()};

	std::string absoluteMeshPath_2{(this->test_path() / "meshes" / "defaultQuad.gltf").string()};
	std::string relativeMeshPath_2{raco::utils::u8path(absoluteMeshPath_2).normalizedRelativePath(raco::utils::u8path(projectPath).parent_path()).string()};

	std::string absoluteMeshPath_3{(this->test_path() / "meshes" / "ToyCar/ToyCar.gltf").string()};
	std::string relativeMeshPath_3{raco::utils::u8path(absoluteMeshPath_3).normalizedRelativePath(raco::utils::u8path(projectPath).parent_path()).string()};

	URIEditorTest() {
		this->project.setCurrentPath(projectPath);
		this->dispatch();
	}

	void setLineEditText(const std::string &newText) {
		uriEditor.getLineEdit()->set(newText);
		uriEditor.getLineEdit()->setModified(true);
		uriEditor.getLineEdit()->Q_EMIT editingFinished();
		this->dispatch();
	}

	void setFromFileDialog(const std::string &path) {
		uriEditor.setFromFileDialog(QString::fromStdString(path));
		this->dispatch();
	}

	void switchToAbsolute() {
		uriEditor.switchToAbsolutePath();
		this->dispatch();
	}
	
	void switchToRelative() {
		uriEditor.switchToRelativePath();
		this->dispatch();
	}
};

TEST_F(URIEditorTest, InstantiationShowWarning) {
	ASSERT_TRUE(propertyBrowserItem.hasError());
	ASSERT_EQ(propertyBrowserItem.maxErrorLevel(), ErrorLevel::WARNING);
}

TEST_F(URIEditorTest, ModificationAddNonExistentPath) {
	propertyBrowserItem.set((test_path() / "THISSHOULDNOTEXIST.txt").string());
	
	ASSERT_TRUE(propertyBrowserItem.hasError());
	ASSERT_EQ(propertyBrowserItem.maxErrorLevel(), ErrorLevel::ERROR);
}

TEST_F(URIEditorTest, ModificationAddExistentMeshPath) {
	propertyBrowserItem.set(absoluteMeshPath_1);
	
	ASSERT_FALSE(propertyBrowserItem.hasError());
}

TEST_F(URIEditorTest, ModificationSetEmptyPathAfterExistantPath) {
	propertyBrowserItem.set(absoluteMeshPath_1);
	propertyBrowserItem.set(std::string());
	
	ASSERT_TRUE(propertyBrowserItem.hasError());
	ASSERT_EQ(propertyBrowserItem.maxErrorLevel(), ErrorLevel::WARNING);
}


TEST_F(URIEditorTest, set_text_abs_start_abs) {
	commandInterface.set(uriHandle_1, absoluteMeshPath_2);
	commandInterface.set(uriHandle_2, absoluteMeshPath_3);
	dispatch();

	setLineEditText(absoluteMeshPath_1);

	EXPECT_EQ(*mesh_1->uri_, absoluteMeshPath_1);
	EXPECT_EQ(*mesh_2->uri_, absoluteMeshPath_1);
}

TEST_F(URIEditorTest, set_text_abs_start_rel) {
	commandInterface.set(uriHandle_1, relativeMeshPath_2);
	commandInterface.set(uriHandle_2, relativeMeshPath_3);
	dispatch();

	setLineEditText(absoluteMeshPath_1);

	EXPECT_EQ(*mesh_1->uri_, absoluteMeshPath_1);
	EXPECT_EQ(*mesh_2->uri_, absoluteMeshPath_1);
}

TEST_F(URIEditorTest, set_text_abs_start_mixed) {
	commandInterface.set(uriHandle_1, absoluteMeshPath_2);
	commandInterface.set(uriHandle_2, relativeMeshPath_3);
	dispatch();

	setLineEditText(absoluteMeshPath_1);

	EXPECT_EQ(*mesh_1->uri_, absoluteMeshPath_1);
	EXPECT_EQ(*mesh_2->uri_, absoluteMeshPath_1);
}


TEST_F(URIEditorTest, set_text_rel_start_abs) {
	commandInterface.set(uriHandle_1, absoluteMeshPath_2);
	commandInterface.set(uriHandle_2, absoluteMeshPath_3);
	dispatch();

	setLineEditText(relativeMeshPath_1);

	EXPECT_EQ(*mesh_1->uri_, relativeMeshPath_1);
	EXPECT_EQ(*mesh_2->uri_, relativeMeshPath_1);
}

TEST_F(URIEditorTest, set_text_rel_start_rel) {
	commandInterface.set(uriHandle_1, relativeMeshPath_2);
	commandInterface.set(uriHandle_2, relativeMeshPath_3);
	dispatch();

	setLineEditText(relativeMeshPath_1);

	EXPECT_EQ(*mesh_1->uri_, relativeMeshPath_1);
	EXPECT_EQ(*mesh_2->uri_, relativeMeshPath_1);
}

TEST_F(URIEditorTest, set_text_rel_start_mixed) {
	commandInterface.set(uriHandle_1, absoluteMeshPath_2);
	commandInterface.set(uriHandle_2, relativeMeshPath_3);
	dispatch();

	setLineEditText(relativeMeshPath_1);

	EXPECT_EQ(*mesh_1->uri_, relativeMeshPath_1);
	EXPECT_EQ(*mesh_2->uri_, relativeMeshPath_1);
}

TEST_F(URIEditorTest, set_dialog_abs_start_abs) {
	commandInterface.set(uriHandle_1, absoluteMeshPath_2);
	commandInterface.set(uriHandle_2, absoluteMeshPath_3);
	dispatch();

	setFromFileDialog(absoluteMeshPath_1);

	EXPECT_EQ(*mesh_1->uri_, absoluteMeshPath_1);
	EXPECT_EQ(*mesh_2->uri_, absoluteMeshPath_1);
}

TEST_F(URIEditorTest, set_dialog_abs_start_rel) {
	commandInterface.set(uriHandle_1, relativeMeshPath_2);
	commandInterface.set(uriHandle_2, relativeMeshPath_3);
	dispatch();

	setFromFileDialog(absoluteMeshPath_1);

	EXPECT_EQ(*mesh_1->uri_, relativeMeshPath_1);
	EXPECT_EQ(*mesh_2->uri_, relativeMeshPath_1);
}

TEST_F(URIEditorTest, set_dialog_abs_start_mixed) {
	commandInterface.set(uriHandle_1, absoluteMeshPath_2);
	commandInterface.set(uriHandle_2, relativeMeshPath_3);
	dispatch();

	setFromFileDialog(absoluteMeshPath_1);

	EXPECT_EQ(*mesh_1->uri_, absoluteMeshPath_1);
	EXPECT_EQ(*mesh_2->uri_, relativeMeshPath_1);
}

TEST_F(URIEditorTest, switch_to_abs_start_abs) {
	commandInterface.set(uriHandle_1, absoluteMeshPath_2);
	commandInterface.set(uriHandle_2, absoluteMeshPath_3);
	dispatch();

	EXPECT_EQ(uriEditor.switchToAbsolutePathEnabled(), false);
	EXPECT_EQ(uriEditor.switchToRelativePathEnabled(), true);

	switchToAbsolute();

	EXPECT_EQ(*mesh_1->uri_, absoluteMeshPath_2);
	EXPECT_EQ(*mesh_2->uri_, absoluteMeshPath_3);

	EXPECT_EQ(uriEditor.switchToAbsolutePathEnabled(), false);
	EXPECT_EQ(uriEditor.switchToRelativePathEnabled(), true);
}

TEST_F(URIEditorTest, switch_to_abs_start_rel) {
	commandInterface.set(uriHandle_1, relativeMeshPath_2);
	commandInterface.set(uriHandle_2, relativeMeshPath_3);
	dispatch();

	EXPECT_EQ(uriEditor.switchToAbsolutePathEnabled(), true);
	EXPECT_EQ(uriEditor.switchToRelativePathEnabled(), false);

	switchToAbsolute();

	EXPECT_EQ(*mesh_1->uri_, absoluteMeshPath_2);
	EXPECT_EQ(*mesh_2->uri_, absoluteMeshPath_3);

	EXPECT_EQ(uriEditor.switchToAbsolutePathEnabled(), false);
	EXPECT_EQ(uriEditor.switchToRelativePathEnabled(), true);
}

TEST_F(URIEditorTest, switch_to_abs_start_mixed) {
	commandInterface.set(uriHandle_1, absoluteMeshPath_2);
	commandInterface.set(uriHandle_2, relativeMeshPath_3);
	dispatch();

	EXPECT_EQ(uriEditor.switchToAbsolutePathEnabled(), true);
	EXPECT_EQ(uriEditor.switchToRelativePathEnabled(), true);

	switchToAbsolute();

	EXPECT_EQ(*mesh_1->uri_, absoluteMeshPath_2);
	EXPECT_EQ(*mesh_2->uri_, absoluteMeshPath_3);

	EXPECT_EQ(uriEditor.switchToAbsolutePathEnabled(), false);
	EXPECT_EQ(uriEditor.switchToRelativePathEnabled(), true);
}

TEST_F(URIEditorTest, switch_to_rel_start_abs) {
	commandInterface.set(uriHandle_1, absoluteMeshPath_2);
	commandInterface.set(uriHandle_2, absoluteMeshPath_3);
	dispatch();

	EXPECT_EQ(uriEditor.switchToAbsolutePathEnabled(), false);
	EXPECT_EQ(uriEditor.switchToRelativePathEnabled(), true);

	switchToRelative();

	EXPECT_EQ(*mesh_1->uri_, relativeMeshPath_2);
	EXPECT_EQ(*mesh_2->uri_, relativeMeshPath_3);

	EXPECT_EQ(uriEditor.switchToAbsolutePathEnabled(), true);
	EXPECT_EQ(uriEditor.switchToRelativePathEnabled(), false);
}

TEST_F(URIEditorTest, switch_to_rel_start_rel) {
	commandInterface.set(uriHandle_1, relativeMeshPath_2);
	commandInterface.set(uriHandle_2, relativeMeshPath_3);
	dispatch();

	EXPECT_EQ(uriEditor.switchToAbsolutePathEnabled(), true);
	EXPECT_EQ(uriEditor.switchToRelativePathEnabled(), false);

	switchToRelative();

	EXPECT_EQ(*mesh_1->uri_, relativeMeshPath_2);
	EXPECT_EQ(*mesh_2->uri_, relativeMeshPath_3);

	EXPECT_EQ(uriEditor.switchToAbsolutePathEnabled(), true);
	EXPECT_EQ(uriEditor.switchToRelativePathEnabled(), false);
}

TEST_F(URIEditorTest, switch_to_rel_start_mixed) {
	commandInterface.set(uriHandle_1, absoluteMeshPath_2);
	commandInterface.set(uriHandle_2, relativeMeshPath_3);
	dispatch();

	EXPECT_EQ(uriEditor.switchToAbsolutePathEnabled(), true);
	EXPECT_EQ(uriEditor.switchToRelativePathEnabled(), true);

	switchToRelative();

	EXPECT_EQ(*mesh_1->uri_, relativeMeshPath_2);
	EXPECT_EQ(*mesh_2->uri_, relativeMeshPath_3);

	EXPECT_EQ(uriEditor.switchToAbsolutePathEnabled(), true);
	EXPECT_EQ(uriEditor.switchToRelativePathEnabled(), false);
}

TEST_F(URIEditorTest, ModificationRerootRelativePath) {
	auto newProjectPath = test_path() / "project" / "projectSubFolder" / "projectFileName";
	auto newRelativeMeshPath = raco::utils::u8path(absoluteMeshPath_1).normalizedRelativePath(newProjectPath.parent_path());

	setLineEditText(relativeMeshPath_1);

	propertyBrowserItem.set(raco::utils::u8path(relativeMeshPath_1).rerootRelativePath(projectPath, newProjectPath).string());
	dispatch();

	ASSERT_EQ(uriEditor.getLineEdit()->text().toStdString(), newRelativeMeshPath);
}

}  // namespace raco::property_browser
