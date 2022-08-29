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

#include "common_widgets/LinkStartSearchView.h"
#include "common_widgets/RaCoClipboard.h"
#include "core/Project.h"
#include "core/Queries.h"
#include "property_browser/PropertyBrowserEditorPopup.h"
#include "property_browser/PropertyBrowserLayouts.h"
#include "property_browser/PropertyBrowserModel.h"
#include "style/Colors.h"
#include "style/Icons.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QLabel>
#include <QMenu>
#include <QMimeData>
#include <QSizeGrip>

namespace raco::property_browser {

using namespace raco::style;

class LinkEditorPopup : public PropertyBrowserEditorPopup {
public:
	using LinkState = raco::core::Queries::LinkState;
	using LinkStartSearchView = raco::common_widgets::LinkStartSearchView;

	LinkEditorPopup(PropertyBrowserItem* item, QWidget* anchor) : PropertyBrowserEditorPopup{item, anchor, new LinkStartSearchView(item->dispatcher(), item->project(), item->valueHandle(), anchor)} {
		bool isLinked{item->linkText().size() > 0};

		if (isLinked) {
			currentRelation_.setReadOnly(true);
			currentRelation_.setText(item->linkText().c_str());
			deleteButton_.setFlat(true);
			deleteButton_.setIcon(Icons::instance().remove);
		} else {
			currentRelation_.setVisible(false);
			deleteButton_.setVisible(false);
		}
	}

protected:
	void establishObjectRelation() override {
		auto allowedStrong = dynamic_cast<LinkStartSearchView*>(list_)->allowedStrong(list_->selection());
		auto allowedWeak = dynamic_cast<LinkStartSearchView*>(list_)->allowedWeak(list_->selection());
		item_->setLink(list_->handleFromIndex(list_->selection()), !allowedStrong && allowedWeak);
	}

	void removeObjectRelation() override {
		item_->removeLink();
	}
};

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

	setLinkState(item->linkState());

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

void LinkEditor::setLinkState(const LinkState& linkstate) {
	// update link editor visuals
	using CurrentLinkState = raco::core::Queries::CurrentLinkState;

	if (linkstate.current == CurrentLinkState::LINKED || linkstate.current == CurrentLinkState::BROKEN) {
		linkButton_->setToolTip(QString::fromStdString(item_->linkText(true)));
	} else {
		linkButton_->setToolTip(QString());
	}

	linkButton_->setVisible(true);
	linkButton_->setDisabled(linkstate.readonly || !linkstate.validLinkTarget);

	goToLinkButton_->setVisible(true);
	goToLinkButton_->setDisabled(true);
	goToLinkButton_->setIcon(Icons::instance().unlinkable);
	goToLinkButton_->setToolTip("");

	QObject::disconnect(goToLinkButtonConnection_);

	if (linkstate.current == CurrentLinkState::PARENT_LINKED) {
		linkButton_->setIcon(Icons::instance().parentIsLinked);
	} else {
		if (linkstate.validLinkTarget) {
			switch (linkstate.current) {
				case CurrentLinkState::NOT_LINKED: {
					linkButton_->setIcon(Icons::instance().linkable);
					break;
				}
				case CurrentLinkState::LINKED: {
					linkButton_->setIcon(Icons::instance().linked);
					break;
				}
				case CurrentLinkState::BROKEN: {
					linkButton_->setIcon(Icons::instance().linkBroken);
					break;
				}
			}
		} else {
			linkButton_->setIcon(Icons::instance().unlinkable);
		}
	}

	auto endingLink = raco::core::Queries::getLink(*item_->project(), item_->valueHandle().getDescriptor());
	auto startingLinks = raco::core::Queries::getLinksConnectedToProperty(*item_->project(), item_->valueHandle(), true, false);

	if (endingLink) {
		auto linkStartObj = *endingLink->startObject_;
		auto linkStartPropName = endingLink->descriptor().start.getFullPropertyPath();

		if (startingLinks.empty()) {
			goToLinkButton_->setDisabled(false);
			goToLinkButton_->setIcon(*endingLink->isWeak_ ? Icons::instance().singleArrowLeft : Icons::instance().doubleArrowLeft);
			goToLinkButton_->setToolTip(QString("Go to link start (%1)").arg(QString::fromStdString(linkStartPropName)));

			QObject::connect(goToLinkButton_, &QPushButton::clicked, [this, linkStartObj]() {
				item_->model()->Q_EMIT objectSelectionRequested(QString::fromStdString(linkStartObj->objectID()));
			});

		} else {
			goToLinkButton_->setDisabled(false);
			goToLinkButton_->setIcon(Icons::instance().goToLeftRight);
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
		goToLinkButton_->setIcon(Icons::instance().goTo);
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
}

void LinkEditor::addLinkEndpointMenuItems(const std::vector<raco::core::SLink>& startingLinks, QMenu* endsMenu, QString& requestedLinkEndObj) {
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

std::map<std::string, std::string> LinkEditor::generateSortedLinkPoints(const std::vector<raco::core::SLink> links) {
	std::map<std::string, std::string> sortedLinkEnds;
	for (const auto& linkEnd : links) {
		auto linkDesc = linkEnd->descriptor();
		sortedLinkEnds[linkDesc.end.getFullPropertyPath()] = linkDesc.end.object()->objectID();
	}

	return sortedLinkEnds;
}

void LinkEditor::dragEnterEvent(QDragEnterEvent* event) {
	if (event->mimeData()->hasFormat(MimeTypes::VALUE_HANDLE_PATH) && event->mimeData()->hasFormat(MimeTypes::EDITOR_OBJECT_ID)) {
		if (auto handle{core::Queries::findByIdAndPath(*item_->project(), event->mimeData()->data(MimeTypes::EDITOR_OBJECT_ID).toStdString(), event->mimeData()->data(MimeTypes::VALUE_HANDLE_PATH).toStdString())}) {
			if (core::Queries::userCanCreateLink(*item_->project(), handle, item_->valueHandle(), false)) {
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
