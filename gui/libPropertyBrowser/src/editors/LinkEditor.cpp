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

class LinkEditorPopup : public QDialog {
public:
	using LinkState = raco::core::Queries::LinkState;
	using LinkStartSearchView = raco::common_widgets::LinkStartSearchView;

	explicit LinkEditorPopup(PropertyBrowserItem* item, QWidget* anchor) : QDialog{anchor}, item_{item}, list_ {item_->dispatcher(), item_->project(), item->valueHandle(), this} {
		setWindowFlags(Qt::Popup);
		setAttribute(Qt::WA_DeleteOnClose);
		setSizeGripEnabled(true);

		bool isLinked{item->linkText().size() > 0};

		if (isLinked) {
			currentLink_.setReadOnly(true);
			currentLink_.setText(item->linkText().c_str());
			deleteButton_.setFlat(true);
			deleteButton_.setIcon(Icons::instance().remove);
		} else {
			currentLink_.setVisible(false);
			deleteButton_.setVisible(false);
		}

		acceptButton_.setFlat(true);
		acceptButton_.setIcon(Icons::instance().done);
		closeButton_.setFlat(true);
		closeButton_.setIcon(Icons::instance().close);

		frame_.setLineWidth(1);
		frame_.setFrameStyle(QFrame::Panel | QFrame::Raised);

		dataTypeLabel_.setAlignment(Qt::AlignCenter);
		dataTypeLabel_.setText(QString::fromStdString(item->luaTypeName()));

		outerLayout_.setContentsMargins(0, 0, 0, 0);
		outerLayout_.addWidget(&frame_, 0, 0, 1, 1);
		layout_.addWidget(&currentLink_, 0, 0, 1, 2);
		layout_.addWidget(&deleteButton_, 0, 2);
		layout_.addWidget(&search_, 1, 0, 1, 3);
		layout_.addWidget(&list_, 2, 0, 1, 3);
		layout_.addWidget(&acceptButton_, 3, 0);
		layout_.addWidget(&dataTypeLabel_, 3, 1);
		layout_.addWidget(&closeButton_, 3, 2);
		layout_.setColumnStretch(1, 1);

		search_.installEventFilter(this);
		search_.setFocus();
		acceptButton_.setEnabled(list_.hasValidSelection());

		QObject::connect(dynamic_cast<QApplication*>(QApplication::instance()), &QApplication::focusChanged, this, [this](QWidget* old, QWidget* now) { if (now != this && !this->isAncestorOf(now)) close(); });
		QObject::connect(&search_, &QLineEdit::textChanged, &list_, &LinkStartSearchView::setFilterByName);
		QObject::connect(&closeButton_, &QPushButton::clicked, this, &QWidget::close);
		QObject::connect(&deleteButton_, &QPushButton::clicked, this, [this, item]() { item->removeLink(); close(); });
		QObject::connect(&list_, &LinkStartSearchView::selectionChanged, &acceptButton_, &QPushButton::setEnabled);
		QObject::connect(&acceptButton_, &QPushButton::clicked, this, &LinkEditorPopup::createLink);
		QObject::connect(&list_, &LinkStartSearchView::clicked, this, &LinkEditorPopup::createLink);
		QObject::connect(&list_, &LinkStartSearchView::activated, this, &LinkEditorPopup::createLink);

		// center horizontally on link button and keep on screen
		search_.setMinimumWidth(500);
		updateGeometry();
		QPoint parentLocation = anchor->mapToGlobal(anchor->geometry().topLeft());
		QSize size = sizeHint();
		QRect screen = QApplication::desktop()->screenGeometry(anchor);
		int x = std::min( parentLocation.x() - size.width() / 2, screen.x() + screen.width() - size.width());
		int y = std::min( parentLocation.y(), screen.height()-size.height());
		move( x, y );

		show();
	}

	void createLink() {
		item_->setLink(list_.handleFromIndex(list_.selection()), !list_.allowedStrong(list_.selection()) && list_.allowedWeak(list_.selection()));
		close();
	}

protected:
	bool eventFilter(QObject* obj, QEvent* event) {
		if (&search_ == obj) {
			if (event->type() == QEvent::KeyPress) {
				QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
				if (keyEvent->key() == Qt::Key_Down) {
					list_.setFocus();
					return true;
				}
			}
			return false;
		}
		return QDialog::eventFilter(obj, event);
	}

	void keyPressEvent(QKeyEvent* event) override {
		auto key{event->key()};
		if (key == Qt::Key_Escape) {
			close();
		} else if (key == Qt::Key_Enter || key == Qt::Key_Return) {
			if (list_.hasValidSelection()) {
				createLink();
			} else if (search_.text().size() == 0 && item_->linkText().size() > 0) {
				item_->removeLink();
				close();
			} else {
				close();
			}
		} else if ((key == Qt::UpArrow || key == Qt::Key_Up)) {
			search_.setFocus();
		}
	}

private:
	PropertyBrowserItem* item_;
	QGridLayout outerLayout_{this};
	QFrame frame_{this};
	QGridLayout layout_{&frame_};
	QLineEdit currentLink_{this};
	QPushButton deleteButton_{this};
	QLineEdit search_{this};
	LinkStartSearchView list_;
	QPushButton acceptButton_{this};
	QPushButton closeButton_{this};
	QLabel dataTypeLabel_{this};
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
