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
#include "property_browser/editors/DoubleEditor.h"

#include "property_browser/PropertyBrowserItem.h"
#include "application/RaCoProject.h"
#include "property_browser/editors/StringEditor.h"
#include "user_types/PerspectiveCamera.h"

#include <gtest/gtest.h>

using namespace raco::core;
using namespace raco::user_types;

namespace raco::property_browser {


class ExposedDoubleEditor : public DoubleEditor {
public:
	ExposedDoubleEditor(PropertyBrowserItem *item) : DoubleEditor(item) {}

	void editSpinBoxValue(const double value) const {
		spinBox_->Q_EMIT valueEdited(value);
	}

	void editSliderValue(const double value) const {
		slider_->Q_EMIT valueEdited(value);
	}

	bool spinBoxHasMultipleValues() const {
		return spinBox_->hasMultipleValues();
	}

	bool sliderHasMultipleValues() const {
		return slider_->hasMultipleValues();
	}

	static void pasteProperty(PropertyBrowserItem* item, data_storage::ValueBase* value) {
		DoubleEditor::pasteProperty(item, value);
	}
};

class DoubleEditorTest : public EditorTestFixture {
public:
	SNode node_1 = this->create<Node>("node_1");
	ValueHandle doubleHandleNode_1{node_1, {"translation", "x"}};

	SNode node_2 = this->create<Node>("node_2");
	ValueHandle doubleHandleNode_2{node_2, {"translation", "x"}};

	SPerspectiveCamera persp_camera = this->create<PerspectiveCamera>("persp_camera");
	ValueHandle doubleHandleCamera{persp_camera, {"translation", "x"}};
	
	PropertyBrowserItem propertyBrowserItem{{doubleHandleNode_1, doubleHandleNode_2, doubleHandleCamera}, this->dataChangeDispatcher, &this->commandInterface, &this->model};
	ExposedDoubleEditor doubleEditor{&propertyBrowserItem};

	void setSpinBoxValue(const double value) {
		doubleEditor.editSpinBoxValue(value);
		this->dispatch();
	}

	void setSliderValue(const double value) {
		doubleEditor.editSliderValue(value);
		this->dispatch();
	}

	bool multipleValuesTextDisplayed() const {
		return doubleEditor.spinBoxHasMultipleValues() && doubleEditor.sliderHasMultipleValues();
	}
};

TEST_F(DoubleEditorTest, set_value_to_2_objects_of_same_type) {
	commandInterface.set(doubleHandleNode_1, 10.0);
	commandInterface.set(doubleHandleNode_2, 20.0);
	dispatch();

	setSpinBoxValue(5.0);

	EXPECT_DOUBLE_EQ(*node_1->translation_->x, 5.0);
	EXPECT_DOUBLE_EQ(*node_2->translation_->x, 5.0);

	setSliderValue(50.0);

	EXPECT_DOUBLE_EQ(*node_1->translation_->x, 50.0);
	EXPECT_DOUBLE_EQ(*node_2->translation_->x, 50.0);
}

TEST_F(DoubleEditorTest, set_value_to_2_objects_of_different_types) {
	commandInterface.set(doubleHandleNode_2, 10.0);
	commandInterface.set(doubleHandleCamera, 20.0);
	dispatch();

	setSpinBoxValue(5.0);

	EXPECT_DOUBLE_EQ(*node_2->translation_->x, 5.0);
	EXPECT_DOUBLE_EQ(*persp_camera->translation_->x, 5.0);

	setSliderValue(50.0);

	EXPECT_DOUBLE_EQ(*node_2->translation_->x, 50.0);
	EXPECT_DOUBLE_EQ(*persp_camera->translation_->x, 50.0);
}

TEST_F(DoubleEditorTest, check_multiple_values_text_displayed) {
	commandInterface.set(doubleHandleNode_1, 5.0);
	commandInterface.set(doubleHandleNode_2, 10.0);
	commandInterface.set(doubleHandleCamera, 5.0);
	dispatch();

	EXPECT_EQ(multipleValuesTextDisplayed(), true);
	
	commandInterface.set(doubleHandleNode_2, 5.0);
	dispatch();

	EXPECT_EQ(multipleValuesTextDisplayed(), false);

	commandInterface.set(doubleHandleCamera, 10.0);
	dispatch();

	EXPECT_EQ(multipleValuesTextDisplayed(), true);
}


TEST_F(DoubleEditorTest, paste_property_from_double) {
	Value<double> value{13.2};

	doubleEditor.pasteProperty(&propertyBrowserItem, &value);
	EXPECT_DOUBLE_EQ(*node_1->translation_->x, 13.2);
	EXPECT_DOUBLE_EQ(*node_2->translation_->x, 13.2);

	commandInterface.set(doubleHandleNode_2, 5.0);
	dispatch();

	doubleEditor.pasteProperty(&propertyBrowserItem, &value);
	EXPECT_DOUBLE_EQ(*node_1->translation_->x, 13.2);
	EXPECT_DOUBLE_EQ(*node_2->translation_->x, 13.2);
}

TEST_F(DoubleEditorTest, paste_property_from_int) {
	Value<int> value{13};

	doubleEditor.pasteProperty(&propertyBrowserItem, &value);
	EXPECT_DOUBLE_EQ(*node_1->translation_->x, 13.0);
	EXPECT_DOUBLE_EQ(*node_2->translation_->x, 13.0);

	commandInterface.set(doubleHandleNode_2, 5.0);
	dispatch();

	doubleEditor.pasteProperty(&propertyBrowserItem, &value);
	EXPECT_DOUBLE_EQ(*node_1->translation_->x, 13.0);
	EXPECT_DOUBLE_EQ(*node_2->translation_->x, 13.0);
}

TEST_F(DoubleEditorTest, paste_property_from_string) {
	Value<std::string> value{"13.2"};

	doubleEditor.pasteProperty(&propertyBrowserItem, &value);
	EXPECT_DOUBLE_EQ(*node_1->translation_->x, 13.2);
	EXPECT_DOUBLE_EQ(*node_2->translation_->x, 13.2);

	commandInterface.set(doubleHandleNode_2, 5.0);
	dispatch();

	doubleEditor.pasteProperty(&propertyBrowserItem, &value);
	EXPECT_DOUBLE_EQ(*node_1->translation_->x, 13.2);
	EXPECT_DOUBLE_EQ(*node_2->translation_->x, 13.2);
}

TEST_F(DoubleEditorTest, paste_property_from_bool) {
	Value<bool> value{true};

	doubleEditor.pasteProperty(&propertyBrowserItem, &value);
	EXPECT_DOUBLE_EQ(*node_1->translation_->x, 0.0);
	EXPECT_DOUBLE_EQ(*node_2->translation_->x, 0.0);
}



}  // namespace raco::property_browser
