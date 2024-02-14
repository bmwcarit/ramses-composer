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

#include "core/SceneBackendInterface.h"
#include "property_browser/PropertyBrowserItem.h"
#include "property_browser/PropertyBrowserLayouts.h"
#include "property_browser/PropertySubtreeView.h"

#include <QMenu>

class QPushButton;

namespace raco::object_tree::view {
class ObjectTreeDockManager;
}

namespace raco::property_browser {
class PropertyBrowserItem;
class PropertyBrowserModel;

class PropertyBrowserWidget final : public QWidget {
public:
	explicit PropertyBrowserWidget(
		components::SDataChangeDispatcher dispatcher,
		core::CommandInterface* commandInterface,
		core::SceneBackendInterface* sceneBackend,
		object_tree::view::ObjectTreeDockManager* treeDockManager,
		QWidget* parent = nullptr);

	PropertyBrowserModel* model() const;

public Q_SLOTS:
	void setObjectFromObjectId(const QString& objectID, const QString& objectProperty);
	void setObjects(const core::SEditorObjectSet& objects, const QString& property);
	void highlightProperty(const QString& property);
	void clear();
	void setLockable(bool lockable) const;

private Q_SLOTS:
	void showRefToThis();

private:
	void showScrollBar(bool isAlwaysOn);
	void setLocked(bool locked);
	void setObjectsImpl(const core::SEditorObjectSet& objects, bool forceExpandStateUpdate);
	std::string getObjectIdInPrefab() const;

	components::SDataChangeDispatcher dispatcher_;
	core::CommandInterface* commandInterface_;
	core::SceneBackendInterface* sceneBackend_;
	object_tree::view::ObjectTreeDockManager* treeDockManager_;
	PropertyBrowserGridLayout layout_;
	std::unique_ptr<PropertySubtreeView> propertyBrowser_{};
	PropertyBrowserItem* rootItem_ = nullptr;
	core::SEditorObjectSet currentObjects_;
	components::Subscription lifecycleSubs_;
	QWidget* emptyLabel_;
	bool locked_;
	PropertyBrowserModel* model_;
	QPushButton* lockButton_;
	QPushButton* refButton_;
	QPushButton* prefabLookupButton_;
};

}  // namespace raco::property_browser
