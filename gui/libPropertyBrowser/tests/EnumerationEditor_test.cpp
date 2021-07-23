/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/editors/EnumerationEditor.h"
#include "EditorTestFixture.h"
#include "core/EditorObject.h"
#include "property_browser/PropertyBrowserItem.h"
#include "components/DataChangeDispatcher.h"
#include "testing/TestEnvironmentCore.h"
#include "testing/TestUtil.h"
#include "user_types/Material.h"

class EnumerationEditorFixture : public EditorTestFixture {};

TEST_F(EnumerationEditorFixture, noValueChangeRecorded_onCreation) {
	auto material{context.createObject(raco::user_types::Material::typeDescription.typeName)};
	dispatch();
	application.processEvents();

	raco::property_browser::PropertyBrowserItem item{{material, {"options", "blendOperationAlpha"}}, dataChangeDispatcher, &commandInterface, &model};
	raco::property_browser::EnumerationEditor editor{&item, nullptr};

	application.processEvents();

	ASSERT_FALSE(raco::isValueChanged(recorder, {material, {"options", "blendOperationAlpha"}}));
}

TEST_F(EnumerationEditorFixture, noValueChangeRecorded_onForeignSet) {
	auto material{context.createObject(raco::user_types::Material::typeDescription.typeName)};
	raco::property_browser::PropertyBrowserItem item{{material, {"options", "blendOperationAlpha"}}, dataChangeDispatcher, &commandInterface, &model};
	raco::property_browser::EnumerationEditor editor{&item, nullptr};
	dispatch();
	application.processEvents();
	ASSERT_EQ(0, editor.currentIndex());

	commandInterface.set({material, {"options", "blendOperationAlpha"}}, 4);
	dispatch();
	application.processEvents();

	ASSERT_EQ(4, editor.currentIndex());
	ASSERT_FALSE(raco::isValueChanged(recorder, {material, {"options", "blendOperationAlpha"}}));
}
