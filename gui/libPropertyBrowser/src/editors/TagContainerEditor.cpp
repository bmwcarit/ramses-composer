/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/editors/TagContainerEditor.h"
#include "TagContainerEditor/TagContainerEditor_Popup.h"
#include "TagContainerEditor/TagContainerEditor_TagDataCache.h"

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

TagContainerEditor::TagContainerEditor(PropertyBrowserItem* item, QWidget* parent) : QWidget{parent}, item_{item} {
	auto* layout{new PropertyBrowserGridLayout{this}};

	if (&item_->valueHandle().rootObject()->getTypeDescription() == &user_types::Material::typeDescription) {
		tagType_ = TagType::MaterialTags;
	} else if (&item_->valueHandle().rootObject()->getTypeDescription() == &user_types::RenderLayer::typeDescription) {
		// TODO: what is a good way of checking if the ValueHandle points to a specific known C++ class member?
		// Would be nice to be able to write something like if(item->valueHandle().refersTo(&user_types::RenderLayer::renderableTags_))?
		// Checking the property name hides the dependency of this code to the property?
		// Or: make the TagType part of the TagContainerAnnotation? That would be easier and probably make more sense?
		if (item->valueHandle().getPropName() == "materialFilterTags") {
			tagType_ = TagType::MaterialTags;
		} else if (item->valueHandle().getPropName() == "renderableTags") {
			tagType_ = TagType::NodeTags_Referencing;
		} else {
			tagType_ = TagType::NodeTags_Referenced;
		}
	} else {
		tagType_ = TagType::NodeTags_Referenced;
	}

	layout->setColumnStretch(0, 0);
	layout->setColumnStretch(1, 1);

	editButton_ = new QPushButton{this};
	editButton_->setFlat(true);
	editButton_->setProperty("slimButton", true);
	editButton_->setIcon(style::Icons::icon(style::Pixmap::open_in_new));

	layout->addWidget(editButton_, 0, 0, Qt::AlignTop);

	tagList_ = new QListWidget{this};
	tagList_->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
	tagList_->setSelectionMode(QAbstractItemView::NoSelection);

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
			item->dispatcher()->registerOnPropertyChange("invertMaterialFilter", [this](core::ValueHandle handle) { Q_EMIT tagCacheDirty(); }),
			item->dispatcher()->registerOnPropertyChange("materialTags", [this](core::ValueHandle handle) { Q_EMIT tagCacheDirty(); }),
			item->dispatcher()->registerOnPropertyChange("renderableTags", [this](core::ValueHandle handle) { Q_EMIT tagCacheDirty(); })};
		if (auto meshNode = item->valueHandle().rootObject()->as<user_types::MeshNode>(); meshNode != nullptr) {
			materialPropertySubscription_ = item->dispatcher()->registerOn(core::ValueHandle{meshNode, &user_types::MeshNode::materials_}, [this]() { Q_EMIT tagCacheDirty(); });
		}
		// Pool all refreshs in one frame into one refresh.
		QObject::connect(this, &TagContainerEditor::tagCacheDirty, this, &TagContainerEditor::updateRenderedBy, Qt::QueuedConnection);

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
}

void TagContainerEditor::editButtonClicked() const {
	new TagContainerEditor_Popup{item_, tagType_, editButton_};
}

void TagContainerEditor::updateTags() const {
	tagList_->clear();
	if (tagType_ == TagType::NodeTags_Referencing) {
		const auto order = static_cast<user_types::ERenderLayerOrder>(item_->valueHandle().rootObject()->as<user_types::RenderLayer>()->sortOrder_.asInt());		
		std::map<int, QStringList> tagsSorted;
		for (size_t index{0}; index < item_->valueHandle().size(); index++) {
			auto tag = item_->valueHandle()[index].getPropName();
			auto orderIndex = order == user_types::ERenderLayerOrder::Manual ? item_->valueHandle()[index].asInt() : 0;
			tagsSorted[orderIndex].append(QString::fromStdString(tag));
		}
		for (auto const& p : tagsSorted) {
			tagList_->addItem(p.second.join(", "));
		}
	} else {
		tagList_->setSortingEnabled(true);
		for (size_t index{0}; index < item_->valueHandle().size(); index++) {
			auto tag = item_->valueHandle()[index].asString();
			tagList_->addItem(QString::fromStdString(tag));
		}
	}
	if (tagList_->count() == 0) {
		// we need a dummy item to make the double click work on empty lists.
		tagList_->addItem("");
	}
	tagList_->setFixedHeight(std::max(tagList_->fontMetrics().height(), tagList_->sizeHintForRow(0) * tagList_->count()) + 2 * tagList_->frameWidth());

	updateRenderedBy();
}

void TagContainerEditor::updateRenderedBy() const {
	if (!showRenderedBy()) {
		return;
	}
	const auto tagData = TagDataCache::createTagDataCache(item_->project(), TagType::NodeTags_Referencing);
	auto alltags = core::Queries::renderableTagsWithParentTags(item_->valueHandle().rootObject());
	const auto rps = tagData->allRenderPassesForObjectWithTags(item_->valueHandle().rootObject(), alltags);
	if (!rps.empty()) {
		renderedBy_->setText(QString::fromStdString(fmt::format("Rendered by: {}", rps)));
	} else {
		auto rls = tagData->allReferencingObjects<user_types::RenderLayer>(alltags);
		if (!rls.empty()) {
			renderedBy_->setText(QString::fromStdString(fmt::format("Rendered by: <none>\nAdded to: {}", rls)));
		} else {
			renderedBy_->setText(QString::fromStdString("Rendered by: <none>\nAdded to: <none>"));			
		}
	}
}

bool TagContainerEditor::showRenderedBy() const {
	return core::Queries::isUserTypeInTypeList(item_->valueHandle().rootObject(), core::Queries::UserTypesWithRenderableTags()) && core::Queries::isTagProperty(item_->valueHandle());
}

}  // namespace raco::property_browser
