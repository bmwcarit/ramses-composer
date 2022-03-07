/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include <QFrame>
#include <QPushButton>
#include <QStyle>
#include <QWidget>
#include <QLabel>

#include "property_browser/PropertyBrowserItem.h"
#include "property_browser/PropertyBrowserLayouts.h"
#include "property_browser/PropertyBrowserModel.h"
#include "property_browser/PropertySubtreeChildrenContainer.h"

namespace raco::property_browser {
class PropertyControl;

class EmbeddedPropertyBrowserView final : public QFrame {
public:
	explicit EmbeddedPropertyBrowserView(PropertyBrowserItem* item, QWidget* parent);
};

class PropertySubtreeView final : public QWidget {
	Q_OBJECT
	Q_PROPERTY(float highlight MEMBER highlight_ NOTIFY update)
public:
	explicit PropertySubtreeView(PropertyBrowserModel* model, PropertyBrowserItem* item, QWidget* parent);
	PropertyBrowserItem const* item() { return item_; }
public Q_SLOTS:
	void playStructureChangeAnimation();
	void setLabelAreaWidth(int offset);
	void updateChildrenContainer();

protected:
	void paintEvent(QPaintEvent* event) override;
	int getLabelAreaWidthHint() const;
	Q_SLOT void updateError();
private:
	void recalculateLabelWidth();
	void collectTabWidgets(QObject* item, QWidgetList& tabWidgets);
	void recalculateTabOrder();

	PropertyBrowserItem* item_{nullptr};
	PropertyBrowserModel* model_ {nullptr};
	PropertyBrowserGridLayout layout_{nullptr};
	QWidget* decorationWidget_{nullptr};
	QLabel* label_{nullptr};
	QWidget* propertyControl_{nullptr};
	PropertySubtreeChildrenContainer* childrenContainer_{nullptr};
	int labelWidth_{0};
	float highlight_{0};
};

}  // namespace raco::property_browser
