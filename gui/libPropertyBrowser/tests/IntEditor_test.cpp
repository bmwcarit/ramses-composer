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
#include "property_browser/editors/IntEditor.h"

#include "property_browser/PropertyBrowserItem.h"
#include "application/RaCoProject.h"
#include "property_browser/editors/StringEditor.h"

#include <gtest/gtest.h>

using namespace raco::core;
using namespace raco::user_types;

namespace raco::property_browser {


class ExposedIntEditor : public IntEditor {
public:
	ExposedIntEditor(PropertyBrowserItem *item) : IntEditor(item) {}

	void editSpinBoxValue(const int value) const {
		spinBox_->Q_EMIT valueEdited(value);
	}

	void editSliderValue(const int value) const {
		slider_->Q_EMIT valueEdited(value);
	}

	bool spinBoxHasMultipleValues() const {
		return spinBox_->hasMultipleValues();
	}

	bool sliderHasMultipleValues() const {
		return slider_->hasMultipleValues();
	}

	static void pasteProperty(PropertyBrowserItem* item, data_storage::ValueBase* value) {
		IntEditor::pasteProperty(item, value);
	}
};


class IntEditorTest : public EditorTestFixture {
public:
	SMeshNode meshNode_1 = this->create<MeshNode>("meshNode_1");
	ValueHandle intHandleMeshNode_1{meshNode_1, {"instanceCount"}};

	SMeshNode meshNode_2 = this->create<MeshNode>("meshNode_2");
	ValueHandle intHandleMeshNode_2{meshNode_2, {"instanceCount"}};
	
	PropertyBrowserItem propertyBrowserItem{{intHandleMeshNode_1, intHandleMeshNode_2}, this->dataChangeDispatcher, &this->commandInterface, &this->model};
	ExposedIntEditor intEditor{&propertyBrowserItem};

	void setSpinBoxValue(const double value) {
		intEditor.editSpinBoxValue(value);
		this->dispatch();
	}

	void setSliderValue(const double value) {
		intEditor.editSliderValue(value);
		this->dispatch();
	}

	bool multipleValuesTextDisplayed() const {
		return intEditor.spinBoxHasMultipleValues() && intEditor.sliderHasMultipleValues();
	}
};

TEST_F(IntEditorTest, set_multiple_values) {
	commandInterface.set(intHandleMeshNode_1, 10);
	commandInterface.set(intHandleMeshNode_2, 20);
	dispatch();

	setSpinBoxValue(5);

	EXPECT_EQ(*meshNode_1->instanceCount_, 5);
	EXPECT_EQ(*meshNode_2->instanceCount_, 5);

	setSliderValue(50);

	EXPECT_EQ(*meshNode_1->instanceCount_, 50);
	EXPECT_EQ(*meshNode_2->instanceCount_, 50);
}

TEST_F(IntEditorTest, check_multiple_values_text_displayed) {
	commandInterface.set(intHandleMeshNode_1, 5);
	commandInterface.set(intHandleMeshNode_2, 10);
	dispatch();

	EXPECT_EQ(multipleValuesTextDisplayed(), true);
	
	commandInterface.set(intHandleMeshNode_2, 5);
	dispatch();

	EXPECT_EQ(multipleValuesTextDisplayed(), false);

	commandInterface.set(intHandleMeshNode_1, 10);
	dispatch();

	EXPECT_EQ(multipleValuesTextDisplayed(), true);
}

TEST_F(IntEditorTest, paste_property_from_int) {
	Value<int> value{13};

	intEditor.pasteProperty(&propertyBrowserItem, &value);
	EXPECT_EQ(*meshNode_1->instanceCount_, 13);
	EXPECT_EQ(*meshNode_2->instanceCount_, 13);

	commandInterface.set(intHandleMeshNode_2, 5);
	dispatch();

	intEditor.pasteProperty(&propertyBrowserItem, &value);
	EXPECT_EQ(*meshNode_1->instanceCount_, 13);
	EXPECT_EQ(*meshNode_2->instanceCount_, 13);
}

TEST_F(IntEditorTest, paste_property_from_double) {
	Value<double> value{13.2};

	intEditor.pasteProperty(&propertyBrowserItem, &value);
	EXPECT_EQ(*meshNode_1->instanceCount_, 13);
	EXPECT_EQ(*meshNode_2->instanceCount_, 13);

	commandInterface.set(intHandleMeshNode_2, 5);
	dispatch();

	intEditor.pasteProperty(&propertyBrowserItem, &value);
	EXPECT_EQ(*meshNode_1->instanceCount_, 13);
	EXPECT_EQ(*meshNode_2->instanceCount_, 13);
}

TEST_F(IntEditorTest, paste_property_from_bool) {
	Value<bool> value{true};

	intEditor.pasteProperty(&propertyBrowserItem, &value);
	EXPECT_EQ(*meshNode_1->instanceCount_, 1);
	EXPECT_EQ(*meshNode_2->instanceCount_, 1);
}

TEST_F(IntEditorTest, paste_property_from_string) {
	Value<std::string> value{"13"};

	intEditor.pasteProperty(&propertyBrowserItem, &value);
	EXPECT_EQ(*meshNode_1->instanceCount_, 13);
	EXPECT_EQ(*meshNode_2->instanceCount_, 13);

	commandInterface.set(intHandleMeshNode_2, 5);
	dispatch();

	intEditor.pasteProperty(&propertyBrowserItem, &value);
	EXPECT_EQ(*meshNode_1->instanceCount_, 13);
	EXPECT_EQ(*meshNode_2->instanceCount_, 13);
}

}  // namespace raco::property_browser
