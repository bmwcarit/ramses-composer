/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include <QWidget>
#include <QMetaObject>
#include <unordered_set>

#include "property_browser/PropertyBrowserItem.h"
#include "property_browser/PropertyBrowserLayouts.h"

class QPushButton;

namespace raco::property_browser {
class PropertyBrowserItem;
class PropertyBrowserModel;

class PropertyBrowserView final : public QWidget {
public:
	explicit PropertyBrowserView(PropertyBrowserItem* item, PropertyBrowserModel* model, QWidget* parent = nullptr);

	std::string getCurrentObjectID() const;

private:
	QPoint verticalPivot_{0, 0};
	QWidget* verticalPivotWidget_{nullptr};
	std::string currentObjectID_;
};

class PropertyBrowserWidget final : public QWidget {
public:
	explicit PropertyBrowserWidget(
		raco::components::SDataChangeDispatcher dispatcher,
		raco::core::CommandInterface* commandInterface,
		QWidget* parent = nullptr);

	PropertyBrowserModel* model() const;

public Q_SLOTS:
	void setValueHandleFromObjectId(const QString& objectID);
	void setValueHandle(raco::core::ValueHandle valueHandle);
	void setValueHandles(const std::set<raco::core::ValueHandle>& valueHandles);
	void clear();
	void setLockable(bool lockable);

private:
	void setLocked(bool locked);
	void clearValueHandle(bool restorable);

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
};

}  // namespace raco::property_browser
