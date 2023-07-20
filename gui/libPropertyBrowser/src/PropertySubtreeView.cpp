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
#include "property_browser/PropertyBrowserWidget.h"
#include "property_browser/WidgetFactory.h"
#include "property_browser/controls/ExpandButton.h"
#include "property_browser/editors/LinkEditor.h"
#include "property_browser/editors/PropertyEditor.h"
#include "user_types/LuaScript.h"
#include "user_types/LuaScriptModule.h"

#include <QApplication>
#include <QDebug>
#include <QFormLayout>
#include <QFrame>
#include <QPainter>
#include <QPropertyAnimation>
#include <QResizeEvent>

namespace raco::property_browser {

void drawHighlight(QWidget* widget, float intensity) {
	if (intensity > 0) {
		QPainter painter{widget};
		QPen pen = painter.pen();
		pen.setStyle(Qt::PenStyle::NoPen);
		painter.setPen(pen);
		painter.setBrush(qApp->palette().highlight());
		painter.drawRect(widget->rect());
	}
}

EmbeddedPropertyBrowserView::EmbeddedPropertyBrowserView(PropertyBrowserItem* item, QWidget* parent)
	: QFrame{parent} {
}

void PropertySubtreeView::registerCopyPasteContextMenu(QWidget* widget) {
	widget->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
	connect(widget, &PropertyEditor::customContextMenuRequested, [this, widget](const QPoint& p) {
		auto* treeViewMenu = new QMenu(this);
		// TODO shouldn't we disable this is we can't copy since we have a multiple value?
		// see PropertyEditor::copyValue
		auto copyAction = treeViewMenu->addAction("Copy", this, [this]() {
			propertyControl_->copyValue();
		});
		copyAction->setEnabled(propertyControl_->canCopyValue());
		// TODO disable when clipboard doesn't contain property !?
		auto pasteAction = treeViewMenu->addAction("Paste", this, [this]() {
			propertyControl_->pasteValue();
		});
		pasteAction->setEnabled(propertyControl_->canPasteValue());

		treeViewMenu->exec(widget->mapToGlobal(p));
	});
}

PropertySubtreeView::PropertySubtreeView(raco::core::SceneBackendInterface* sceneBackend, PropertyBrowserModel* model, PropertyBrowserItem* item, QWidget* parent)
	: QWidget{parent}, item_{item}, model_{model}, sceneBackend_{sceneBackend}, layout_{this} {
	layout_.setAlignment(Qt::AlignTop);

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
	if (item->isProperty()) {
		label_ = WidgetFactory::createPropertyLabel(item, labelContainer);
		label_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

		if (item->expandable()) {
			decorationWidget_ = new ExpandButton(item, labelContainer);
		} else {
			// Easier to make a dummy spacer widget than figuring out how to move everything else at the right place without it
			decorationWidget_ = new QWidget(this);
			decorationWidget_->setFixedHeight(0);
		}
		decorationWidget_->setFixedWidth(28);

		auto* linkControl = WidgetFactory::createLinkControl(item, labelContainer);

		propertyControl_ = WidgetFactory::createPropertyEditor(item, labelContainer);

		if (propertyControl_ != nullptr) {
			registerCopyPasteContextMenu(label_);
		}

		labelLayout->addWidget(decorationWidget_, 0);
		labelLayout->addWidget(label_, 0);
		labelLayout->addWidget(linkControl, 1);

		generateItemTooltip(item, true);

		linkControl->setControl(propertyControl_);
		label_->setEnabled(item->editable());
		QObject::connect(item, &PropertyBrowserItem::editableChanged, label_, &QWidget::setEnabled);
	} else {
		// Dummy label for the case that we are an object.
		decorationWidget_ = new QWidget{this};
		label_ = new QLabel{this};
		decorationWidget_->setFixedWidth(0);
		decorationWidget_->setFixedHeight(0);
		label_->setFixedWidth(0);
		label_->setFixedHeight(0);
		labelLayout->addWidget(decorationWidget_, 0);
		labelLayout->addWidget(label_, 0);
	}

	if (item->isObject() && item->valueHandles().size() > 1) {
		for (auto handle : item->valueHandles()) {
			auto nameHandle = handle.get("objectName");
			objectNameChangeSubscriptions_.push_back(item->dispatcher()->registerOn(nameHandle, [this]() {
				updateObjectNameDisplay();
			}));
		}
		updateObjectNameDisplay();
	}

	QObject::connect(item, &PropertyBrowserItem::errorChanged, this, &PropertySubtreeView::updateError);
	updateError();

	layout_.addWidget(labelContainer, 2, 0);

	if (item->expandable()) {
		// Events which can cause a build or destroy of the childrenContainer_
		QObject::connect(item, &PropertyBrowserItem::childrenChanged, this, &PropertySubtreeView::updateChildrenContainer);
		QObject::connect(item, &PropertyBrowserItem::showChildrenChanged, this, &PropertySubtreeView::updateChildrenContainer);
		QObject::connect(item, &PropertyBrowserItem::childrenChangedOrCollapsedChildChanged, this, &PropertySubtreeView::playStructureChangeAnimation);
		updateChildrenContainer();
	}
}


void PropertySubtreeView::generateItemTooltip(PropertyBrowserItem* item, bool connectWithChangeEvents) {
	auto labelToolTip = QString::fromStdString(item->getPropertyName());

	if (item->isLuaProperty()) {
		labelToolTip.append(" [" + QString::fromStdString(item->luaTypeName()) + "]");
	}
	label_->setToolTip(labelToolTip);

	if (item->valueHandles().begin()->isRefToProp(&core::EditorObject::objectName_)) {
		QString tooltipText;
		if (item->valueHandles().size() == 1) {
			tooltipText = QString::fromStdString(sceneBackend_->getExportedObjectNames(item->valueHandles().begin()->rootObject()));
		} else {
			QStringList items = objectNames();
			items.push_front("Current objects:");
			tooltipText = items.join("\n");
		}
		if (!tooltipText.isEmpty()) {
			propertyControl_->setToolTip(tooltipText);
		}

		if (connectWithChangeEvents) {
			connect(item, &PropertyBrowserItem::valueChanged, this, [this, item] { generateItemTooltip(item, false); });
		}
	}
}

QStringList PropertySubtreeView::objectNames() const {
	QStringList items;
	for (auto handle : item_->valueHandles()) {
		auto object = handle.rootObject();
		std::string labelText = fmt::format("{} [{}]", object->objectName(), object->getTypeDescription().typeName);
		items.push_back(QString::fromStdString(labelText));
	}
	items.sort();
	return items;
}

void PropertySubtreeView::updateObjectNameDisplay() {
	QStringList items = objectNames();
	items.push_front("Current objects:");
	QString description = items.join("\n");

	if (layout_.itemAtPosition(0, 0) && layout_.itemAtPosition(0, 0)->widget()) {
		auto* widget = layout_.itemAtPosition(0, 0)->widget();
		dynamic_cast<ErrorBox*>(widget)->updateContent(description);
	} else {
		layout_.addWidget(new ErrorBox(description, core::ErrorLevel::INFORMATION, this), 0, 0);
	}
}

void PropertySubtreeView::updateError() {
	if (layout_.itemAtPosition(1, 0) && layout_.itemAtPosition(1, 0)->widget()) {
		auto* widget = layout_.itemAtPosition(1, 0)->widget();
		layout_.removeWidget(widget);
		widget->hide();
		widget->deleteLater();
	}

	if (item_->hasError()) {
		std::string errorMsg;
		auto optCategory = item_->errorCategory();
		if (!optCategory.has_value()) {
			errorMsg = "Multiple Erorrs";
		} else {
			auto category = optCategory.value();
			if (category == core::ErrorCategory::RAMSES_LOGIC_RUNTIME || category == core::ErrorCategory::PARSING || category == core::ErrorCategory::GENERAL || category == core::ErrorCategory::MIGRATION) {
				errorMsg = item_->errorMessage().c_str();
			}
		}
		if (!errorMsg.empty()) {
			layout_.addWidget(new ErrorBox(QString::fromStdString(errorMsg), item_->maxErrorLevel(), this), 1, 0);
			// It is unclear why this is needed - but without it, the error box does not appear immediately when an incompatible render buffer is assigned to a render target and the scene error view is in the background.
			// The error box does appear later, e. g. when the mouse cursor is moved over the preview or when the left mouse button is clicked in a different widget.
			update();
		}
	}
}

void PropertySubtreeView::collectTabWidgets(QObject* item, QWidgetList& tabWidgets) {
	if (auto itemWidget = qobject_cast<QWidget*>(item)) {
		if (itemWidget->focusPolicy() & Qt::TabFocus) {
			tabWidgets.push_back(itemWidget);
		}
	}
	for (auto child : item->children()) {
		collectTabWidgets(child, tabWidgets);
	}
}


void PropertySubtreeView::recalculateTabOrder() {
	QWidgetList tabWidgets;
	collectTabWidgets(this, tabWidgets);

	auto lastWidget = previousInFocusChain();
	for (auto widget : tabWidgets) {
		setTabOrder(lastWidget, widget);
		lastWidget = widget;
	}
}

void PropertySubtreeView::updateChildrenContainer() {
	if (item_->showChildren() && !childrenContainer_) {
		if (item_->isProperty() && item_->type() == core::PrimitiveType::Ref) {
			childrenContainer_ = new PropertySubtreeChildrenContainer{item_, this};
			childrenContainer_->addWidget(new EmbeddedPropertyBrowserView{item_, this});
			layout_.addWidget(childrenContainer_, 3, 0);
		} else 
			if (item_->isObject() || hasTypeSubstructure(item_->type())) {
			childrenContainer_ = new PropertySubtreeChildrenContainer{item_, this};

			for (const auto& child : item_->children()) {
				auto* subtree = new PropertySubtreeView{sceneBackend_, model_, child, childrenContainer_};
				childrenContainer_->addWidget(subtree);
			}
			QObject::connect(item_, &PropertyBrowserItem::childrenChanged, childrenContainer_, [this](const QList<PropertyBrowserItem*> items) {
				Q_EMIT model_->beforeStructuralChange(this);
				for (auto& childWidget : childrenContainer_->findChildren<PropertySubtreeView*>(QString{}, Qt::FindDirectChildrenOnly)) {
					Q_EMIT model_->beforeRemoveWidget(childWidget);
					childrenContainer_->removeWidget(childWidget);
					childWidget->deleteLater();
				}
				for (auto& child : items) {
					auto* subtree = new PropertySubtreeView{sceneBackend_, model_, child, childrenContainer_};
					childrenContainer_->addWidget(subtree);
				}
				recalculateTabOrder();
			});
			recalculateLabelWidth();
			layout_.addWidget(childrenContainer_, 3, 0);
		}
	} else if (!item_->showChildren() && childrenContainer_) {
		delete childrenContainer_;
		childrenContainer_ = nullptr;
	}

	recalculateTabOrder();
}

void PropertySubtreeView::setLabelAreaWidth(int width) {
	if (labelWidth_ != width) {
		labelWidth_ = width;
		recalculateLabelWidth();
	}
}

void PropertySubtreeView::recalculateLabelWidth() { 
	if (childrenContainer_ != nullptr) {
		childrenContainer_->setOffset(decorationWidget_->width());
	}
	const int labelWidthHint = std::max(labelWidth_, getLabelAreaWidthHint() - decorationWidget_->width());
	if (label_->width() != labelWidthHint) {
		label_->setFixedWidth(labelWidthHint);
	}

	for (const auto& child : childrenContainer_->findChildren<PropertySubtreeView*>(QString{}, Qt::FindDirectChildrenOnly)) {
		child->setLabelAreaWidth(labelWidthHint - decorationWidget_->width());
	}
}

int PropertySubtreeView::getLabelAreaWidthHint() const {
	int labelWidthHint = label_->fontMetrics().boundingRect(label_->text()).width();
	if (childrenContainer_ != nullptr && childrenContainer_->size().height() > 0) {
		for (const auto& child : childrenContainer_->findChildren<PropertySubtreeView*>(QString{}, Qt::FindDirectChildrenOnly)) {
			const auto childLabelWidthHint = child->getLabelAreaWidthHint();
			if (labelWidthHint < childLabelWidthHint) {
				labelWidthHint = childLabelWidthHint;
			}
		}
	}
	return labelWidthHint + decorationWidget_->width();
}

void PropertySubtreeView::paintEvent(QPaintEvent* event) {
	drawHighlight(this, highlight_);
	recalculateLabelWidth();
	QWidget::paintEvent(event);
}

void PropertySubtreeView::playStructureChangeAnimation() {
	if (!item_->hasCollapsedParent()) {
		if (visibleRegion().isEmpty()) {
			Q_EMIT model_->addNotVisible(this);
		} else {
			auto* animation = new QPropertyAnimation(this, "highlight");
			animation->setDuration(250);
			animation->setStartValue(0.0);
			animation->setEndValue(1.0);
			QObject::connect(animation, &QPropertyAnimation::destroyed, this, [this]() {
				highlight_ = 0.0;
				update();
			});
			animation->start(QPropertyAnimation::DeleteWhenStopped);
		}
	} else {
		Q_EMIT model_->addNotVisible(this);
	}
}

}  // namespace raco::property_browser
