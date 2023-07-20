/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/editors/TagContainerEditor.h"
#include "TagContainerEditor/TagContainerEditor_Popup.h"
#include "core/TagDataCache.h"
#include "core/Queries_Tags.h"

#include "property_browser/PropertyBrowserItem.h"
#include "property_browser/PropertyBrowserLayouts.h"

#include "style/Icons.h"

#include "components/EditorObjectFormatter.h"

#include "user_types/Enumerations.h"
#include "user_types/Material.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include "user_types/PrefabInstance.h"
#include "user_types/RenderLayer.h"
#include "user_types/RenderPass.h"

#include <QListWidget>

namespace raco::property_browser {

TagContainerEditor::TagContainerEditor(PropertyBrowserItem* item, QWidget* parent) : PropertyEditor(item, parent) {
	auto* layout{new PropertyBrowserGridLayout{this}};

	tagType_ = core::Queries::getHandleTagType(*item->valueHandles().begin());

	layout->setColumnStretch(0, 0);
	layout->setColumnStretch(1, 1);

	editButton_ = new QPushButton{this};
	editButton_->setFlat(true);
	editButton_->setProperty("slimButton", true);
	editButton_->setIcon(style::Icons::instance().openInNew);

	layout->addWidget(editButton_, 0, 0, Qt::AlignTop);

	tagList_ = new QListWidget{this};
	tagList_->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
	tagList_->setSelectionMode(QAbstractItemView::NoSelection);

	if (item_->valueHandles().size() > 1) {
		editButton_->setDisabled(true);
		tagList_->setDisabled(true);
	}

	layout->addWidget(tagList_, 0, 1, Qt::AlignTop);

	if (showRenderedBy()) {
		renderLayerSubscriptions_ = {
			item->dispatcher()->registerOnObjectsLifeCycle([this](core::SEditorObject obj) { Q_EMIT tagCacheDirty(); }, [this](core::SEditorObject obj) { Q_EMIT tagCacheDirty(); }),
			item->dispatcher()->registerOnPropertyChange("children", [this](core::ValueHandle handle) { Q_EMIT tagCacheDirty(); }),
			item->dispatcher()->registerOnPropertyChange("tags", [this](core::ValueHandle handle) { Q_EMIT tagCacheDirty(); }),
			item->dispatcher()->registerOnPropertyChange("layer0", [this](core::ValueHandle handle) { Q_EMIT tagCacheDirty(); }),
			item->dispatcher()->registerOnPropertyChange("layer1", [this](core::ValueHandle handle) { Q_EMIT tagCacheDirty(); }),
			item->dispatcher()->registerOnPropertyChange("layer2", [this](core::ValueHandle handle) { Q_EMIT tagCacheDirty(); }),
			item->dispatcher()->registerOnPropertyChange("layer3", [this](core::ValueHandle handle) { Q_EMIT tagCacheDirty(); }),
			item->dispatcher()->registerOnPropertyChange("layer4", [this](core::ValueHandle handle) { Q_EMIT tagCacheDirty(); }),
			item->dispatcher()->registerOnPropertyChange("layer5", [this](core::ValueHandle handle) { Q_EMIT tagCacheDirty(); }),
			item->dispatcher()->registerOnPropertyChange("layer6", [this](core::ValueHandle handle) { Q_EMIT tagCacheDirty(); }),
			item->dispatcher()->registerOnPropertyChange("layer7", [this](core::ValueHandle handle) { Q_EMIT tagCacheDirty(); }),
			item->dispatcher()->registerOnPropertyChange("materialFilterMode", [this](core::ValueHandle handle) { Q_EMIT tagCacheDirty(); }),
			item->dispatcher()->registerOnPropertyChange("materialFilterTags", [this](core::ValueHandle handle) { Q_EMIT tagCacheDirty(); }),
			item->dispatcher()->registerOnPropertyChange("renderableTags", [this](core::ValueHandle handle) { Q_EMIT tagCacheDirty(); })};
		std::for_each(item->valueHandles().begin(), item->valueHandles().end(), [&item, this](const core::ValueHandle& handle) {
			if (auto meshNode = handle.rootObject()->as<user_types::MeshNode>(); meshNode != nullptr) {
				materialPropertySubscriptions_.emplace_back(item->dispatcher()->registerOn(core::ValueHandle{meshNode, &user_types::MeshNode::materials_}, [this]() { Q_EMIT tagCacheDirty(); }));
			}
		});
		// Pool all refreshs in one frame into one refresh.
		QObject::connect(this, &TagContainerEditor::tagCacheDirty, this, &TagContainerEditor::updateTextAndButton, Qt::QueuedConnection);

		renderedBy_ = new QLabel{this};
		renderedBy_->setForegroundRole(QPalette::BrightText);
		layout->addWidget(renderedBy_, 1, 1, Qt::AlignTop);
	}
	
	updateTags();

	QObject::connect(editButton_, &QPushButton::clicked, this, &TagContainerEditor::editButtonClicked);
	QObject::connect(tagList_, &QListWidget::activated, this, &TagContainerEditor::editButtonClicked);
	QObject::connect(item, &PropertyBrowserItem::childrenChanged, this, &TagContainerEditor::updateTags);
	if (auto* sortOrderItem = item->siblingItem("sortOrder"); sortOrderItem != nullptr) {
		QObject::connect(sortOrderItem, &PropertyBrowserItem::valueChanged, this, &TagContainerEditor::updateTags);
	}
	QObject::connect(item, &PropertyBrowserItem::widgetRequestFocus, this, [this]() {
		editButton_->setFocus();
	});
}

void TagContainerEditor::editButtonClicked() const {
	new TagContainerEditor_Popup{item_, tagType_, editButton_};
}

void TagContainerEditor::updateTags() const {
	tagList_->clear();

	// Rearrange tags from all value handles into the map which has tag name as a key
	std::map<std::string, std::set<core::ValueHandle>> tagHandlesMap;
	std::for_each(item_->valueHandles().begin(), item_->valueHandles().end(), [this, &tagHandlesMap](const core::ValueHandle& handle) {
		for (size_t index{0}; index < handle.size(); index++) {
			std::string tag;
			if (tagType_ == core::TagType::NodeTags_Referencing) {
				tag = handle[index].getPropName();
			} else {
				tag = handle[index].asString();
			}

			if (tagHandlesMap.find(tag) == tagHandlesMap.end()) {
				tagHandlesMap[tag] = {};
			}
			tagHandlesMap[tag].insert(handle);
		}
	});

	bool multipleValues = false;
	std::vector<std::string> commonTags;
	std::for_each(tagHandlesMap.begin(), tagHandlesMap.end(), [this, &multipleValues, &commonTags](const std::pair<std::string, std::set<core::ValueHandle>>& tagHandlesPair) {
		if (tagHandlesPair.second.size() == item_->valueHandles().size()) { // if all handles have this tag
			commonTags.emplace_back(tagHandlesPair.first);
		} else if (!tagHandlesPair.second.empty()) {
			multipleValues = true;
		}
	});

	if (multipleValues) {
		tagList_->addItem("Multiple values");
	}

	std::for_each(commonTags.begin(), commonTags.end(), [this](const std::string& commonTag) {
		tagList_->addItem(QString::fromStdString(commonTag));
	});

	if (tagList_->count() == 0) {
		// we need a dummy item to make the double click work on empty lists.
		tagList_->addItem("");
	}
	tagList_->setFixedHeight(std::max(tagList_->fontMetrics().height(), tagList_->sizeHintForRow(0) * tagList_->count()) + 2 * tagList_->frameWidth());

	updateTextAndButton();
}

void TagContainerEditor::updateTextAndButton() const {
	if (!showRenderedBy()) {
		return;
	}

	std::set<std::shared_ptr<user_types::RenderPass>> renderedBy;
	std::set<std::shared_ptr<user_types::RenderLayer>> addedTo;
	bool isMultipleRenderedBy = false;
	bool isMultipleAddedTo = false;
	item_->getTagsInfo(renderedBy, isMultipleRenderedBy, addedTo, isMultipleAddedTo);
	std::string renderedByText = "Rendered by: <none>\n";
	if (isMultipleRenderedBy) {
		renderedByText = "Rendered by: Multiple values\n";
	}
	else if (!renderedBy.empty()) {
		renderedByText = fmt::format("Rendered by: {}\n", renderedBy);
	}

	if (isMultipleAddedTo) {
		renderedByText += "Added to: Multiple values";
	}
	else if (!addedTo.empty()) {
		renderedByText += fmt::format("Added to: {}", addedTo);
	} else {
		renderedByText += "Added to: <none>";
	}
	renderedBy_->setText(QString::fromStdString(renderedByText));
}

bool TagContainerEditor::showRenderedBy() const {
	return std::all_of(item_->valueHandles().begin(), item_->valueHandles().end(), [](const core::ValueHandle& handle) {
		return core::Queries::isUserTypeInTypeList(handle.rootObject(), core::Queries::UserTypesWithRenderableTags()) && core::Queries::isTagProperty(handle);
	});
}

}  // namespace raco::property_browser
