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
#include "property_browser/editors/LinkEditor.h"
#include "property_browser/editors/LinkEditorPopup.h"

#include "property_browser/PropertyBrowserItem.h"
#include "application/RaCoProject.h"

#include <gtest/gtest.h>

using namespace raco::core;
using namespace raco::user_types;

namespace raco::property_browser {

class ExposedLinkEditor : public LinkEditor {
public:
	ExposedLinkEditor(PropertyBrowserItem* item) : LinkEditor(item, nullptr) {}

	bool isLinkButtonEnabled() const {
		return linkButton_->isEnabled();
	}

	QString getLinkButtonToolTip() const {
		return linkButton_->toolTip();
	}

	LinkIcon getLinkButtonIcon() const {
		return linkButtonIcon_;
	}

	bool isGoToLinkButtonEnabled() const {
		return goToLinkButton_->isEnabled();
	}

	QString getGoToLinkButtonToolTip() const {
		return goToLinkButton_->toolTip();
	}

	LinkIcon getGoToLinkButtonIcon() const {
		return goToLinkButtonIcon_;
	}
};

class ExposedLinkEditorPopup : public LinkEditorPopup {
public:
	ExposedLinkEditorPopup(PropertyBrowserItem* item) : LinkEditorPopup(item, new QPushButton()) {}

	QString currentRelationText() const {
		return currentRelation_.text();
	}

	bool isCurrentRelationVisible() const {
		return currentRelation_.isVisible();
	}

	bool isDeleteButtonVisible() const {
		return deleteButton_.isVisible();
	}
};

class LinkEditorTest : public EditorTestFixture {
public:
	LinkEditorTest() : EditorTestFixture() {}
};

TEST_F(LinkEditorTest, add_remove_link_to_lua_script) {
	const SLuaScript luaScript_1 = create_lua("luaScript_1", "scripts/types-scalar.lua");
	const SLuaScript luaScript_2 = create_lua("luaScript_2", "scripts/types-scalar.lua");

	const ValueHandle scriptInputHandle_1{luaScript_1, {"inputs", "float"}};
	const ValueHandle scriptInputHandle_2{luaScript_2, {"inputs", "float"}};
	const ValueHandle scriptOutputHandle_2{luaScript_2, {"outputs", "ofloat"}};

	// add link
	commandInterface.addLink(scriptOutputHandle_2, scriptInputHandle_1);
	dispatch();

	// single selection case
	PropertyBrowserItem single_item{{scriptInputHandle_1}, dataChangeDispatcher, &commandInterface, nullptr};
	const auto single_linkEditor = ExposedLinkEditor(&single_item);
	const auto single_linkEditorPopup = ExposedLinkEditorPopup(&single_item);

	EXPECT_TRUE(single_linkEditor.isLinkButtonEnabled());
	EXPECT_EQ(single_linkEditor.getLinkButtonIcon(), LinkEditor::LinkIcon::linked);
	EXPECT_EQ(single_linkEditor.getLinkButtonToolTip(), "luaScript_2.outputs.ofloat");

	EXPECT_TRUE(single_linkEditor.isGoToLinkButtonEnabled());
	EXPECT_EQ(single_linkEditor.getGoToLinkButtonIcon(), LinkEditor::LinkIcon::doubleArrowLeft);
	EXPECT_EQ(single_linkEditor.getGoToLinkButtonToolTip(), "Go to link start (luaScript_2.outputs.ofloat)");

	EXPECT_TRUE(single_linkEditorPopup.isDeleteButtonVisible());
	EXPECT_TRUE(single_linkEditorPopup.isCurrentRelationVisible());
	EXPECT_EQ(single_linkEditorPopup.currentRelationText(), "luaScript_2.outputs.ofloat");

	// multiple selection case
	PropertyBrowserItem multi_item{{scriptInputHandle_1, scriptInputHandle_2}, dataChangeDispatcher, &commandInterface, nullptr};
	const auto multi_linkEditor = ExposedLinkEditor(&multi_item);
	const auto multi_linkEditorPopup = ExposedLinkEditorPopup(&multi_item);

	EXPECT_TRUE(multi_linkEditor.isLinkButtonEnabled());
	EXPECT_EQ(multi_linkEditor.getLinkButtonIcon(), LinkEditor::LinkIcon::linkMultipleValues);
	EXPECT_EQ(multi_linkEditor.getLinkButtonToolTip(), "Multiple values");

	EXPECT_FALSE(multi_linkEditor.isGoToLinkButtonEnabled());
	EXPECT_EQ(multi_linkEditor.getGoToLinkButtonIcon(), LinkEditor::LinkIcon::unlinkable);
	EXPECT_EQ(multi_linkEditor.getGoToLinkButtonToolTip(), "");
	
	EXPECT_TRUE(multi_linkEditorPopup.isDeleteButtonVisible());
	EXPECT_TRUE(multi_linkEditorPopup.isCurrentRelationVisible());
	EXPECT_EQ(multi_linkEditorPopup.currentRelationText(), "Multiple values");

	// remove link
	commandInterface.removeLink(scriptInputHandle_1.getDescriptor());
	dispatch();

	// single selection case
	EXPECT_TRUE(single_linkEditor.isLinkButtonEnabled());
	EXPECT_EQ(single_linkEditor.getLinkButtonIcon(), LinkEditor::LinkIcon::linkable);
	EXPECT_EQ(single_linkEditor.getLinkButtonToolTip(), "");

	EXPECT_FALSE(single_linkEditor.isGoToLinkButtonEnabled());
	EXPECT_EQ(single_linkEditor.getGoToLinkButtonIcon(), LinkEditor::LinkIcon::unlinkable);
	EXPECT_EQ(single_linkEditor.getGoToLinkButtonToolTip(), "");

	EXPECT_FALSE(single_linkEditorPopup.isCurrentRelationVisible());
	EXPECT_FALSE(single_linkEditorPopup.isDeleteButtonVisible());
	EXPECT_EQ(single_linkEditorPopup.currentRelationText(), "");

	// multiple selection case
	EXPECT_TRUE(multi_linkEditor.isLinkButtonEnabled());
	EXPECT_EQ(multi_linkEditor.getLinkButtonIcon(), LinkEditor::LinkIcon::linkable);
	EXPECT_EQ(multi_linkEditor.getLinkButtonToolTip(), "");

	EXPECT_FALSE(multi_linkEditor.isGoToLinkButtonEnabled());
	EXPECT_EQ(multi_linkEditor.getGoToLinkButtonIcon(), LinkEditor::LinkIcon::unlinkable);
	EXPECT_EQ(multi_linkEditor.getGoToLinkButtonToolTip(), "");

	EXPECT_FALSE(multi_linkEditorPopup.isCurrentRelationVisible());
	EXPECT_FALSE(multi_linkEditorPopup.isDeleteButtonVisible());
	EXPECT_EQ(multi_linkEditorPopup.currentRelationText(), "");
}

TEST_F(LinkEditorTest, delete_one_of_the_selected_objects) {
	const SLuaScript luaScript_1 = create_lua("luaScript_1", "scripts/types-scalar.lua");
	const SLuaScript luaScript_2 = create_lua("luaScript_2", "scripts/types-scalar.lua");

	const ValueHandle scriptInputHandle_1{luaScript_1, {"inputs", "float"}};
	const ValueHandle scriptInputHandle_2{luaScript_2, {"inputs", "float"}};
	const ValueHandle scriptOutputHandle_2{luaScript_2, {"outputs", "ofloat"}};

	// add link
	commandInterface.addLink(scriptOutputHandle_2, scriptInputHandle_1);
	dispatch();

	// single selection case
	PropertyBrowserItem single_item{{scriptInputHandle_1}, dataChangeDispatcher, &commandInterface, nullptr};
	const auto single_linkEditor = ExposedLinkEditor(&single_item);
	const auto single_linkEditorPopup = ExposedLinkEditorPopup(&single_item);

	EXPECT_TRUE(single_linkEditor.isLinkButtonEnabled());
	EXPECT_EQ(single_linkEditor.getLinkButtonIcon(), LinkEditor::LinkIcon::linked);
	EXPECT_EQ(single_linkEditor.getLinkButtonToolTip(), "luaScript_2.outputs.ofloat");

	EXPECT_TRUE(single_linkEditor.isGoToLinkButtonEnabled());
	EXPECT_EQ(single_linkEditor.getGoToLinkButtonIcon(), LinkEditor::LinkIcon::doubleArrowLeft);
	EXPECT_EQ(single_linkEditor.getGoToLinkButtonToolTip(), "Go to link start (luaScript_2.outputs.ofloat)");

	EXPECT_TRUE(single_linkEditorPopup.isDeleteButtonVisible());
	EXPECT_TRUE(single_linkEditorPopup.isCurrentRelationVisible());
	EXPECT_EQ(single_linkEditorPopup.currentRelationText(), "luaScript_2.outputs.ofloat");

	// multiple selection case
	PropertyBrowserItem multi_item{{scriptInputHandle_1, scriptInputHandle_2}, dataChangeDispatcher, &commandInterface, nullptr};
	const auto multi_linkEditor = ExposedLinkEditor(&multi_item);
	const auto multi_linkEditorPopup = ExposedLinkEditorPopup(&multi_item);

	EXPECT_TRUE(multi_linkEditor.isLinkButtonEnabled());
	EXPECT_EQ(multi_linkEditor.getLinkButtonIcon(), LinkEditor::LinkIcon::linkMultipleValues);
	EXPECT_EQ(multi_linkEditor.getLinkButtonToolTip(), "Multiple values");

	EXPECT_FALSE(multi_linkEditor.isGoToLinkButtonEnabled());
	EXPECT_EQ(multi_linkEditor.getGoToLinkButtonIcon(), LinkEditor::LinkIcon::unlinkable);
	EXPECT_EQ(multi_linkEditor.getGoToLinkButtonToolTip(), "");

	EXPECT_TRUE(multi_linkEditorPopup.isDeleteButtonVisible());
	EXPECT_TRUE(multi_linkEditorPopup.isCurrentRelationVisible());
	EXPECT_EQ(multi_linkEditorPopup.currentRelationText(), "Multiple values");

	// remove link
	commandInterface.deleteObjects(std::vector<SEditorObject>{luaScript_2});
	dispatch();

	// single selection case
	EXPECT_TRUE(single_linkEditor.isLinkButtonEnabled());
	EXPECT_EQ(single_linkEditor.getLinkButtonIcon(), LinkEditor::LinkIcon::linkable);
	EXPECT_EQ(single_linkEditor.getLinkButtonToolTip(), "");

	EXPECT_FALSE(single_linkEditor.isGoToLinkButtonEnabled());
	EXPECT_EQ(single_linkEditor.getGoToLinkButtonIcon(), LinkEditor::LinkIcon::unlinkable);
	EXPECT_EQ(single_linkEditor.getGoToLinkButtonToolTip(), "");

	EXPECT_FALSE(single_linkEditorPopup.isCurrentRelationVisible());
	EXPECT_FALSE(single_linkEditorPopup.isDeleteButtonVisible());
	EXPECT_EQ(single_linkEditorPopup.currentRelationText(), "");

	// multiple selection case
	EXPECT_TRUE(multi_linkEditor.isLinkButtonEnabled());
	EXPECT_EQ(multi_linkEditor.getLinkButtonIcon(), LinkEditor::LinkIcon::linkable);
	EXPECT_EQ(multi_linkEditor.getLinkButtonToolTip(), "");

	EXPECT_FALSE(multi_linkEditor.isGoToLinkButtonEnabled());
	EXPECT_EQ(multi_linkEditor.getGoToLinkButtonIcon(), LinkEditor::LinkIcon::unlinkable);
	EXPECT_EQ(multi_linkEditor.getGoToLinkButtonToolTip(), "");

	EXPECT_FALSE(multi_linkEditorPopup.isCurrentRelationVisible());
	EXPECT_FALSE(multi_linkEditorPopup.isDeleteButtonVisible());
	EXPECT_EQ(multi_linkEditorPopup.currentRelationText(), "");
}

TEST_F(LinkEditorTest, change_script_uri_to_empty) {
	const SLuaScript luaScript_1 = create_lua("luaScript_1", "scripts/types-scalar.lua");
	const SLuaScript luaScript_2 = create_lua("luaScript_2", "scripts/types-scalar.lua");

	const SLuaInterface luaInterface_1 = create_lua_interface("luaInterface_1", "scripts/interface-scalar-types.lua");
	const SLuaInterface luaInterface_2 = create_lua_interface("luaInterface_2", "scripts/interface-scalar-types.lua");

	const ValueHandle scriptHandleInput_1{luaScript_1, {"inputs", "float"}};

	const ValueHandle scriptHandleOutput_1{luaScript_1, {"outputs", "ofloat"}};
	const ValueHandle scriptHandleOutput_2{luaScript_2, {"outputs", "ofloat"}};

	const ValueHandle interfaceHandle_1{luaInterface_1, {"inputs", "float"}};
	const ValueHandle interfaceHandle_2{luaInterface_2, {"inputs", "float"}};

	const ValueHandle uriScriptHandle_1{luaScript_1, {"uri"}};

	// add link
	commandInterface.addLink(scriptHandleOutput_2, scriptHandleInput_1);
	commandInterface.addLink(scriptHandleOutput_1, interfaceHandle_1);
	commandInterface.addLink(interfaceHandle_2, scriptHandleInput_1);
	dispatch();

	// single selection case
	PropertyBrowserItem single_item{{interfaceHandle_1}, dataChangeDispatcher, &commandInterface, nullptr};
	const auto single_linkEditor = ExposedLinkEditor(&single_item);
	const auto single_linkEditorPopup = ExposedLinkEditorPopup(&single_item);

	EXPECT_TRUE(single_linkEditor.isLinkButtonEnabled());
	EXPECT_EQ(single_linkEditor.getLinkButtonIcon(), LinkEditor::LinkIcon::linked);
	EXPECT_EQ(single_linkEditor.getLinkButtonToolTip(), "luaScript_1.outputs.ofloat");

	EXPECT_TRUE(single_linkEditor.isGoToLinkButtonEnabled());
	EXPECT_EQ(single_linkEditor.getGoToLinkButtonIcon(), LinkEditor::LinkIcon::doubleArrowLeft);
	EXPECT_EQ(single_linkEditor.getGoToLinkButtonToolTip(), "Go to link start (luaScript_1.outputs.ofloat)");

	EXPECT_TRUE(single_linkEditorPopup.isCurrentRelationVisible());
	EXPECT_TRUE(single_linkEditorPopup.isDeleteButtonVisible());
	EXPECT_EQ(single_linkEditorPopup.currentRelationText(), "luaScript_1.outputs.ofloat");

	// multiple selection case
	PropertyBrowserItem multi_item{{interfaceHandle_1, interfaceHandle_2}, dataChangeDispatcher, &commandInterface, nullptr};
	const auto multi_linkEditor = ExposedLinkEditor(&multi_item);
	const auto multi_linkEditorPopup = ExposedLinkEditorPopup(&multi_item);

	EXPECT_TRUE(multi_linkEditor.isLinkButtonEnabled());
	EXPECT_EQ(multi_linkEditor.getLinkButtonIcon(), LinkEditor::LinkIcon::linkMultipleValues);
	EXPECT_EQ(multi_linkEditor.getLinkButtonToolTip(), "Multiple values");

	EXPECT_TRUE(multi_linkEditor.isGoToLinkButtonEnabled());
	EXPECT_EQ(multi_linkEditor.getGoToLinkButtonIcon(), LinkEditor::LinkIcon::goTo);
	EXPECT_EQ(multi_linkEditor.getGoToLinkButtonToolTip(), "Go to link ends...");

	EXPECT_TRUE(multi_linkEditorPopup.isCurrentRelationVisible());
	EXPECT_TRUE(multi_linkEditorPopup.isDeleteButtonVisible());
	EXPECT_EQ(multi_linkEditorPopup.currentRelationText(), "Multiple values");

	// set empty uri
	commandInterface.set(uriScriptHandle_1, std::string{});
	dispatch();

	// single selection case
	EXPECT_TRUE(single_linkEditor.isLinkButtonEnabled());
	EXPECT_EQ(single_linkEditor.getLinkButtonIcon(), LinkEditor::LinkIcon::linkBroken);
	EXPECT_EQ(single_linkEditor.getLinkButtonToolTip(), "luaScript_1.outputs.ofloat (broken)");

	EXPECT_TRUE(single_linkEditor.isGoToLinkButtonEnabled());
	EXPECT_EQ(single_linkEditor.getGoToLinkButtonIcon(), LinkEditor::LinkIcon::doubleArrowLeft);
	EXPECT_EQ(single_linkEditor.getGoToLinkButtonToolTip(), "Go to link start (luaScript_1.outputs.ofloat)");

	EXPECT_FALSE(single_linkEditorPopup.isCurrentRelationVisible());
	EXPECT_FALSE(single_linkEditorPopup.isDeleteButtonVisible());
	EXPECT_EQ(single_linkEditorPopup.currentRelationText(), "luaScript_1.outputs.ofloat (broken)");

	// multiple selection case
	EXPECT_TRUE(multi_linkEditor.isLinkButtonEnabled());
	EXPECT_EQ(multi_linkEditor.getLinkButtonIcon(), LinkEditor::LinkIcon::linkMultipleValues);
	EXPECT_EQ(multi_linkEditor.getLinkButtonToolTip(), "Multiple values");

	EXPECT_FALSE(multi_linkEditor.isGoToLinkButtonEnabled());
	EXPECT_EQ(multi_linkEditor.getGoToLinkButtonIcon(), LinkEditor::LinkIcon::unlinkable);
	EXPECT_EQ(multi_linkEditor.getGoToLinkButtonToolTip(), "");

	EXPECT_TRUE(multi_linkEditorPopup.isCurrentRelationVisible());
	EXPECT_TRUE(multi_linkEditorPopup.isDeleteButtonVisible());
	EXPECT_EQ(multi_linkEditorPopup.currentRelationText(), "Multiple values");
 }


}  // namespace raco::property_browser
