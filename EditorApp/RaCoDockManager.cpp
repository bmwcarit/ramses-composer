/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "RaCoDockManager.h"

RaCoDockManager::RaCoDockManager(QWidget* parent) : ads::CDockManager(parent) {
}

RaCoDockManager::~RaCoDockManager() {
	// explicitly delete all widgets
	// because CDockManager does not delete all tabbed widgets when destroyed, only the visible ones
	// TODO: Delete this with the newest qt-ADS release, the newest release will fix this.
	for (auto widget : dockWidgetsMap()) {
		delete widget;
	}
}

void RaCoDockManager::loadAllLayouts(QSettings& settings) {
	auto loadCustomDocks = [&settings](const QString& layoutName, LayoutDocks& docks) {
		settings.beginGroup(layoutName);
		auto layoutDockAmount = settings.beginReadArray("Docks");
		for (int i = 0; i < layoutDockAmount; ++i) {
			settings.setArrayIndex(i);
			auto dockType = settings.value("Dock Type").toString();
			auto dockName = settings.value("Dock Name").toString();
			if (dockType.isEmpty() || dockName.isEmpty()) {
				continue;
			}

			docks.append({dockType, dockName});
		}
		settings.endArray();
		settings.endGroup();
	};

	cachedLayoutInfo_.clear();
	customLayoutInfos_.clear();

	auto childKeys = settings.childGroups();

	if (childKeys.contains("cachedLayout")) {
		loadCustomDocks("cachedLayout", cachedLayoutInfo_);
		cachedLayoutState_ = settings.value("cachedLayout/cachedLayoutState").toByteArray();
	}

	if (childKeys.contains("customLayouts")) {
		settings.beginGroup("customLayouts");

		for (auto layoutName : settings.childGroups()) {
			loadCustomDocks(layoutName, customLayoutInfos_[layoutName]);
		}

		settings.endGroup();

		ads::CDockManager::loadPerspectives(settings);
	}
}

void RaCoDockManager::saveCustomLayouts(QSettings& settings) {
	// clear all saved layouts and remove layouts from settings that have been possibly deleted in RaCo
	settings.remove("customLayouts");
	settings.remove("Perspectives");

	settings.beginGroup("customLayouts");
	for (auto layoutListIt = customLayoutInfos_.constBegin(); layoutListIt != customLayoutInfos_.constEnd(); ++layoutListIt) {
		auto &layoutName = layoutListIt.key();
		auto &layoutDocks = layoutListIt.value();
		saveCustomDocks(settings, layoutName, layoutDocks);
	}
	settings.endGroup();

	ads::CDockManager::savePerspectives(settings);
}

void RaCoDockManager::saveCurrentLayoutInCache(QSettings& settings) {
	cachedLayoutInfo_.clear();
	for (auto dockWidget : dockWidgetsMap()) {
		cachedLayoutInfo_.append({dockWidget->windowTitle(), dockWidget->objectName()});
	}

	cachedLayoutState_ = ads::CDockManager::saveState();

	settings.remove("cachedLayout");
	saveCustomDocks(settings, "cachedLayout", cachedLayoutInfo_);

	settings.setValue("cachedLayout/cachedLayoutState", cachedLayoutState_);
}

void RaCoDockManager::addCustomLayout(const QString& layoutName) {
	if (customLayoutInfos_.contains(layoutName)) {
		customLayoutInfos_[layoutName].clear();
	}

	for (auto dockWidget : dockWidgetsMap()) {
		customLayoutInfos_[layoutName].append({dockWidget->windowTitle(), dockWidget->objectName()});
	}

	ads::CDockManager::addPerspective(layoutName);
}

void RaCoDockManager::removeCustomLayout(const QString& layoutName) {
	customLayoutInfos_.erase(customLayoutInfos_.find(layoutName));
	ads::CDockManager::removePerspective(layoutName);
}

void RaCoDockManager::restoreCachedLayoutState() {
	if (!cachedLayoutState_.isEmpty()) {
		ads::CDockManager::restoreState(cachedLayoutState_);
	}
}

const RaCoDockManager::LayoutDocks RaCoDockManager::getCachedLayoutInfo() const {
	return cachedLayoutInfo_;
}

const RaCoDockManager::LayoutDocks RaCoDockManager::getCustomLayoutInfo(const QString& layoutName) const {
	if (!customLayoutInfos_.contains(layoutName)) {
		return {};
	}

	return customLayoutInfos_[layoutName];
}

void RaCoDockManager::saveCustomDocks(QSettings& settings, const QString& layoutName, const LayoutDocks& savedDocks) {
	settings.beginGroup(layoutName);
	settings.beginWriteArray("Docks", savedDocks.size());
	auto i = 0;
	for (auto dockIt = savedDocks.constBegin(); dockIt != savedDocks.constEnd(); ++dockIt) {
		settings.setArrayIndex(i++);
		settings.setValue("Dock Type", dockIt->first);
		settings.setValue("Dock Name", dockIt->second);
	}
	settings.endArray();
	settings.endGroup();
}

