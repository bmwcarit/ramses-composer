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
#include "property_browser/editors/BoolEditor.h"

#include "property_browser/PropertyBrowserItem.h"
#include "application/RaCoProject.h"
#include "property_browser/editors/StringEditor.h"
#include "user_types/PerspectiveCamera.h"

#include <QCheckBox>
#include <gtest/gtest.h>

using namespace raco::core;
using namespace raco::user_types;

namespace raco::property_browser {


class ExposedBoolEditor : public BoolEditor {
public:
	ExposedBoolEditor(PropertyBrowserItem *item) : BoolEditor(item) {}

	void clickOnCheckBox(const bool checked) const {
		checkBox_->Q_EMIT clicked(checked);
	}

	bool isTristate() const {
		return checkBox_->checkState() == Qt::CheckState::PartiallyChecked && checkBox_->isTristate();
	}

	static void pasteProperty(PropertyBrowserItem* item, data_storage::ValueBase* value) {
		BoolEditor::pasteProperty(item, value);
	}
};

class BoolEditorTest : public EditorTestFixture {
public:
	SNode node_1 = this->create<Node>("node_1");
	ValueHandle boolHandleNode_1{node_1, {"visibility"}};

	SNode node_2 = this->create<Node>("node_2");
	ValueHandle boolHandleNode_2{node_2, {"visibility"}};

	SPerspectiveCamera persp_camera = this->create<PerspectiveCamera>("persp_camera");
	ValueHandle boolHandleCamera{persp_camera, {"visibility"}};
	
	PropertyBrowserItem propertyBrowserItem{{boolHandleNode_1, boolHandleNode_2, boolHandleCamera}, this->dataChangeDispatcher, &this->commandInterface, &this->model};
	ExposedBoolEditor boolEditor{&propertyBrowserItem};

	void setCheckBoxValue(const bool checked) {
		boolEditor.clickOnCheckBox(checked);
		this->dispatch();
	}

	bool isTristate() {
		return boolEditor.isTristate();
	}
};

TEST_F(BoolEditorTest, set_value_to_2_objects_of_same_type) {
	commandInterface.set(boolHandleNode_1, true);
	commandInterface.set(boolHandleNode_2, false);
	dispatch();

	setCheckBoxValue(false);

	EXPECT_EQ(*node_1->visibility_, false);
	EXPECT_EQ(*node_2->visibility_, false);

	setCheckBoxValue(true);

	EXPECT_EQ(*node_1->visibility_, true);
	EXPECT_EQ(*node_2->visibility_, true);
}

TEST_F(BoolEditorTest, set_value_to_2_objects_of_different_types) {
	commandInterface.set(boolHandleNode_2, true);
	commandInterface.set(boolHandleCamera, false);
	dispatch();

	setCheckBoxValue(true);

	EXPECT_EQ(*node_2->visibility_, true);
	EXPECT_EQ(*persp_camera->visibility_, true);

	setCheckBoxValue(false);

	EXPECT_EQ(*node_2->visibility_, false);
	EXPECT_EQ(*persp_camera->visibility_, false);
}

TEST_F(BoolEditorTest, check_tristate_for_multiple_objects) {
	commandInterface.set(boolHandleNode_1, true);
	commandInterface.set(boolHandleNode_2, false);
	commandInterface.set(boolHandleCamera, true);
	dispatch();

	EXPECT_EQ(isTristate(), true);
	
	commandInterface.set(boolHandleNode_2, true);
	dispatch();

	EXPECT_EQ(isTristate(), false);

	commandInterface.set(boolHandleCamera, false);
	dispatch();

	EXPECT_EQ(isTristate(), true);
}

TEST_F(BoolEditorTest, paste_property_from_bool) {
	commandInterface.set(boolHandleNode_1, false);
	commandInterface.set(boolHandleNode_2, false);
	dispatch();

	Value<bool> value{true};
	boolEditor.pasteProperty(&propertyBrowserItem, &value);
	EXPECT_EQ(*node_1->visibility_, true);
	EXPECT_EQ(*node_2->visibility_, true);

	commandInterface.set(boolHandleNode_2, false);
	dispatch();

	boolEditor.pasteProperty(&propertyBrowserItem, &value);
	EXPECT_EQ(*node_1->visibility_, true);
	EXPECT_EQ(*node_2->visibility_, true);
}

TEST_F(BoolEditorTest, paste_property_from_string) {
	commandInterface.set(boolHandleNode_1, false);
	commandInterface.set(boolHandleNode_2, false);
	dispatch();

	Value<std::string> value{"true"};
	boolEditor.pasteProperty(&propertyBrowserItem, &value);
	EXPECT_EQ(*node_1->visibility_, true);
	EXPECT_EQ(*node_2->visibility_, true);

	commandInterface.set(boolHandleNode_2, false);
	dispatch();

	boolEditor.pasteProperty(&propertyBrowserItem, &value);
	EXPECT_EQ(*node_1->visibility_, true);
	EXPECT_EQ(*node_2->visibility_, true);
}

}  // namespace raco::property_browser
