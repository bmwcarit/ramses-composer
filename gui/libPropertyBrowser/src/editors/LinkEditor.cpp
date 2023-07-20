/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/editors/LinkEditor.h"

#include "common_widgets/RaCoClipboard.h"
#include "core/Project.h"
#include "core/Queries.h"
#include "property_browser/editors/LinkEditorPopup.h"
#include "property_browser/PropertyBrowserEditorPopup.h"
#include "property_browser/PropertyBrowserLayouts.h"
#include "property_browser/PropertyBrowserModel.h"
#include "property_browser/PropertyBrowserUtilities.h"
#include "style/Colors.h"
#include "style/Icons.h"

#include <QApplication>
#include <QDropEvent>
#include <QMenu>
#include <QMimeData>

namespace raco::property_browser {

using namespace style;

LinkEditor::LinkEditor(
	PropertyBrowserItem* item,
	QWidget* parent) : QWidget{parent}, item_{item} {
	setAcceptDrops(true);
	layout_ = new PropertyBrowserGridLayout{this};
	layout_->setColumnStretch(0, 0);
	layout_->setColumnStretch(1, 0);
	layout_->setColumnStretch(2, 1);

	linkButton_ = new QPushButton{this};
	linkButton_->setFlat(true);
	linkButton_->setProperty("slimButton", true);

	goToLinkButton_ = new QPushButton{this};
	goToLinkButton_->setFlat(true);
	goToLinkButton_->setProperty("slimButton", true);

	layout_->addWidget(goToLinkButton_, 0, 0, Qt::AlignLeft);
	layout_->addWidget(linkButton_, 0, 1, Qt::AlignLeft);

	setLinkState();

	QObject::connect(item_, &PropertyBrowserItem::linkStateChanged, this, &LinkEditor::setLinkState);
	QObject::connect(linkButton_, &QPushButton::clicked, this, &LinkEditor::linkButtonClicked);
}

void LinkEditor::setControl(QWidget* widget) {
	layout_->addWidget(widget, 0, 2);
}

void LinkEditor::setExpanded(bool expanded) {
	layout_->setColumnStretch(1, expanded ? 0 : 1);
}

void LinkEditor::linkButtonClicked() {
	new LinkEditorPopup{item_, linkButton_};
}

void LinkEditor::setLinkState() {
	// update link editor visuals
	using CurrentLinkState = core::Queries::CurrentLinkState;

	std::vector<LinkState> linkStates;
	for (auto handle : item_->valueHandles()) {
		linkStates.emplace_back(core::Queries::linkState(*item_->commandInterface()->project(), handle));
	}

	// link button is disabled when it is disabled for at least one property:
	linkButton_->setDisabled(std::any_of(linkStates.begin(), linkStates.end(), [](auto linkState) {
		return linkState.readonly || !linkState.validLinkTarget;
	}));

	const auto linkText = item_->linkText(true);
	if (linkText.has_value()) {
		linkButton_->setToolTip(QString::fromStdString(linkText.value()));
	} else {
		linkButton_->setToolTip(PropertyBrowserItem::MultipleValueText);
	}

	// link button shows
	// - parent linked when at least one linkstate is parent linked
	// - unlinkable when at least one linkstate is unlinkable
	// - multivalue when
	//   - current link states not all equal or
	//   - current link states are all equal and are linked/parent_linked/broken but with different links
	// - linked when
	//   all linked with identical link start
	// - broken when
	//   all linked with identical link start
	// - linkable otherwise
	if (std::any_of(linkStates.begin(), linkStates.end(), [](auto linkState) {
			return linkState.current == CurrentLinkState::PARENT_LINKED;
		})) {
		linkButtonIcon_ = LinkIcon::parentIsLinked;
	} else {
		if (std::any_of(linkStates.begin(), linkStates.end(), [](auto linkState) {
				return !linkState.validLinkTarget;
			})) {
			linkButtonIcon_ = LinkIcon::unlinkable;
		} else {
			auto currentState = linkStates.begin()->current;
			bool singleCurrentState = std::all_of(linkStates.begin(), linkStates.end(), [currentState](auto linkState) {
				return linkState.current == currentState;
			});
			if (singleCurrentState && linkText.has_value()) {
				switch (currentState) {
					case CurrentLinkState::NOT_LINKED:
						linkButtonIcon_ = LinkIcon::linkable;
						break;
					case CurrentLinkState::LINKED:
						linkButtonIcon_ = LinkIcon::linked;
						break;
					case CurrentLinkState::BROKEN:
						linkButtonIcon_ = LinkIcon::linkBroken;
						break;
				}
			} else {
				linkButtonIcon_ = LinkIcon::linkMultipleValues;
			}
		}
	}

	goToLinkButton_->setDisabled(true);
	goToLinkButtonIcon_ = LinkIcon::unlinkable;
	goToLinkButton_->setToolTip("");

	QObject::disconnect(goToLinkButtonConnection_);

	// Ending link
	auto endingLink = core::Queries::getLink(*item_->project(), item_->valueHandles().begin()->getDescriptor());
	if (endingLink) {
		if (!std::all_of(++item_->valueHandles().begin(), item_->valueHandles().end(), [this, &endingLink](const core::ValueHandle& handle) {
				const auto link = core::Queries::getLink(*item_->project(), handle.getDescriptor());
				return link && link->startObject_ == endingLink->startObject_ &&
					   link->startProp_ == endingLink->startProp_ &&
					   link->isWeak_ == endingLink->isWeak_;
			})) {
			endingLink = nullptr;
		}
	}

	auto startingLinks = map_reduce<std::vector<core::SLink>>(
		item_->valueHandles(),
		set_union<std::vector<core::SLink>>,
		[this](auto handle) {
			return sorted(core::Queries::getLinksConnectedToProperty(*item_->project(), handle, true, false));
		});

	if (endingLink) {
		auto linkStartObj = *endingLink->startObject_;
		auto linkStartPropName = endingLink->descriptor().start.getFullPropertyPath();

		if (startingLinks.empty()) {
			goToLinkButton_->setDisabled(false);
			goToLinkButtonIcon_ = *endingLink->isWeak_ ? LinkIcon::singleArrowLeft : LinkIcon::doubleArrowLeft;
			goToLinkButton_->setToolTip(QString("Go to link start (%1)").arg(QString::fromStdString(linkStartPropName)));

			QObject::connect(goToLinkButton_, &QPushButton::clicked, [this, linkStartObj]() {
				item_->model()->Q_EMIT objectSelectionRequested(QString::fromStdString(linkStartObj->objectID()));
			});

		} else {
			goToLinkButton_->setDisabled(false);
			goToLinkButtonIcon_ = LinkIcon::goToLeftRight;
			goToLinkButton_->setToolTip("Go to...");

			goToLinkButtonConnection_ = QObject::connect(goToLinkButton_, &QPushButton::clicked, [this, linkStartObj, linkStartPropName, startingLinks]() {
				auto* linkMenu = new QMenu(this);
				QString requestedLinkEndObj;

				auto startAction = linkMenu->addAction(QString("Link start (%1)").arg(QString::fromStdString(linkStartPropName)), [this, linkStartObj, &requestedLinkEndObj]() {
					requestedLinkEndObj = QString::fromStdString(linkStartObj->objectID());
				});

				auto endsMenu = linkMenu->addMenu("Link ends...");

				addLinkEndpointMenuItems(startingLinks, endsMenu, requestedLinkEndObj);

				linkMenu->exec(mapToGlobal(goToLinkButton_->pos() + QPoint(goToLinkButton_->width(), 0)));
				if (!requestedLinkEndObj.isEmpty()) {
					item_->model()->Q_EMIT objectSelectionRequested(requestedLinkEndObj);
				}
			});
		}
	} else if (!startingLinks.empty()) {
		goToLinkButton_->setDisabled(false);
		goToLinkButtonIcon_ = LinkIcon::goTo;
		goToLinkButton_->setToolTip("Go to link ends...");

		goToLinkButtonConnection_ = QObject::connect(goToLinkButton_, &QPushButton::clicked, [this, startingLinks]() {
			auto* linkEndMenu = new QMenu(this);
			QString requestedLinkEndObj;

			addLinkEndpointMenuItems(startingLinks, linkEndMenu, requestedLinkEndObj);

			linkEndMenu->exec(mapToGlobal(goToLinkButton_->pos() + QPoint(goToLinkButton_->width(), 0)));
			if (!requestedLinkEndObj.isEmpty()) {
				item_->model()->Q_EMIT objectSelectionRequested(requestedLinkEndObj);
			}
		});
	}

	linkButton_->setIcon(LinkStateIcons_[linkButtonIcon_]);
	goToLinkButton_->setIcon(LinkStateIcons_[goToLinkButtonIcon_]);
}

void LinkEditor::addLinkEndpointMenuItems(const std::vector<core::SLink>& startingLinks, QMenu* endsMenu, QString& requestedLinkEndObj) {
	auto sortedLinkEnds = generateSortedLinkPoints(startingLinks);

	for (const auto& linkEnd : sortedLinkEnds) {
		auto linkEndPath = linkEnd.first;
		auto linkEndObjID = linkEnd.second;
		endsMenu->addAction(QString::fromStdString(linkEndPath),
			[this, linkEndObjID, &requestedLinkEndObj]() {
				requestedLinkEndObj = QString::fromStdString(linkEndObjID);
			});
	}
}

std::map<std::string, std::string> LinkEditor::generateSortedLinkPoints(const std::vector<core::SLink> links) {
	std::map<std::string, std::string> sortedLinkEnds;
	for (const auto& link : links) {
		auto linkDesc = link->descriptor();
		auto actionText = fmt::format("{} -> {}", (*link->startObject_)->objectName(), linkDesc.end.getFullPropertyPath());
		sortedLinkEnds[actionText] = linkDesc.end.object()->objectID();
	}

	return sortedLinkEnds;
}

void LinkEditor::dragEnterEvent(QDragEnterEvent* event) {
	if (event->mimeData()->hasFormat(MimeTypes::VALUE_HANDLE_PATH) && event->mimeData()->hasFormat(MimeTypes::EDITOR_OBJECT_ID)) {
		if (auto handle{core::Queries::findByIdAndPath(*item_->project(), event->mimeData()->data(MimeTypes::EDITOR_OBJECT_ID).toStdString(), event->mimeData()->data(MimeTypes::VALUE_HANDLE_PATH).toStdString())}) {
			if (std::all_of(item_->valueHandles().begin(), item_->valueHandles().end(), [this](const core::ValueHandle& handle) {
					return core::Queries::userCanCreateLink(*item_->project(), handle, handle, false);
				})) {
				event->acceptProposedAction();
				validDropTarget_ = true;
			}
		}
	}
}

void LinkEditor::dragLeaveEvent(QDragLeaveEvent* event) {
	validDropTarget_ = false;
}

void LinkEditor::dropEvent(QDropEvent* event) {
	if (auto handle{core::Queries::findByIdAndPath(*item_->project(), event->mimeData()->data(MimeTypes::EDITOR_OBJECT_ID).toStdString(), event->mimeData()->data(MimeTypes::VALUE_HANDLE_PATH).toStdString())}) {
		item_->setLink(handle, false);
	}
	validDropTarget_ = false;
}

}  // namespace raco::property_browser
