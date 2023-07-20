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
#include "property_browser/editors/VecNTEditor.h"

#include "property_browser/PropertyBrowserItem.h"
#include "application/RaCoProject.h"
#include "property_browser/editors/StringEditor.h"
#include "user_types/PerspectiveCamera.h"
#include "testing/TestUtil.h"

#include <gtest/gtest.h>

using namespace raco::core;
using namespace raco::user_types;

namespace raco::property_browser {


class ExposedVecNTEditor : public VecNTEditor<double, 4> {
public:
	ExposedVecNTEditor(PropertyBrowserItem *item) : VecNTEditor<double, 4>(item) {}

	void editSpinBoxValue(const int index, const double value) {
		spinboxes_[index]->Q_EMIT valueEdited(value);
	}

	void editSpinBoxValues(const double value) {
		std::for_each(spinboxes_.begin(), spinboxes_.end(), [&value](const auto& spinBox) {
			spinBox->Q_EMIT valueEdited(value);
		});
	}

	bool spinBoxHasMultipleValues(const int index) const {
		return spinboxes_[index]->hasMultipleValues();
	}

	bool spinBoxesHaveMultipleValues() const {
		return std::any_of(spinboxes_.begin(), spinboxes_.end(), [](const auto& spinBox) {
			return spinBox->hasMultipleValues();
		});
	}

	static void pasteProperty(PropertyBrowserItem* item, data_storage::ValueBase* value) {
		VecNTEditor<double, 4>::pasteProperty(item, value);
	}
};

class VecNTEditorTest : public EditorTestFixture {
public:
	SMaterial material_1 = this->create<Material>("material_1");
	ValueHandle vecHandleNode_1{material_1, {"options", "blendColor"}};

	SMaterial material_2 = this->create<Material>("material_2");
	ValueHandle vecHandleNode_2{material_2, {"options", "blendColor"}};
	
	PropertyBrowserItem propertyBrowserItem{{vecHandleNode_1, vecHandleNode_2}, this->dataChangeDispatcher, &this->commandInterface, &this->model};
	ExposedVecNTEditor vecNTEditor{&propertyBrowserItem};

	void setSpinBoxValue(const int index, const double value) {
		vecNTEditor.editSpinBoxValue(index, value);
		this->dispatch();
	}

	void setSpinBoxValues(const double value) {
		vecNTEditor.editSpinBoxValues(value);
		this->dispatch();
	}

	bool multipleValuesTextDisplayed(const int index) const {
		return vecNTEditor.spinBoxHasMultipleValues(index);
	}

	bool multipleValuesTextDisplayed() const {
		return vecNTEditor.spinBoxesHaveMultipleValues();
	}
};

TEST_F(VecNTEditorTest, set_multiple_values) {
	commandInterface.set(vecHandleNode_1, std::array<double, 4>({0.0, 1.0, 2.0, 3.0}));
	commandInterface.set(vecHandleNode_2, std::array<double, 4>({0.0, 1.0, 2.0, 3.0}));
	dispatch();

	setSpinBoxValues(5.0);

	checkVec4fValue({material_1, {"options", "blendColor"}}, {5.0, 5.0, 5.0, 5.0});
	checkVec4fValue({material_2, {"options", "blendColor"}}, {5.0, 5.0, 5.0, 5.0});

	setSpinBoxValue(0, 0.0);
	setSpinBoxValue(1, 1.0);
	setSpinBoxValue(2, 2.0);
	setSpinBoxValue(3, 3.0);

	checkVec4fValue({material_1, {"options", "blendColor"}}, {0.0, 1.0, 2.0, 3.0});
	checkVec4fValue({material_2, {"options", "blendColor"}}, {0.0, 1.0, 2.0, 3.0});
}

TEST_F(VecNTEditorTest, check_multiple_values_text_displayed) {
	commandInterface.set(vecHandleNode_1, std::array<double, 4>({0.0, 1.0, 2.0, 3.0}));
	commandInterface.set(vecHandleNode_2, std::array<double, 4>({0.0, 0.0, 0.0, 0.0}));
	dispatch();

	EXPECT_EQ(multipleValuesTextDisplayed(0), false);
	EXPECT_EQ(multipleValuesTextDisplayed(1), true);
	EXPECT_EQ(multipleValuesTextDisplayed(2), true);
	EXPECT_EQ(multipleValuesTextDisplayed(3), true);
	
	commandInterface.set(vecHandleNode_2[1], 1.0);
	commandInterface.set(vecHandleNode_2[2], 2.0);
	commandInterface.set(vecHandleNode_2[3], 3.0);
	dispatch();

	EXPECT_EQ(multipleValuesTextDisplayed(), false);

	commandInterface.set(vecHandleNode_1[0], 10.0);
	dispatch();

	EXPECT_EQ(multipleValuesTextDisplayed(), true);
}

TEST_F(VecNTEditorTest, paste_property_from_vec4f) {
	commandInterface.set(vecHandleNode_1, std::array<double, 4>({0.0, 1.0, 2.0, 3.0}));
	commandInterface.set(vecHandleNode_2, std::array<double, 4>({0.0, 1.0, 2.0, 3.0}));
	dispatch();

	Value<Vec4f> value{5.0};
	vecNTEditor.pasteProperty(&propertyBrowserItem, &value);

	checkVec4fValue({material_1, {"options", "blendColor"}}, {5.0, 5.0, 5.0, 5.0});
	checkVec4fValue({material_2, {"options", "blendColor"}}, {5.0, 5.0, 5.0, 5.0});
}

TEST_F(VecNTEditorTest, paste_property_from_vec3f) {
	commandInterface.set(vecHandleNode_1, std::array<double, 4>({0.0, 1.0, 2.0, 3.0}));
	commandInterface.set(vecHandleNode_2, std::array<double, 4>({0.0, 1.0, 2.0, 3.0}));
	dispatch();

	Value<Vec3f> value{5.0};
	vecNTEditor.pasteProperty(&propertyBrowserItem, &value);

	checkVec4fValue({material_1, {"options", "blendColor"}}, {5.0, 5.0, 5.0, 3.0});
	checkVec4fValue({material_2, {"options", "blendColor"}}, {5.0, 5.0, 5.0, 3.0});
}

TEST_F(VecNTEditorTest, paste_property_from_vec4i) {
	commandInterface.set(vecHandleNode_1, std::array<double, 4>({0.0, 1.0, 2.0, 3.0}));
	commandInterface.set(vecHandleNode_2, std::array<double, 4>({0.0, 1.0, 2.0, 3.0}));
	dispatch();

	Value<Vec4i> value{5};
	vecNTEditor.pasteProperty(&propertyBrowserItem, &value);

	checkVec4fValue({material_1, {"options", "blendColor"}}, {5.0, 5.0, 5.0, 5.0});
	checkVec4fValue({material_2, {"options", "blendColor"}}, {5.0, 5.0, 5.0, 5.0});
}

TEST_F(VecNTEditorTest, paste_property_from_vec3i) {
	commandInterface.set(vecHandleNode_1, std::array<double, 4>({0.0, 1.0, 2.0, 3.0}));
	commandInterface.set(vecHandleNode_2, std::array<double, 4>({0.0, 1.0, 2.0, 3.0}));
	dispatch();

	Value<Vec3i> value{5};
	vecNTEditor.pasteProperty(&propertyBrowserItem, &value);

	checkVec4fValue({material_1, {"options", "blendColor"}}, {5.0, 5.0, 5.0, 3.0});
	checkVec4fValue({material_2, {"options", "blendColor"}}, {5.0, 5.0, 5.0, 3.0});
}

}  // namespace raco::property_browser
