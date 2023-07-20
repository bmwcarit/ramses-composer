/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/editors/EnumerationEditor.h"
#include "EditorTestFixture.h"
#include "core/EditorObject.h"
#include "property_browser/PropertyBrowserItem.h"
#include "components/DataChangeDispatcher.h"
#include "testing/TestUtil.h"
#include "user_types/Material.h"
#include "user_types/Texture.h"

#include <ramses-client-api/TextureEnums.h>

#include <QComboBox>

class ExposedEnumerationEditor : public raco::property_browser::EnumerationEditor {
public:
	ExposedEnumerationEditor(raco::property_browser::PropertyBrowserItem* item, QWidget* parent) : EnumerationEditor(item, parent) {}

	QComboBox* comboBox() {
		return comboBox_;
	}

	void setComboBoxIndex(int index) {
		comboBox_->setCurrentIndex(index);
		comboBox_->Q_EMIT activated(index);
	}

	static void pasteProperty(raco::property_browser::PropertyBrowserItem* item, raco::data_storage::ValueBase* value) {
		raco::property_browser::EnumerationEditor::pasteProperty(item, value);
	}
};

class EnumerationEditorFixture : public EditorTestFixture {};

TEST_F(EnumerationEditorFixture, noValueChangeRecorded_onForeignSet) {
	auto material{context.createObject(raco::user_types::Material::typeDescription.typeName)};
	raco::core::ValueHandle handle{material, {"options", "blendOperationAlpha"}};
	std::set handles{handle};

	raco::property_browser::PropertyBrowserItem item{handles, dataChangeDispatcher, &commandInterface, &model};
	raco::property_browser::EnumerationEditor editor{&item, nullptr};
	dispatch();
	ASSERT_EQ(0, editor.currentIndex());

	commandInterface.set({material, {"options", "blendOperationAlpha"}}, 4);
	dispatch();

	ASSERT_EQ(4, editor.currentIndex());
	ASSERT_FALSE(recorder.hasValueChanged({material, {"options", "blendOperationAlpha"}}));
}

TEST_F(EnumerationEditorFixture, nonContinuousEnum) {
	auto texture{context.createObject(raco::user_types::Texture::typeDescription.typeName)};
	raco::core::ValueHandle handle{texture, {"textureFormat"}};
	const std::set handles{handle};

	raco::property_browser::PropertyBrowserItem item{handles, dataChangeDispatcher, &commandInterface, &model};
	raco::property_browser::EnumerationEditor editor{&item, nullptr};
	dispatch();
	ASSERT_EQ(3, editor.currentIndex());

	commandInterface.set({texture, {"textureFormat"}}, static_cast<int>(ramses::ETextureFormat::RGBA16F));
	dispatch();

	ASSERT_EQ(5, editor.currentIndex());
	ASSERT_FALSE(recorder.hasValueChanged({texture, {"textureFormat"}}));

	commandInterface.set({texture, {"textureFormat"}}, static_cast<int>(ramses::ETextureFormat::R8));
	dispatch();

	ASSERT_EQ(0, editor.currentIndex());
	ASSERT_FALSE(recorder.hasValueChanged({texture, {"textureFormat"}}));
}

TEST_F(EnumerationEditorFixture, multiselection_setup_single_value) {
	const auto material_1{context.createObject(raco::user_types::Material::typeDescription.typeName, "material_1")};
	const auto material_2{context.createObject(raco::user_types::Material::typeDescription.typeName, "material_2")};

	const raco::core::ValueHandle handle_1{material_1, {"options", "blendOperationAlpha"}};
	const raco::core::ValueHandle handle_2{material_2, {"options", "blendOperationAlpha"}};

	raco::property_browser::PropertyBrowserItem item{{handle_1, handle_2}, dataChangeDispatcher, &commandInterface, &model};
	raco::property_browser::EnumerationEditor editor{&item, nullptr};
	dispatch();
	application.processEvents();
	ASSERT_EQ(0, editor.currentIndex());

	// Set multiple values
	commandInterface.set({material_1, {"options", "blendOperationAlpha"}}, 4);
	dispatch();
	application.processEvents();
	ASSERT_EQ(-1, editor.currentIndex());

	// Set equal values
	commandInterface.set({material_2, {"options", "blendOperationAlpha"}}, 4);
	dispatch();
	application.processEvents();
	ASSERT_EQ(4, editor.currentIndex());
}

TEST_F(EnumerationEditorFixture, multiselection_setup_multiple_value) {
	const auto material_1{context.createObject(raco::user_types::Material::typeDescription.typeName, "material_1")};
	const auto material_2{context.createObject(raco::user_types::Material::typeDescription.typeName, "material_2")};

	const raco::core::ValueHandle handle_1{material_1, {"options", "blendOperationAlpha"}};
	const raco::core::ValueHandle handle_2{material_2, {"options", "blendOperationAlpha"}};

	// Set multiple values
	commandInterface.set({material_1, {"options", "blendOperationAlpha"}}, 4);

	raco::property_browser::PropertyBrowserItem item{{handle_1, handle_2}, dataChangeDispatcher, &commandInterface, &model};
	raco::property_browser::EnumerationEditor editor{&item, nullptr};
	dispatch();
	application.processEvents();
	ASSERT_EQ(-1, editor.currentIndex());

	// Set equal values
	commandInterface.set({material_2, {"options", "blendOperationAlpha"}}, 4);
	dispatch();
	application.processEvents();
	ASSERT_EQ(4, editor.currentIndex());
}

TEST_F(EnumerationEditorFixture, set_from_single_value) {
	const auto material_1{context.createObject(raco::user_types::Material::typeDescription.typeName, "material_1")};
	const auto material_2{context.createObject(raco::user_types::Material::typeDescription.typeName, "material_2")};

	const raco::core::ValueHandle handle_1{material_1, {"options", "blendOperationAlpha"}};
	const raco::core::ValueHandle handle_2{material_2, {"options", "blendOperationAlpha"}};

	raco::property_browser::PropertyBrowserItem item{{handle_1, handle_2}, dataChangeDispatcher, &commandInterface, &model};
	ExposedEnumerationEditor editor{&item, nullptr};
	dispatch();
	application.processEvents();
	ASSERT_EQ(0, editor.currentIndex());

	editor.setComboBoxIndex(4);
	dispatch();
	application.processEvents();

	EXPECT_EQ(handle_1.asInt(), 4);
	EXPECT_EQ(handle_2.asInt(), 4);
}

TEST_F(EnumerationEditorFixture, set_from_multi_value) {
	const auto material_1{context.createObject(raco::user_types::Material::typeDescription.typeName, "material_1")};
	const auto material_2{context.createObject(raco::user_types::Material::typeDescription.typeName, "material_2")};

	const raco::core::ValueHandle handle_1{material_1, {"options", "blendOperationAlpha"}};
	const raco::core::ValueHandle handle_2{material_2, {"options", "blendOperationAlpha"}};

	// Set multiple values
	commandInterface.set({material_1, {"options", "blendOperationAlpha"}}, 4);

	raco::property_browser::PropertyBrowserItem item{{handle_1, handle_2}, dataChangeDispatcher, &commandInterface, &model};
	ExposedEnumerationEditor editor{&item, nullptr};
	dispatch();
	application.processEvents();
	ASSERT_EQ(-1, editor.currentIndex());

	editor.setComboBoxIndex(2);
	dispatch();
	application.processEvents();

	EXPECT_EQ(handle_1.asInt(), 2);
	EXPECT_EQ(handle_2.asInt(), 2);
}


TEST_F(EnumerationEditorFixture, paste_property_from_int_in_range) {
	const auto material_1{context.createObject(raco::user_types::Material::typeDescription.typeName, "material_1")};
	const auto material_2{context.createObject(raco::user_types::Material::typeDescription.typeName, "material_2")};

	const raco::core::ValueHandle handle_1{material_1, {"options", "blendOperationAlpha"}};
	const raco::core::ValueHandle handle_2{material_2, {"options", "blendOperationAlpha"}};

	// Set multiple values
	commandInterface.set({material_1, {"options", "blendOperationAlpha"}}, 4);

	raco::property_browser::PropertyBrowserItem item{{handle_1, handle_2}, dataChangeDispatcher, &commandInterface, &model};
	ExposedEnumerationEditor editor{&item, nullptr};
	dispatch();
	application.processEvents();
	ASSERT_EQ(-1, editor.currentIndex());

	raco::data_storage::Value<int> value{2};

	editor.pasteProperty(&item, &value);
	EXPECT_EQ(handle_1.asInt(), 2);
	EXPECT_EQ(handle_2.asInt(), 2);

	commandInterface.set({material_1, {"options", "blendOperationAlpha"}}, 3);
	dispatch();

	editor.pasteProperty(&item, &value);
	EXPECT_EQ(handle_1.asInt(), 2);
	EXPECT_EQ(handle_2.asInt(), 2);
}

TEST_F(EnumerationEditorFixture, paste_property_from_int_out_of_range) {
	const auto material_1{context.createObject(raco::user_types::Material::typeDescription.typeName, "material_1")};
	const auto material_2{context.createObject(raco::user_types::Material::typeDescription.typeName, "material_2")};

	const raco::core::ValueHandle handle_1{material_1, {"options", "blendOperationAlpha"}};
	const raco::core::ValueHandle handle_2{material_2, {"options", "blendOperationAlpha"}};

	// Set multiple values
	commandInterface.set({material_1, {"options", "blendOperationAlpha"}}, 4);

	raco::property_browser::PropertyBrowserItem item{{handle_1, handle_2}, dataChangeDispatcher, &commandInterface, &model};
	ExposedEnumerationEditor editor{&item, nullptr};
	dispatch();
	application.processEvents();
	ASSERT_EQ(-1, editor.currentIndex());

	raco::data_storage::Value<int> value{12};

	editor.pasteProperty(&item, &value);
	EXPECT_EQ(handle_1.asInt(), 4);
	EXPECT_EQ(handle_2.asInt(), 0);
}
