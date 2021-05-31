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

#include "DockManager.h"

#include <QSettings>

// Qt Advanced Docking System dock manager extension
// because the Qt Advanced Docking System is not designed to e.g. regenerate closed widgets when restoring a perspective
class RaCoDockManager : public ads::CDockManager {
public:
	using LayoutDocks = QVector<QPair<QString, QString>>;
	using ExtraLayoutInfo = QMap<QString, LayoutDocks>;

	RaCoDockManager(QWidget *parent = nullptr);
	~RaCoDockManager();

	void loadAllLayouts(QSettings &settings);
	void saveCustomLayouts(QSettings &settings);
	void saveCurrentLayoutInCache(QSettings &settings);

	void addCustomLayout(const QString &layoutName);
	void removeCustomLayout(const QString &layoutName);

	void restoreCachedLayoutState();
	const LayoutDocks getCachedLayoutInfo() const;
	const LayoutDocks getCustomLayoutInfo(const QString &layoutName) const;

protected:
	ExtraLayoutInfo customLayoutInfos_;
	LayoutDocks cachedLayoutInfo_;
	QByteArray cachedLayoutState_;

	void saveCustomDocks(QSettings &settings, const QString &layoutName, const LayoutDocks &savedDocks);
};