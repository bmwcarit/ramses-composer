/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/PropertySubtreeView.h"

#include "ErrorBox.h"
#include "core/CoreFormatter.h"
#include "log_system/log.h"
#include "property_browser/PropertyBrowserLayouts.h"
#include "property_browser/WidgetFactory.h"
#include "property_browser/editors/LinkEditor.h"
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

PropertySubtreeView::PropertySubtreeView(PropertyBrowserModel* model, PropertyBrowserItem* item, QWidget* parent)
	: QWidget{parent}, item_{item}, model_{model}, layout_{this} {
	LOG_TRACE(log_system::PROPERTY_BROWSER, "{}", item->valueHandle());
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
	if (!item->valueHandle().isObject()) {
		button_ = new ExpandControlButton{item, labelContainer};
		button_->setFixedWidth(28);
		label_ = WidgetFactory::createPropertyLabel(item, labelContainer);
		label_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
		auto* linkControl = WidgetFactory::createLinkControl(item, labelContainer);

		propertyControl_ = WidgetFactory::createPropertyControl(item, labelContainer);
		propertyControl_->setEnabled(item->editable());
		QObject::connect(item, &PropertyBrowserItem::editableChanged, propertyControl_, &QWidget::setEnabled);
		propertyControl_->setVisible(item->showControl());
		QObject::connect(item, &PropertyBrowserItem::showControlChanged, this, [this](bool show) { propertyControl_->setVisible(show); });
		linkControl->setControl(propertyControl_);
		label_->setEnabled(item->editable());
		QObject::connect(item, &PropertyBrowserItem::editableChanged, label_, &QWidget::setEnabled);

		labelLayout->addWidget(button_, 0);
		labelLayout->addWidget(label_, 0);
		labelLayout->addWidget(linkControl, 1);
	} else {
		// Dummy label for the case that we are an object.
		button_ = new QWidget{this};
		label_ = new QLabel{this};
		button_->setFixedWidth(0);
		button_->setFixedHeight(0);
		label_->setFixedWidth(0);
		label_->setFixedHeight(0);
		labelLayout->addWidget(button_, 0);
		labelLayout->addWidget(label_, 0);
	}

	QObject::connect(item, &PropertyBrowserItem::errorChanged, this, &PropertySubtreeView::updateError);
	updateError();

	layout_.addWidget(labelContainer, 1, 0);

	// Events which can cause a build or destroy of the childrenContainer_
	QObject::connect(item, &PropertyBrowserItem::childrenChanged, this, &PropertySubtreeView::updateChildrenContainer);
	QObject::connect(item, &PropertyBrowserItem::showChildrenChanged, this, &PropertySubtreeView::updateChildrenContainer);
	QObject::connect(item, &PropertyBrowserItem::childrenChangedOrCollapsedChildChanged, this, &PropertySubtreeView::playStructureChangeAnimation);
	updateChildrenContainer();
}

void PropertySubtreeView::updateError() {
	if (layout_.itemAtPosition(0, 0) && layout_.itemAtPosition(0, 0)->widget()) {
		auto* widget = layout_.itemAtPosition(0, 0)->widget();
		layout_.removeWidget(widget);
		widget->deleteLater();
	}
	if (item_->hasError()) {
		auto errorItem = item_->error();
		if (errorItem.category() == core::ErrorCategory::RAMSES_LOGIC_RUNTIME_ERROR || errorItem.category() == core::ErrorCategory::PARSE_ERROR || errorItem.category() == core::ErrorCategory::GENERAL) {
			layout_.addWidget(new ErrorBox(errorItem.message().c_str(), errorItem.level(), this), 0, 0);
		}
	}
}

void PropertySubtreeView::updatePropertyControl() {
	if (propertyControl_) {
		propertyControl_->setVisible(!item_->expanded() || item_->size() == 0);
	}
}

void PropertySubtreeView::updateChildrenContainer() {
	if (item_->showChildren() && !childrenContainer_) {
		if (!item_->valueHandle().isObject() && item_->type() == core::PrimitiveType::Ref) {
			childrenContainer_ = new PropertySubtreeChildrenContainer{item_, this};
			childrenContainer_->addWidget(new EmbeddedPropertyBrowserView{item_, this});
			layout_.addWidget(childrenContainer_, 2, 0);
		} else if (item_->valueHandle().isObject() || hasTypeSubstructure(item_->type())) {
			LOG_TRACE(log_system::PROPERTY_BROWSER, "Adding {} children for {}", item_->children().size(), item_->valueHandle());
			childrenContainer_ = new PropertySubtreeChildrenContainer{item_, this};

			for (const auto& child : item_->children()) {
				auto* subtree = new PropertySubtreeView{model_, child, childrenContainer_};
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
					auto* subtree = new PropertySubtreeView{model_, child, childrenContainer_};
					childrenContainer_->addWidget(subtree);
				}
			});
			recalculateLabelWidth();
			layout_.addWidget(childrenContainer_, 2, 0);
		}
	} else if (!item_->showChildren() && childrenContainer_) {
		childrenContainer_->deleteLater();
		childrenContainer_ = nullptr;
	}
}

void PropertySubtreeView::setLabelAreaWidth(int width) {
	if (labelWidth_ != width) {
		labelWidth_ = width;
		recalculateLabelWidth();
	}
}

void PropertySubtreeView::recalculateLabelWidth() {
	if (childrenContainer_ != nullptr) {
		childrenContainer_->setOffset(button_->width());
	}
	const int labelWidthHint = std::max(labelWidth_, getLabelAreaWidthHint() - button_->width());
	if (label_->width() != labelWidthHint) {
		label_->setFixedWidth(labelWidthHint);
	}

	for (const auto& child : childrenContainer_->findChildren<PropertySubtreeView*>(QString{}, Qt::FindDirectChildrenOnly)) {
		child->setLabelAreaWidth(labelWidthHint - button_->width());
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
	return labelWidthHint + button_->width();
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
