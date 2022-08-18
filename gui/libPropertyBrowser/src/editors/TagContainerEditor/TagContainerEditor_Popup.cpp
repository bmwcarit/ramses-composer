/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "TagContainerEditor_Popup.h"
#include "core/Queries_Tags.h"
#include "core/TagDataCache.h"
#include "TagContainerEditor_AvailableTagsItemModel.h"
#include "TagContainerEditor_AppliedTagModel.h"
#include "TreeViewWithDel.h"

#include "components/EditorObjectFormatter.h"

#include "property_browser/PropertyBrowserItem.h"

#include "style/Icons.h"

#include "user_types/Enumerations.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include "user_types/PrefabInstance.h"
#include "user_types/RenderLayer.h"
#include "user_types/RenderPass.h"

#include <QApplication>
#include <QCompleter>
#include <QDesktopWidget>
#include <QGuiApplication>
#include <QHeaderView>
#include <QLineEdit>
#include <QStyledItemDelegate>
#include <QWindow>

namespace raco::property_browser {

	class TreeViewItemWithFullWidthEditor : public QStyledItemDelegate {
	public:
		TreeViewItemWithFullWidthEditor(QObject* parent, QCompleter* completer) : QStyledItemDelegate(parent), completer_(completer) {}

		QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
			auto editor = QStyledItemDelegate::createEditor(parent, option, index);
			// Also add auto-completion to the editor
			QLineEdit* lineEdit = dynamic_cast<QLineEdit*>(editor);
			if (index.column() == 0) {
				lineEdit->setCompleter(completer_);				
			}
			return editor;
		}
		
		void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
			// This delegate does nothing different from QStyledItemDelegate. Still the edit box looks slightly off with the default deleagte. Go figure.
			// Largely copied from QStyledItemDelegate::updateEditorGeometry.
			if (editor == nullptr) {
				return;
			}
			auto widget = option.widget;

			QStyleOptionViewItem opt = option;
			initStyleOption(&opt, index);
			opt.showDecorationSelected = true;

			QStyle* style = widget ? widget->style() : QApplication::style();
			const QRect geom = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, widget);
			editor->setGeometry(geom);
		}

		QCompleter* completer_;
	};

	TagContainerEditor_Popup::TagContainerEditor_Popup(PropertyBrowserItem* item, raco::core::TagType tagType, QWidget* anchor) : PopupDialog{anchor, QString::fromStdString(item->valueHandle().rootObject()->objectName()) + "." +  QString::fromStdString(item->displayName())}, tagType_{tagType}, item_{item} {
		listOfTags_ = std::make_unique<TreeViewWithDel>(this);
		availableTagsItemModel_ = new TagContainerEditor_AvailableTagsItemModel(&listOfAvailableTags_);
		tagListItemModel_ = new TagContainerEditor_AppliedTagModel(listOfTags_.get(), tagType_);
		
		acceptButton_.setFlat(true);
		acceptButton_.setText("Ok");
		closeButton_.setFlat(true);
		closeButton_.setText("Cancel");

		// Set up list of tags
		for (size_t index{ 0 }; index < item->valueHandle().size(); index++) {
			if (tagType_ == raco::core::TagType::NodeTags_Referencing) {
				tagListItemModel_->addTag(item->valueHandle()[index].getPropName(), -1,
					item->valueHandle()[index].asInt());
				
			} else {
				tagListItemModel_->addTag(item->valueHandle()[index].asString());				
			}
		}
		listOfTags_->setDragEnabled(true);
		listOfTags_->setAcceptDrops(true);
		listOfTags_->setDropIndicatorShown(tagType == raco::core::TagType::NodeTags_Referencing);
		listOfTags_->setDefaultDropAction(Qt::MoveAction);
		completer_.setModel(availableTagsItemModel_);
		listOfTags_->setItemDelegate(new TreeViewItemWithFullWidthEditor(listOfTags_.get(), &completer_));
		listOfTags_->setModel(tagListItemModel_);
		if (tagType_ == raco::core::TagType::NodeTags_Referencing) {
			tagListItemModel_->setHorizontalHeaderLabels({"Applied tags", "Render Order"});
			listOfTags_->header()->setStretchLastSection(false);
			listOfTags_->header()->setSectionResizeMode(0, QHeaderView::ResizeMode::Stretch);
			listOfTags_->header()->setSectionResizeMode(1, QHeaderView::ResizeMode::ResizeToContents);
			auto renderLayer = item->valueHandle().rootObject()->as<user_types::RenderLayer>();
			if (renderLayer && renderLayer->sortOrder_.asInt() != static_cast<int>(user_types::ERenderLayerOrder::Manual)) {
				listOfTags_->header()->hideSection(1);
			}
		} else {
			tagListItemModel_->setHorizontalHeaderLabels({"Applied tags"});
		}

		tagDataCache_ = raco::core::TagDataCache::createTagDataCache(item->project(), tagType);

		onTagListUpdated();
		
		// Set up list of available tags
		availableTagsItemModel_->setHorizontalHeaderLabels({ "Tag", "Render Layers" });
		listOfAvailableTags_.setDragEnabled(true);
		for (auto const& p : *tagDataCache_) {
			auto const tag = QString::fromStdString(p.first);
			QStringList listOfRenderLayerNames;
			for (auto const& r : p.second.referencingObjects_) {
				if (tagType_ == raco::core::TagType::MaterialTags) {
					auto renderLayer = std::dynamic_pointer_cast<user_types::RenderLayer>(r);
					listOfRenderLayerNames.push_back((*renderLayer->materialFilterMode_ == static_cast<int>(user_types::ERenderLayerMaterialFilterMode::Exclusive) ? "+" : "-") + QString::fromStdString(r->objectName()));
				}
				else {
					listOfRenderLayerNames.push_back(QString::fromStdString(r->objectName()));
				}
			}
			listOfRenderLayerNames.sort();
			availableTagsItemModel_->addTag(tag, listOfRenderLayerNames, forbiddenTags_.find(tag.toStdString()) == forbiddenTags_.end());
		}
		listOfAvailableTags_.setModel(availableTagsItemModel_);

		if (showRenderedBy()) {
			referencedBy_.setWordWrap(true);
			renderedBy_.setWordWrap(true);
			updateRenderedBy();			
		} else {
			referencedBy_.setFixedHeight(0);
			renderedBy_.setFixedHeight(0);
		}
		
		frame_.setLineWidth(1);
		frame_.setFrameStyle(QFrame::Panel | QFrame::Raised);

		outerLayout_.setContentsMargins(0, 0, 0, 0);
		outerLayout_.addWidget(&frame_, 0, 0, 1, 1);

		layout_.addWidget(&listOfAvailableTags_, 0, 0, 1, 2);
		layout_.addWidget(listOfTags_.get(), 0, 2, 1, 2);
		layout_.addWidget(&referencedBy_, 1, 0, 1, 4);
		layout_.addWidget(&renderedBy_, 2, 0, 1, 4);
		layout_.addWidget(&acceptButton_, 3, 0);
		layout_.addWidget(&closeButton_, 3, 3);
		layout_.setColumnStretch(1, 1);
		layout_.setColumnStretch(2, 1);

		QObject::connect(&listOfAvailableTags_, &QTreeView::doubleClicked, this, &TagContainerEditor_Popup::addTag);
		QObject::connect(listOfTags_.get(), &TreeViewWithDel::deletePressed, [this](const QModelIndex& index) {
			if (index.isValid() && index.row() >= 0 && index.row()<tagListItemModel_->rowCount() - 1) {
				tagListItemModel_->removeRow(index.row());				
			}
		});
		QObject::connect(tagListItemModel_, &TagContainerEditor_AppliedTagModel::tagListChanged, this, &TagContainerEditor_Popup::onTagListUpdated);

		QObject::connect(&closeButton_, &QPushButton::clicked, this, &TagContainerEditor_Popup::newTagListRejected);
		QObject::connect(&acceptButton_, &QPushButton::clicked, this, &TagContainerEditor_Popup::newTagListAccepted);

		showCenteredOnAnchor();
	}

	TagContainerEditor_Popup::~TagContainerEditor_Popup() {
	}

	void TagContainerEditor_Popup::newTagListAccepted() {
		if (tagType_ == raco::core::TagType::NodeTags_Referencing) {
			item_->setTags(tagListItemModel_->renderableTags());
		} else {
			item_->setTags(tagListItemModel_->tags());
		}
		accept();
	}

	void TagContainerEditor_Popup::newTagListRejected() {
		reject();
	}

	void TagContainerEditor_Popup::addTag(const QModelIndex& itemIndex) {
		auto const tag = availableTagsItemModel_->tagForIndex(itemIndex);
		if (!tag.empty() && forbiddenTags_.find(tag) == forbiddenTags_.end()) {
			tagListItemModel_->addTag(tag);			
		}
	}

	void TagContainerEditor_Popup::onTagListUpdated() {
		// Calculate the list of forbidden tags (tags which would create a loop in the render layer hierarchy)
		auto appliedTags = tagListItemModel_->tags();
		forbiddenTags_ = { appliedTags.begin(), appliedTags.end() };
		switch (tagType_) {
			case raco::core::TagType::MaterialTags: {
				break;
			}
			case raco::core::TagType::NodeTags_Referenced: {				
				raco::core::Queries::findForbiddenTags(*tagDataCache_, item_->valueHandle().rootObject(), forbiddenTags_);
				break;
			}
			case raco::core::TagType::NodeTags_Referencing: {
				raco::core::Queries::findRenderLayerForbiddenRenderableTags(*tagDataCache_, item_->valueHandle().rootObject()->as<user_types::RenderLayer>(), forbiddenTags_);
				break;
			}
		}
		tagListItemModel_->setForbiddenTags(forbiddenTags_);
		availableTagsItemModel_->setForbiddenTags(forbiddenTags_);

		updateRenderedBy();
	}

	void TagContainerEditor_Popup::updateRenderedBy() {
		if (!showRenderedBy()) {
			return;
		}
		std::set<std::string> alltags;
		auto node = item_->valueHandle().rootObject()->as<user_types::Node>();
		if (node != nullptr && node->getParent() != nullptr) {
			alltags = core::Queries::renderableTagsWithParentTags(node->getParent());
		}
		auto mytags = tagListItemModel_->tags<std::set<std::string>>();
		alltags.merge(mytags);		
		auto allReferencingRenderLayers = tagDataCache_->allReferencingObjects<user_types::RenderLayer>(alltags);
		const auto rps = tagDataCache_->allRenderPassesForObjectWithTags(item_->valueHandle().rootObject(), alltags);
		referencedBy_.setText(QString::fromStdString(fmt::format("Added to: {}", allReferencingRenderLayers)));
		renderedBy_.setText(QString::fromStdString(fmt::format("Rendered by: {}", rps)));
	}

	bool TagContainerEditor_Popup::showRenderedBy() const {
		return core::Queries::isUserTypeInTypeList(item_->valueHandle().rootObject(), core::Queries::UserTypesWithRenderableTags()) && core::Queries::isTagProperty(item_->valueHandle());
	}

}