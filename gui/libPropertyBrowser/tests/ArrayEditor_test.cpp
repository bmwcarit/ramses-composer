/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/editors/ArrayEditor.h"
#include "EditorTestFixture.h"

#include "application/RaCoProject.h"
#include "property_browser/PropertyBrowserItem.h"

#include <gtest/gtest.h>

using namespace raco::core;
using namespace raco::user_types;

namespace raco::property_browser {

class ExposedArrayEditor : public ArrayEditor {
public:
	ExposedArrayEditor(PropertyBrowserItem* item) : ArrayEditor(item) {}

	QString sizeLabel() const {
		return sizeLabel_->text();
	}

	void grow() {
		Q_EMIT growButton_->clicked();
	}

	void shrink() {
		Q_EMIT shrinkButton_->clicked();
	}
};

class ArrayEditorTest : public EditorTestFixture {
public:
	ArrayEditorTest() : EditorTestFixture(&TestObjectFactory::getInstance()) {}

	using SObjectWithArrays = std::shared_ptr<ObjectWithArrays>;

	SObjectWithArrays object_1 = this->create<ObjectWithArrays>("object1");
	SObjectWithArrays object_2 = this->create<ObjectWithArrays>("object2");
	const ValueHandle arrayHandle_1{object_1, &ObjectWithArrays::array_ref_resizable_};
	const ValueHandle arrayHandle_2{object_2, &ObjectWithArrays::array_ref_resizable_};

	PropertyBrowserItem propertyBrowserItem{{arrayHandle_1, arrayHandle_2}, this->dataChangeDispatcher, &this->commandInterface, &this->model};
	ExposedArrayEditor editor{&propertyBrowserItem};

	void grow() {
		editor.grow();
		dispatch();
	}

	void shrink() {
		editor.shrink();
		dispatch();
	}
};

TEST_F(ArrayEditorTest, update_model) {
	dispatch();

	EXPECT_EQ(editor.sizeLabel(), "0");

	commandInterface.resizeArray(arrayHandle_1, 3);
	commandInterface.resizeArray(arrayHandle_2, 3);
	dispatch();

	EXPECT_EQ(editor.sizeLabel(), "3");
}

TEST_F(ArrayEditorTest, editor_activate_grow) {
	dispatch();

	EXPECT_EQ(object_1->array_ref_resizable_->size(), 0);
	EXPECT_EQ(object_2->array_ref_resizable_->size(), 0);
	EXPECT_EQ(editor.sizeLabel(), "0");

	editor.grow();
	dispatch();

	EXPECT_EQ(object_1->array_ref_resizable_->size(), 1);
	EXPECT_EQ(object_2->array_ref_resizable_->size(), 1);
	EXPECT_EQ(editor.sizeLabel(), "1");
}

TEST_F(ArrayEditorTest, editor_activate_shrink) {
	commandInterface.resizeArray(arrayHandle_1, 2);
	commandInterface.resizeArray(arrayHandle_2, 2);
	dispatch();

	EXPECT_EQ(object_1->array_ref_resizable_->size(), 2);
	EXPECT_EQ(object_2->array_ref_resizable_->size(), 2);
	EXPECT_EQ(editor.sizeLabel(), "2");

	editor.shrink();
	dispatch();

	EXPECT_EQ(object_1->array_ref_resizable_->size(), 1);
	EXPECT_EQ(object_2->array_ref_resizable_->size(), 1);
	EXPECT_EQ(editor.sizeLabel(), "1");

	editor.shrink();
	dispatch();

	EXPECT_EQ(object_1->array_ref_resizable_->size(), 1);
	EXPECT_EQ(object_2->array_ref_resizable_->size(), 1);
	EXPECT_EQ(editor.sizeLabel(), "1");
}

}  // namespace raco::property_browser
