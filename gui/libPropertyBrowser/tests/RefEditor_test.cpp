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
#include "property_browser/editors/RefEditor.h"
#include "user_types/RenderPass.h"

#include "property_browser/PropertyBrowserItem.h"
#include "application/RaCoProject.h"

#include <QLineEdit>
#include <QPushButton>
#include <gtest/gtest.h>

using namespace raco::core;
using namespace raco::user_types;

namespace raco::property_browser {

class ExposedRefEditor : public RefEditor {
public:
	ExposedRefEditor(PropertyBrowserItem* item) : RefEditor(item, nullptr) {}

	QString getCurrentRefText() const {
		return currentRef_->text();
	}

	bool isRefEmpty() const {
		return unexpectedEmptyReference();
	}

	bool isCurrentRefTextEnabled() const {
		return currentRef_->isEnabled();
	}

	QString getCurrentRefToolTip() const {
		return currentRef_->toolTip();
	}	

	bool isChangeRefButtonEnabled() const {
		return changeRefButton_->isEnabled();
	}

	bool isGoToRefButtonEnabled() const {
		return goToRefObjectButton_->isEnabled();
	}

	QStringList getRefItems() const {
		QStringList refItems;

		auto items = item_->refItem()->items();
		std::for_each(items.begin(), items.end(), [&refItems](const PropertyBrowserRef::RefItem& item) {
			refItems.append(item.objName);
		});

		return refItems;
	}

	static void pasteProperty(PropertyBrowserItem* item, data_storage::ValueBase* value) {
		RefEditor::pasteProperty(item, value);
	}
};

class RefEditorTest : public EditorTestFixture {
public:
	RefEditorTest() : EditorTestFixture() {}

	const SPrefab prefab_1 = create<Prefab>("prefab_1");
	const SPrefab prefab_2 = create<Prefab>("prefab_2");
	const SPrefab prefab_3 = create<Prefab>("prefab_3");
	const SPrefab prefab_4 = create<Prefab>("prefab_4");

	const SPrefabInstance inst_1 = create<PrefabInstance>("inst_1", prefab_1);
	const SPrefabInstance inst_2 = create<PrefabInstance>("inst_2", prefab_2);

	ValueHandle handle_1{inst_1, &PrefabInstance::template_};
	ValueHandle handle_2{inst_2, &PrefabInstance::template_};
};

TEST_F(RefEditorTest, single_selection_setup) {
	commandInterface.set(handle_1, prefab_3);
	dispatch();

	PropertyBrowserItem item{{handle_1}, dataChangeDispatcher, &commandInterface, nullptr};
	const auto refEditor = ExposedRefEditor(&item);

	EXPECT_EQ(refEditor.getRefItems(), QStringList({"<empty>", "prefab_2", "prefab_3", "prefab_4"}));
	EXPECT_EQ(refEditor.getCurrentRefText(), "prefab_3");
	EXPECT_EQ(refEditor.getCurrentRefToolTip(), "prefab_3");

	EXPECT_TRUE(refEditor.isCurrentRefTextEnabled());
	EXPECT_TRUE(refEditor.isChangeRefButtonEnabled());
	EXPECT_TRUE(refEditor.isGoToRefButtonEnabled());
	EXPECT_FALSE(refEditor.isRefEmpty());
}

TEST_F(RefEditorTest, single_selection_setup_empty_expected) {
	SRenderPass renderpass = create<RenderPass>("renderpass");

	dispatch();

	PropertyBrowserItem item0 {{ValueHandle(renderpass, &RenderPass::layer0_)}, dataChangeDispatcher, &commandInterface, nullptr};
	const auto refEditor0 = ExposedRefEditor(&item0);
	PropertyBrowserItem item1{{ValueHandle(renderpass, &RenderPass::layer1_)}, dataChangeDispatcher, &commandInterface, nullptr};
	const auto refEditor1 = ExposedRefEditor(&item1);

	EXPECT_EQ(refEditor0.getCurrentRefText(), "<empty>");
	EXPECT_EQ(refEditor1.getCurrentRefText(), "<empty>");
	EXPECT_TRUE(refEditor0.isRefEmpty());
	EXPECT_FALSE(refEditor1.isRefEmpty());
}

TEST_F(RefEditorTest, setup_single_value) {
	commandInterface.set(handle_1, prefab_3);
	commandInterface.set(handle_2, prefab_3);
	dispatch();

	PropertyBrowserItem item{{handle_1, handle_2}, dataChangeDispatcher, &commandInterface, nullptr};
	const auto refEditor = ExposedRefEditor(&item);

	EXPECT_EQ(refEditor.getRefItems(), QStringList({"<empty>", "prefab_3", "prefab_4"}));
	
	EXPECT_EQ(refEditor.getCurrentRefText(), "prefab_3");
	EXPECT_EQ(refEditor.getCurrentRefToolTip(), "prefab_3");
	
	EXPECT_TRUE(refEditor.isCurrentRefTextEnabled());
	EXPECT_TRUE(refEditor.isChangeRefButtonEnabled());
	EXPECT_TRUE(refEditor.isGoToRefButtonEnabled());
	EXPECT_FALSE(refEditor.isRefEmpty());
}

TEST_F(RefEditorTest, setup_multiple_value) {
	commandInterface.set(handle_1, prefab_3);
	commandInterface.set(handle_2, prefab_4);
	dispatch();

	PropertyBrowserItem item{{handle_1, handle_2}, dataChangeDispatcher, &commandInterface, nullptr};
	const auto refEditor = ExposedRefEditor(&item);

	EXPECT_EQ(refEditor.getRefItems(), QStringList({"<empty>", "prefab_3", "prefab_4"}));

	EXPECT_EQ(refEditor.getCurrentRefText(), PropertyBrowserItem::MultipleValueText);
	EXPECT_EQ(refEditor.getCurrentRefToolTip(), PropertyBrowserItem::MultipleValueText);

	EXPECT_TRUE(refEditor.isCurrentRefTextEnabled());
	EXPECT_TRUE(refEditor.isChangeRefButtonEnabled());
	EXPECT_FALSE(refEditor.isGoToRefButtonEnabled());
	EXPECT_FALSE(refEditor.isRefEmpty());
}

TEST_F(RefEditorTest, setup_empty_value) {
	PropertyBrowserItem item{{handle_1, handle_2}, dataChangeDispatcher, &commandInterface, nullptr};
	const auto refEditor = ExposedRefEditor(&item);

	EXPECT_EQ(refEditor.getRefItems(), QStringList({"<empty>", "prefab_3", "prefab_4"}));
	EXPECT_EQ(refEditor.getCurrentRefText(), "<empty>");
	EXPECT_EQ(refEditor.getCurrentRefToolTip(), "");

	EXPECT_TRUE(refEditor.isCurrentRefTextEnabled());
	EXPECT_TRUE(refEditor.isChangeRefButtonEnabled());
	EXPECT_FALSE(refEditor.isGoToRefButtonEnabled());
	EXPECT_TRUE(refEditor.isRefEmpty());
}

TEST_F(RefEditorTest, set_index) {
	commandInterface.set(handle_1, prefab_3);
	commandInterface.set(handle_2, prefab_4);
	dispatch();

	PropertyBrowserItem item{{handle_1, handle_2}, dataChangeDispatcher, &commandInterface, nullptr};
	const auto refEditor = ExposedRefEditor(&item);

	EXPECT_EQ(refEditor.getRefItems(), QStringList({"<empty>", "prefab_3", "prefab_4"}));
	// Emulate the action taken by the RefEditor:
	item.refItem()->setIndex(2);
	dispatch();

	EXPECT_EQ(*inst_1->template_, prefab_4);
	EXPECT_EQ(*inst_2->template_, prefab_4);

	EXPECT_EQ(refEditor.getCurrentRefText(), "prefab_4");
	EXPECT_EQ(refEditor.getCurrentRefToolTip(), "prefab_4");

	EXPECT_TRUE(refEditor.isCurrentRefTextEnabled());
	EXPECT_TRUE(refEditor.isChangeRefButtonEnabled());
	EXPECT_TRUE(refEditor.isGoToRefButtonEnabled());

	EXPECT_FALSE(refEditor.isRefEmpty());
}

TEST_F(RefEditorTest, paste_property_target_valid) {
	commandInterface.set(handle_1, prefab_3);
	commandInterface.set(handle_2, prefab_4);
	dispatch();

	PropertyBrowserItem item{{handle_1, handle_2}, dataChangeDispatcher, &commandInterface, nullptr};
	const auto refEditor = ExposedRefEditor(&item);

	EXPECT_EQ(refEditor.getRefItems(), QStringList({"<empty>", "prefab_3", "prefab_4"}));
	EXPECT_EQ(refEditor.getCurrentRefText(), PropertyBrowserItem::MultipleValueText);
	EXPECT_EQ(refEditor.getCurrentRefToolTip(), PropertyBrowserItem::MultipleValueText);

	EXPECT_TRUE(refEditor.isCurrentRefTextEnabled());
	EXPECT_TRUE(refEditor.isChangeRefButtonEnabled());

	EXPECT_FALSE(refEditor.isGoToRefButtonEnabled());
	EXPECT_FALSE(refEditor.isRefEmpty());

	Value<SPrefab> value{prefab_4};
	refEditor.pasteProperty(&item, &value);
	dispatch();

	EXPECT_EQ(*inst_1->template_, prefab_4);
	EXPECT_EQ(*inst_2->template_, prefab_4);

	EXPECT_EQ(refEditor.getCurrentRefText(), "prefab_4");
	EXPECT_EQ(refEditor.getCurrentRefToolTip(), "prefab_4");

	EXPECT_TRUE(refEditor.isCurrentRefTextEnabled());
	EXPECT_TRUE(refEditor.isChangeRefButtonEnabled());
	EXPECT_TRUE(refEditor.isGoToRefButtonEnabled());

	EXPECT_FALSE(refEditor.isRefEmpty());
}

TEST_F(RefEditorTest, paste_property_target_invalid) {
	commandInterface.set(handle_1, prefab_3);
	commandInterface.set(handle_2, prefab_4);
	dispatch();

	PropertyBrowserItem item{{handle_1, handle_2}, dataChangeDispatcher, &commandInterface, nullptr};
	const auto refEditor = ExposedRefEditor(&item);

	EXPECT_EQ(refEditor.getRefItems(), QStringList({"<empty>", "prefab_3", "prefab_4"}));
	EXPECT_EQ(refEditor.getCurrentRefText(), PropertyBrowserItem::MultipleValueText);
	EXPECT_EQ(refEditor.getCurrentRefToolTip(), PropertyBrowserItem::MultipleValueText);

	EXPECT_TRUE(refEditor.isCurrentRefTextEnabled());
	EXPECT_TRUE(refEditor.isChangeRefButtonEnabled());

	EXPECT_FALSE(refEditor.isGoToRefButtonEnabled());
	EXPECT_FALSE(refEditor.isRefEmpty());

	Value<SPrefab> value{prefab_1};
	refEditor.pasteProperty(&item, &value);
	dispatch();

	EXPECT_EQ(*inst_1->template_, prefab_3);
	EXPECT_EQ(*inst_2->template_, prefab_4);

	EXPECT_EQ(refEditor.getCurrentRefText(), PropertyBrowserItem::MultipleValueText);
	EXPECT_EQ(refEditor.getCurrentRefToolTip(), PropertyBrowserItem::MultipleValueText);

	EXPECT_TRUE(refEditor.isCurrentRefTextEnabled());
	EXPECT_TRUE(refEditor.isChangeRefButtonEnabled());

	EXPECT_FALSE(refEditor.isGoToRefButtonEnabled());
	EXPECT_FALSE(refEditor.isRefEmpty());
}

}  // namespace raco::property_browser
