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
#include "property_browser/editors/TagContainerEditor.h"
#include "../src/editors/TagContainerEditor/TagContainerEditor_Popup.h"
#include "../src/editors/TagContainerEditor/TagContainerEditor_AppliedTagModel.h"

#include "property_browser/PropertyBrowserItem.h"
#include "application/RaCoProject.h"
#include "user_types/RenderLayer.h"

#include <QDialog>
#include <QListWidget>
#include <gtest/gtest.h>

using namespace raco::core;
using namespace raco::user_types;

namespace raco::property_browser {


class ExposedTagContainerEditor : public TagContainerEditor {
public:
	ExposedTagContainerEditor(PropertyBrowserItem *item) : TagContainerEditor(item, nullptr) {}

	std::vector<std::string> getTagsList() const {
		std::vector<std::string> tagsList;
		for (auto item : tagList_->findItems("*", Qt::MatchWildcard)){
			tagsList.push_back(item->text().toStdString());
		}
		return tagsList;
	}
};

class ExposedTagContainerEditor_Popup : public TagContainerEditor_Popup {
public:
	ExposedTagContainerEditor_Popup(PropertyBrowserItem* item, TagType tagType, QWidget* anchor) : TagContainerEditor_Popup(item, tagType, anchor) {}

	std::vector<std::pair<std::string, int>> getRenderableTagsList() const {
		return tagListItemModel_->renderableTags();
	}

	std::vector<std::string> getTagsList() const {
		return tagListItemModel_->tags<std::vector<std::string>>();
	}
};

class TagContainerEditorTest : public EditorTestFixture {
public:
	TagContainerEditorTest() : EditorTestFixture() {
		// RenderLayer objects
		const auto renderLayer_1 = create<RenderLayer>("RenderLayer_1");
		const auto renderLayer_2 = create<RenderLayer>("RenderLayer_2");
		const auto renderLayer_3 = create<RenderLayer>("RenderLayer_3");
		const auto renderLayer_4 = create<RenderLayer>("RenderLayer_4");
		const auto renderLayer_5 = create<RenderLayer>("RenderLayer_5");

		// Renderable tags
		const std::vector<std::pair<std::string, int>> renderable_tags_1{{"render_main", 2}, {"rt_1", 1}, {"rt_2", 0}};
		const std::vector<std::pair<std::string, int>> renderable_tags_2{{"rt_2", 0}, {"rt_3", 1}};
		const std::vector<std::pair<std::string, int>> renderable_tags_3{{"rt_2", 1}, {"rt_3", 0}, {"rt_4", 2}};
		const std::vector<std::pair<std::string, int>> renderable_tags_4{{"rt_2", 1}, {"rt_3", 2}};
		const std::vector<std::pair<std::string, int>> renderable_tags_5{};

		renderableTagsHandle_1 = {renderLayer_1, {"renderableTags"}};
		renderableTagsHandle_2 = {renderLayer_2, {"renderableTags"}};
		renderableTagsHandle_3 = {renderLayer_3, {"renderableTags"}};
		renderableTagsHandle_4 = {renderLayer_4, {"renderableTags"}};
		renderableTagsHandle_5 = {renderLayer_5, {"renderableTags"}};

		commandInterface.setRenderableTags(renderableTagsHandle_1, renderable_tags_1);
		commandInterface.setRenderableTags(renderableTagsHandle_2, renderable_tags_2);
		commandInterface.setRenderableTags(renderableTagsHandle_3, renderable_tags_3);
		commandInterface.setRenderableTags(renderableTagsHandle_4, renderable_tags_4);
		commandInterface.setRenderableTags(renderableTagsHandle_5, renderable_tags_5);

		// Material tags
		std::vector<std::string> material_tags_1{"mt_1", "mt_2", "mt_3"};
		std::vector<std::string> material_tags_2{};
		std::vector<std::string> material_tags_3{"mt_1", "mt_2"};
		std::vector<std::string> material_tags_4{"mt_2"};
		std::vector<std::string> material_tags_5{"mt_1"};

		materialTagsHandle_1 = {renderLayer_1, {"materialFilterTags"}};
		materialTagsHandle_2 = {renderLayer_2, {"materialFilterTags"}};
		materialTagsHandle_3 = {renderLayer_3, {"materialFilterTags"}};
		materialTagsHandle_4 = {renderLayer_4, {"materialFilterTags"}};
		materialTagsHandle_5 = {renderLayer_5, {"materialFilterTags"}};

		commandInterface.setTags(materialTagsHandle_1, material_tags_1);
		commandInterface.setTags(materialTagsHandle_2, material_tags_2);
		commandInterface.setTags(materialTagsHandle_3, material_tags_3);
		commandInterface.setTags(materialTagsHandle_4, material_tags_4);
		commandInterface.setTags(materialTagsHandle_5, material_tags_5);
	}

	ValueHandle renderableTagsHandle_1;
	ValueHandle renderableTagsHandle_2;
	ValueHandle renderableTagsHandle_3;
	ValueHandle renderableTagsHandle_4;
	ValueHandle renderableTagsHandle_5;

	ValueHandle materialTagsHandle_1;
	ValueHandle materialTagsHandle_2;
	ValueHandle materialTagsHandle_3;
	ValueHandle materialTagsHandle_4;
	ValueHandle materialTagsHandle_5;

	static bool pairIsEqual(const std::pair<std::string, int> &lhs, const std::pair<std::string, int>& rhs) {
		return lhs.first == rhs.first && lhs.second == rhs.second;
	}
};

TEST_F(TagContainerEditorTest, no_common_renderable_tags) {
	const std::set handles{renderableTagsHandle_1, renderableTagsHandle_5};
	PropertyBrowserItem item{handles, dataChangeDispatcher, &commandInterface, nullptr};

	const auto tagEditor = ExposedTagContainerEditor(&item);
	EXPECT_EQ(tagEditor.getTagsList(), std::vector<std::string>({"Multiple values"}));
	
	QWidget anchor;
	const auto tagEditorPopup = ExposedTagContainerEditor_Popup(&item, TagType::NodeTags_Referencing, &anchor);
	const auto tags = tagEditorPopup.getRenderableTagsList();
	EXPECT_TRUE(tags.empty());
}

TEST_F(TagContainerEditorTest, one_common_renderable_tag_single_value)
{
	const std::set handles{renderableTagsHandle_1, renderableTagsHandle_2};
	PropertyBrowserItem item{handles, dataChangeDispatcher, &commandInterface, nullptr};
	const auto tagEditor = ExposedTagContainerEditor(&item);
	EXPECT_EQ(tagEditor.getTagsList(), std::vector<std::string>({"Multiple values", "rt_2"}));

	QWidget anchor;
	const auto tagEditorPopup = ExposedTagContainerEditor_Popup(&item, TagType::NodeTags_Referencing, &anchor);
	const auto tags = tagEditorPopup.getRenderableTagsList();
	EXPECT_EQ(tags.size(), 1);
	EXPECT_TRUE(TagContainerEditorTest::pairIsEqual(tags[0], {"rt_2", 0}));
}

TEST_F(TagContainerEditorTest, several_common_renderable_tags_multiple_values) {
	const std::set handles{renderableTagsHandle_2, renderableTagsHandle_3};
	PropertyBrowserItem item{handles, dataChangeDispatcher, &commandInterface, nullptr};
	const auto tagEditor = ExposedTagContainerEditor(&item);
	EXPECT_EQ(tagEditor.getTagsList(), std::vector<std::string>({"Multiple values", "rt_2", "rt_3"}));

	QWidget anchor;
	const auto tagEditorPopup = ExposedTagContainerEditor_Popup(&item, TagType::NodeTags_Referencing, &anchor);
	const auto tags = tagEditorPopup.getRenderableTagsList();
	EXPECT_EQ(tags.size(), 2);
	EXPECT_TRUE(TagContainerEditorTest::pairIsEqual(tags[0], {"rt_2", 0}));
	EXPECT_TRUE(TagContainerEditorTest::pairIsEqual(tags[1], {"rt_3", 0}));
}

TEST_F(TagContainerEditorTest, several_common_renderable_tags_mixed_values) {
	const std::set handles{renderableTagsHandle_3, renderableTagsHandle_4};
	PropertyBrowserItem item{handles, dataChangeDispatcher, &commandInterface, nullptr};
	const auto tagEditor = ExposedTagContainerEditor(&item);
	EXPECT_EQ(tagEditor.getTagsList(), std::vector<std::string>({"Multiple values", "rt_2", "rt_3"}));

	QWidget anchor;
	const auto tagEditorPopup = ExposedTagContainerEditor_Popup(&item, TagType::NodeTags_Referencing, &anchor);
	const auto tags = tagEditorPopup.getRenderableTagsList();
	EXPECT_EQ(tags.size(), 2);
	EXPECT_TRUE(TagContainerEditorTest::pairIsEqual(tags[0], {"rt_3", 0}));
	EXPECT_TRUE(TagContainerEditorTest::pairIsEqual(tags[1], {"rt_2", 1}));
}

TEST_F(TagContainerEditorTest, no_common_material_tags) {
	const std::set handles{materialTagsHandle_1, materialTagsHandle_2};
	PropertyBrowserItem item{handles, dataChangeDispatcher, &commandInterface, nullptr};
	const auto tagEditor = ExposedTagContainerEditor(&item);
	EXPECT_EQ(tagEditor.getTagsList(), std::vector<std::string>({"Multiple values"}));

	QWidget anchor;
	const auto tagEditorPopup = ExposedTagContainerEditor_Popup(&item, TagType::MaterialTags, &anchor);
	const auto tags = tagEditorPopup.getTagsList();
	EXPECT_TRUE(tags.empty());
}

TEST_F(TagContainerEditorTest, several_common_material_tags) {
	const std::set handles{materialTagsHandle_1, materialTagsHandle_3};
	PropertyBrowserItem item{handles, dataChangeDispatcher, &commandInterface, nullptr};
	const auto tagEditor = ExposedTagContainerEditor(&item);
	EXPECT_EQ(tagEditor.getTagsList(), std::vector<std::string>({"Multiple values", "mt_1", "mt_2"}));

	QWidget anchor;
	const auto tagEditorPopup = ExposedTagContainerEditor_Popup(&item, TagType::MaterialTags, &anchor);
	const auto tags = tagEditorPopup.getTagsList();
	EXPECT_EQ(tags.size(), 2);
	EXPECT_EQ(tags[0], "mt_1");
	EXPECT_EQ(tags[1], "mt_2");
}

TEST_F(TagContainerEditorTest, one_common_material_tag) {
	const std::set handles{materialTagsHandle_1, materialTagsHandle_3, materialTagsHandle_5};
	PropertyBrowserItem item{handles, dataChangeDispatcher, &commandInterface, nullptr};
	const auto tagEditor = ExposedTagContainerEditor(&item);
	EXPECT_EQ(tagEditor.getTagsList(), std::vector<std::string>({"Multiple values", "mt_1"}));

	QWidget anchor;
	const auto tagEditorPopup = ExposedTagContainerEditor_Popup(&item, TagType::MaterialTags, &anchor);
	const auto tags = tagEditorPopup.getTagsList();
	EXPECT_EQ(tags.size(), 1);
	EXPECT_EQ(tags[0], "mt_1");
}

}  // namespace raco::property_browser
