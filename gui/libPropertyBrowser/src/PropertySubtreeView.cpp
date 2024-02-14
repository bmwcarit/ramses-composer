/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/PropertySubtreeView.h"

#include "ErrorBox.h"
#include "core/CoreFormatter.h"
#include "property_browser/PropertyBrowserLayouts.h"
#include "property_browser/PropertyBrowserUtilities.h"
#include "property_browser/PropertyBrowserWidget.h"
#include "property_browser/Utilities.h"
#include "property_browser/WidgetFactory.h"
#include "property_browser/controls/ExpandButton.h"
#include "property_browser/editors/LinkEditor.h"
#include "property_browser/editors/PropertyEditor.h"
#include "user_types/LuaScript.h"

#include <QApplication>
#include <QFormLayout>
#include <QPainter>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QScrollArea>
#include <QTimer>

namespace raco::property_browser {

void PropertySubtreeView::registerLabelContextMenu(QWidget* labelWidget, PropertyBrowserItem* item) {
	labelWidget->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

	connect(labelWidget, &PropertyEditor::customContextMenuRequested, [this, labelWidget, item](const QPoint& p) {
		auto* menu = new QMenu(this);

		for (const auto& entry : item->contextMenuActions()) {
			auto action = menu->addAction(QString::fromStdString(entry.description), entry.action);
			action->setEnabled(entry.enabled);
		}

		menu->exec(labelWidget->mapToGlobal(p));
	});
}

PropertySubtreeView::PropertySubtreeView(core::SceneBackendInterface* sceneBackend, PropertyBrowserModel* model, PropertyBrowserItem* item, QWidget *parent,  PropertySubtreeView* parentSubtree)
	: QWidget{parent}, sceneBackend_{sceneBackend}, item_{item}, model_{model}, layout_{this}, parentSubtree_(parentSubtree) {
	// .PropertySubtreeView--------------------------------------------------------------.
	// | expand button |  label + margin       | link control | property / value control |
	// .PropertySubtreeChildrenContainer-------------------------------------------------.
	// |               | PropertySubtreeView                                             |
	// | button margin | PropertySubtreeView                                             |
	// |               | PropertySubtreeView                                             |
	// .---------------------------------------------------------------------------------.
	auto* labelContainer = new QWidget{this};
	auto* labelLayout = new PropertyBrowserHBoxLayout{labelContainer};
	labelLayout->setAlignment(Qt::AlignLeft);
	if (!item->isRootItem()) {
		// Note: the factory function might return a label that is not a QLabel but some other widget.
		// The code below is still expected to work with this.
		std::tie(label_, controlWidget_) = WidgetFactory::createPropertyWidgets(item, labelContainer);
		label_->setToolTip(QString::fromStdString(item->labelToolTipText()));
		registerLabelContextMenu(label_, item);

		if (item->expandable()) {
			decorationWidget_ = new ExpandButton(item, labelContainer);
		} else {
			// Easier to make a dummy spacer widget than figuring out how to move everything else at the right place without it
			decorationWidget_ = new QWidget(this);
			decorationWidget_->setFixedHeight(0);
		}
		decorationWidget_->setFixedWidth(28);

		labelLayout->addWidget(decorationWidget_, 0);
		labelLayout->addWidget(label_, 0);
		labelLayout->addWidget(controlWidget_, 0);
	} else {
		// Root items show only the children but no expand button, label, or link/property controls.
		// We setup dummy button & label widgets anyway since they are used in the layout calculation.
		decorationWidget_ = new QWidget{this};
		label_ = new QLabel{this};
		decorationWidget_->setFixedWidth(0);
		decorationWidget_->setFixedHeight(0);
		label_->setFixedWidth(0);
		label_->setFixedHeight(0);
		labelLayout->addWidget(decorationWidget_, 0);
		labelLayout->addWidget(label_, 0);
	}

	errorContainer_ = new QWidget(this);
	errorLayout_ = new PropertyBrowserVBoxLayout(errorContainer_);
	layout_.insertWidget(ErrorRow, errorContainer_);
	QObject::connect(item, &PropertyBrowserItem::displayErrorChanged, this, &PropertySubtreeView::updateErrors);
	updateErrors();

	layout_.insertWidget(LabelRow, labelContainer);

	// Since the horizontal layouting is done via label_->setFixedWidth we have to cache the minimum size
	// hint before calling setFixedWidth because the size hint will just be the fixed width afterwards.
	labelMinWidth_ = label_->minimumSizeHint().width();

	if (parentSubtree_) {
		// We need to initialize things here before performing the layout recalculation below
		parentSubtree_->childrenContainer_->addWidget(this);
		setLabelAreaWidth(parentSubtree_->label_->width());
	}
	updateLabelAreaWidth();

	if (item->expandable()) {
		// Events which can cause a build or destroy of the childrenContainer_
		QObject::connect(item, &PropertyBrowserItem::childrenChanged, this, &PropertySubtreeView::updateChildrenContainer);
		QObject::connect(item, &PropertyBrowserItem::showChildrenChanged, this, &PropertySubtreeView::updateChildrenContainer);
		QObject::connect(item, &PropertyBrowserItem::childrenChangedOrCollapsedChildChanged, [this]() {
			playHighlightAnimation(250, 0.0f, 1.0f);
		});
		updateChildrenContainer();
	}

	layout_.addStretch(1);
}

void PropertySubtreeView::updateErrors() {
	while (errorLayout_->count()) {
		errorLayout_->removeItem(errorLayout_->itemAt(0));
	}
	const auto errorItems = item_->getDisplayErrorItems();
	if (!errorItems.empty()) {
		errorContainer_->show();
		for (const auto& item : errorItems) {
			errorLayout_->addWidget(new ErrorBox(item.message, item.level, this));
		}
		// TODO figure out if this is actually needed; it used to be
		update();
	} else {
		errorContainer_->hide();
	}
}

void PropertySubtreeView::collectTabWidgets(QObject* item, QWidgetList& tabWidgets) {
	if (const auto itemWidget = qobject_cast<QWidget*>(item)) {
		if (itemWidget->focusPolicy() & Qt::TabFocus) {
			tabWidgets.push_back(itemWidget);
		}
	}
	for (const auto child : item->children()) {
		collectTabWidgets(child, tabWidgets);
	}
}

void PropertySubtreeView::recalculateTabOrder() {
	QWidgetList tabWidgets;
	collectTabWidgets(this, tabWidgets);

	auto lastWidget = previousInFocusChain();
	for (const auto widget : tabWidgets) {
		setTabOrder(lastWidget, widget);
		lastWidget = widget;
	}
}

void PropertySubtreeView::updateChildrenContainer() {
	if (item_->showChildren()) {
		if (childrenContainer_) {
			childrenContainer_->deleteAllChildren();
		} else {
			childrenContainer_ = new PropertySubtreeChildrenContainer{item_, this};
			childrenContainer_->setOffset(decorationWidget_->width());
			layout_.insertWidget(ChildrenContainerRow, childrenContainer_);
		}

		for (const auto& child : item_->children()) {
			// The PropertySubtreeView will add itself as a child to the childrenContainer_ of its parent
			// and perform a relayouting afterwards
			auto subtree = new PropertySubtreeView{sceneBackend_, model_, child, childrenContainer_, this};

			QObject::connect(child, &PropertyBrowserItem::highlighted, [this, subtree]() {
				// We need to use a timer otherwise the scrolling does not work correctly
				QTimer::singleShot(1, this, [this, subtree]() {
					subtree->ensurePropertyVisible();
					subtree->playHighlightAnimation(2000, 1.0f, 0.0f);
				});
			});
		}
		recalculateTabOrder();
	} else {
		if (childrenContainer_) {
			delete childrenContainer_;
			childrenContainer_ = nullptr;

			updateLabelAreaWidth();
			recalculateTabOrder();
		}
	}
}

void PropertySubtreeView::ensurePropertyVisible() {
	if (const auto scrollArea = property_browser::findAncestor<QScrollArea>(this)) {
		scrollArea->ensureWidgetVisible(this);
	}
}

void PropertySubtreeView::setLabelAreaWidth(int labelAreaWidth) {
	const auto labelWidth = labelAreaWidth - decorationWidget_->width();
	if (labelAreaWidth_ != labelAreaWidth) {
		labelAreaWidth_ = labelAreaWidth;
		label_->setFixedWidth(labelWidth);

		if (childrenContainer_ != nullptr) {
			for (const auto& child : childrenContainer_->getChildSubtreeViews()) {
				child->setLabelAreaWidth(labelWidth);
			}
		}
	}
}

bool PropertySubtreeView::updateLabelAreaWidth(bool initialize) {
	int newWidth = getLabelAreaWidth();
	if (newWidth != subtreeMaxWidth_) {
		subtreeMaxWidth_ = newWidth;
		// If the subtree width changed we propagate upwards
		if (parentSubtree_) {
			bool status = parentSubtree_->updateLabelAreaWidth(false);
			if (!status && initialize) {
				setLabelAreaWidth(labelAreaWidth_);
			}
			return status;
		} else {
			setLabelAreaWidth(subtreeMaxWidth_);
			return true;
		}
	}
	return false;
}

int PropertySubtreeView::getLabelAreaWidth() const {
	int labelWidthHint = labelMinWidth_;
	if (childrenContainer_ != nullptr) {
		for (const auto& child : childrenContainer_->getChildSubtreeViews()) {
			labelWidthHint = std::max(labelWidthHint, child->subtreeMaxWidth_);
		}
	}
	return labelWidthHint + decorationWidget_->width();
}

void PropertySubtreeView::drawHighlight(float intensity) {
	if (intensity > 0) {
		QPainter painter{this};
		QPen pen = painter.pen();
		pen.setStyle(Qt::PenStyle::NoPen);
		painter.setPen(pen);
		
		QColor highlightColor = qApp->palette().highlight().color();
		highlightColor.setAlphaF(intensity);
		QBrush highlightBrush = qApp->palette().highlight();
		highlightBrush.setColor(highlightColor);
		
		painter.setBrush(highlightBrush);
		painter.drawRect(rect());
	}
}

void PropertySubtreeView::paintEvent(QPaintEvent* event) {
	drawHighlight(highlight_);
	QWidget::paintEvent(event);
}

void PropertySubtreeView::playHighlightAnimation(int duration, float start, float end) {
	if (!item_->hasCollapsedParent()) {
		if (!visibleRegion().isEmpty()) {
			auto* animation = new QPropertyAnimation(this, "highlight");
			animation->setDuration(duration);
			animation->setStartValue(start);
			animation->setEndValue(end);
			QObject::connect(animation, &QPropertyAnimation::destroyed, this, [this]() {
				highlight_ = 0.0;
				update();
			});
			animation->start(QPropertyAnimation::DeleteWhenStopped);
		}
	}
}

}  // namespace raco::property_browser
