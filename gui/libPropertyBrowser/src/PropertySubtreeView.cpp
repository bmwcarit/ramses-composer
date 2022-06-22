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
#include "property_browser/controls/ExpandButton.h"
#include "property_browser/editors/LinkEditor.h"
#include "property_browser/editors/PropertyEditor.h"
#include "property_browser/PropertyBrowserLayouts.h"
#include "property_browser/WidgetFactory.h"
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

PropertySubtreeView::PropertySubtreeView(PropertyBrowserModel* model, PropertyBrowserItem* item, QWidget* parent)
	: QWidget{parent}, item_{item}, model_{model}, layout_{this} {
	layout_.setAlignment(Qt::AlignTop);
	setContextMenuPolicy(Qt::CustomContextMenu);

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

		labelLayout->addWidget(decorationWidget_, 0);
		labelLayout->addWidget(label_, 0);
		labelLayout->addWidget(linkControl, 1);

		auto isLuaScriptProperty = !item->valueHandle().isObject() && &item->valueHandle().rootObject()->getTypeDescription() == &raco::user_types::LuaScript::typeDescription && !item->valueHandle().parent().isObject();
		if (isLuaScriptProperty) {
			label_->setToolTip(QString::fromStdString(item->luaTypeName()));
		}

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

	QObject::connect(item, &PropertyBrowserItem::errorChanged, this, &PropertySubtreeView::updateError);
	updateError();

	layout_.addWidget(labelContainer, 1, 0);

	if (item->expandable()) {
		// Events which can cause a build or destroy of the childrenContainer_
		QObject::connect(item, &PropertyBrowserItem::childrenChanged, this, &PropertySubtreeView::updateChildrenContainer);
		QObject::connect(item, &PropertyBrowserItem::showChildrenChanged, this, &PropertySubtreeView::updateChildrenContainer);
		QObject::connect(item, &PropertyBrowserItem::childrenChangedOrCollapsedChildChanged, this, &PropertySubtreeView::playStructureChangeAnimation);
		updateChildrenContainer();
	}
    insertKeyFrameAction_ = new QAction("insert KeyFrame", this);
    copyProperty_ = new QAction("copy Property", this);
    connect(insertKeyFrameAction_, &QAction::triggered, this, &PropertySubtreeView::slotInsertKeyFrame);
    connect(copyProperty_, &QAction::triggered, this, &PropertySubtreeView::slotCopyProperty);
    // 添加右键菜单栏
    connect(this, &PropertySubtreeView::customContextMenuRequested, this, &PropertySubtreeView::slotTreeMenu);
}

void PropertySubtreeView::updateError() {
	if (layout_.itemAtPosition(0, 0) && layout_.itemAtPosition(0, 0)->widget()) {
		auto* widget = layout_.itemAtPosition(0, 0)->widget();
		layout_.removeWidget(widget);
		widget->hide();
		widget->deleteLater();
	}
	if (item_->hasError()) {
		auto errorItem = item_->error();
		if (errorItem.category() == core::ErrorCategory::RAMSES_LOGIC_RUNTIME_ERROR || errorItem.category() == core::ErrorCategory::PARSE_ERROR || errorItem.category() == core::ErrorCategory::GENERAL || errorItem.category() == core::ErrorCategory::MIGRATION_ERROR) {
			layout_.addWidget(new ErrorBox(errorItem.message().c_str(), errorItem.level(), this), 0, 0);
			// It is unclear why this is needed - but without it, the error box does not appear immediately when an incompatible render buffer is assigned to a render target and the scene error view is in the background.
			// The error box does appear later, e. g. when the mouse cursor is moved over the preview or when the left mouse button is clicked in a different widget.
			update();
		}
	}
}

void PropertySubtreeView::slotTreeMenu(const QPoint &pos) {
    QMenu menu;
    if (item_->valueHandle().isProperty()) {
        std::string property = item_->valueHandle().getPropertyPath();
        QStringList strList = QString::fromStdString(property).split(".");
        // 判断是否为System/Custom Property, 且是否为Float类型
        if (!isValidValueHandle(strList, item_->valueHandle())) {
            return;
        }
		menu.addAction(insertKeyFrameAction_);
		menu.addAction(copyProperty_);
        menu.exec(QCursor::pos());
    }
}

void PropertySubtreeView::slotInsertKeyFrame() {
    raco::core::ValueHandle valueHandle = item_->valueHandle();
    if (valueHandle.isProperty()) {
        QString propertyPath = QString::fromStdString(valueHandle.getPropertyPath());
        if (valueHandle.parent() != NULL) {
            raco::core::ValueHandle parentHandle = valueHandle.parent();

            QString property;
            property = QString::fromStdString(parentHandle.getPropName()) + "." + QString::fromStdString(valueHandle.getPropName());
            if (QString::fromStdString(valueHandle.getPropertyPath()).contains("material")) {
                property = QString::fromStdString(valueHandle.getPropertyPath());
                property = property.section(".", 1);
            }

            // is have active animation
            std::string sampleProperty = animationDataManager::GetInstance().GetActiveAnimation();
            if (sampleProperty == std::string()) {
                return;
            }
            QString curve = QString::fromStdString(sampleProperty) + "_" + propertyPath;

            double value{0};
            if (valueHandle.type() == raco::core::PrimitiveType::Double) {
                value = valueHandle.asDouble();
            }
            std::map<std::string, std::string> bindingMap;
            NodeDataManager::GetInstance().getActiveNode()->NodeExtendRef().curveBindingRef().getPropCurve(sampleProperty, bindingMap);

            auto it = bindingMap.find(property.toStdString());
            if (it != bindingMap.end()) {
                Q_EMIT item_->model()->sigCreateCurve(property, QString::fromStdString(it->second), value);
                return;
            }

            // 若无对应binding
            Q_EMIT item_->model()->sigCreateCurveAndBinding(property, curve, value);
        }
    }
}

void PropertySubtreeView::slotCopyProperty() {
    raco::core::ValueHandle valueHandle = item_->valueHandle();
    if (valueHandle.isProperty()) {
        QString property = QString::fromStdString(valueHandle.getPropertyPath());
        if (property.contains("material")) {
            property = property.section(".", 1);
            QClipboard* clip = QApplication::clipboard();
            clip->setText(property);
            return;
        }
        std::string str = valueHandle.getPropName();
        if (valueHandle.parent() != NULL) {
            valueHandle = valueHandle.parent();
            QString property = QString::fromStdString(valueHandle.getPropName()) + "." + QString::fromStdString(str);
            QClipboard* clip = QApplication::clipboard();
            clip->setText(property);
        }
    }
}

bool PropertySubtreeView::isValidValueHandle(QStringList list, core::ValueHandle handle) {
    // lamada
    auto func = [&](QStringList tempList, raco::core::ValueHandle tempHandle)->bool {
        if (tempHandle.isObject()) {
            for (int i{1}; i < list.size(); i++) {
                QString str = list[i];
                tempHandle = tempHandle.get(str.toStdString());
                if (!tempHandle.isProperty()) {
                    return false;
                }
            }
        }
        return true;
    };

    if (list.contains("translation") || list.contains("rotation") || list.contains("scale")) {
        if (list.contains("x") || list.contains("y") || list.contains("z")) {
            return true;
        }
	} else if (list.contains("uniforms")) {
        if (list.contains("x") || list.contains("y") || list.contains("z")) {
            return true;
        } else {
            if (func(list, handle)) {
				if (!handle.hasProperty("x") && !handle.hasProperty("y") && !handle.hasProperty("z")) {
                    return true;
                }
            }
        }
	}
    return false;
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
		if (!item_->valueHandle().isObject() && item_->type() == core::PrimitiveType::Ref) {
			childrenContainer_ = new PropertySubtreeChildrenContainer{item_, this};
			childrenContainer_->addWidget(new EmbeddedPropertyBrowserView{item_, this});
			layout_.addWidget(childrenContainer_, 2, 0);
		} else if (item_->valueHandle().isObject() || hasTypeSubstructure(item_->type())) {
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
				recalculateTabOrder();
			});
			recalculateLabelWidth();
			layout_.addWidget(childrenContainer_, 2, 0);
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
