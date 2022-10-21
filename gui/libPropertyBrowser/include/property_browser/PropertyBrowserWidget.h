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

#include <QWidget>
#include <QMetaObject>
#include <unordered_set>
#include <QMenu>

#include "property_browser/PropertyBrowserItem.h"
#include "property_browser/PropertyBrowserLayouts.h"
#include "property_browser_extend/PropertyBrowserNodeWidget.h"
#include "property_browser_extend/PropertyBrowserMeshWidget.h"
#include "property_browser_extend/PropertyBrowserCustomWidget.h"
#include "property_browser_extend/PropertyBrowserCurveBindingWidget.h"
#include "property_browser/PropertySubtreeView.h"

class QPushButton;

namespace raco::property_browser {
class PropertyBrowserItem;
class PropertyBrowserModel;

class PropertyBrowserView final : public QWidget {
    Q_OBJECT
public:
	explicit PropertyBrowserView(PropertyBrowserItem* item, PropertyBrowserModel* model, QWidget* parent = nullptr);

	std::string getCurrentObjectID() const;

private:
	QPoint verticalPivot_{0, 0};
	QWidget* verticalPivotWidget_{nullptr};
	std::string currentObjectID_;
    PropertySubtreeView* propertySubTreeView_{nullptr};
};

class PropertyBrowserWidget final : public QWidget {
    Q_OBJECT
public:
	explicit PropertyBrowserWidget(
		raco::components::SDataChangeDispatcher dispatcher,
		raco::core::CommandInterface* commandInterface,
		QWidget* parent = nullptr);

	PropertyBrowserModel* model() const;
    void initPropertyBrowserWidget();

public Q_SLOTS:
	void setValueHandle(raco::core::ValueHandle valueHandle);
	void setValueHandles(const std::set<raco::core::ValueHandle>& valueHandles);
	void clear();
    void setLockable(bool lockable);

    void slotInsertCurveBinding(QString property, QString curve);
    void slotTreeMenu(const QPoint& pos);
    void slotRefreshPropertyBrowser();
    void slotRefreshPropertyBrowserAfterUndo(raco::core::ValueHandle valueHandle);
	void slotRefreshCurveBindingWidget();

private:
	void setLocked(bool locked);
	void clearValueHandle(bool restorable);
    void switchNode(std::string objectID);

	raco::components::SDataChangeDispatcher dispatcher_;
	raco::core::CommandInterface* commandInterface_;
	PropertyBrowserGridLayout layout_;
	std::unique_ptr<PropertyBrowserView> propertyBrowser_{};
	raco::components::Subscription subscription_;
	std::string restorableObjectId_;
	QWidget* emptyLabel_;
	bool locked_;
	PropertyBrowserModel* model_;
	QPushButton* lockButton_;
	PropertyBrowserItem* item_{nullptr};
    PropertyBrowserNodeWidget* systemNodeWidget_{nullptr};
    PropertyBrowserNodeWidget* meshNodeWidget_{nullptr};
    PropertyBrowserNodeWidget* customNodeWidget_{nullptr};
    PropertyBrowserNodeWidget* curveBindingNodeWidget_{nullptr};
    PropertyBrowserMeshWidget* meshWidget_{nullptr};
    PropertyBrowserCustomWidget* customWidget_{nullptr};
    PropertyBrowserCurveBindingWidget* curveBindingWidget_{nullptr};
};

}  // namespace raco::property_browser
