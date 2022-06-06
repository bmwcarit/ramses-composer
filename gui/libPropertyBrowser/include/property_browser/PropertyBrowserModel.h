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

#include <QObject>

namespace raco::property_browser {

/**
 * Model for signals which will always be handled by the PropertyBrowser.
 * No bubbeling necessary.
 */
class PropertyBrowserModel final : public QObject {
	Q_OBJECT
public:
	explicit PropertyBrowserModel(QObject* parent = nullptr) noexcept
		: QObject{parent} {}
Q_SIGNALS:
	void addNotVisible(QWidget* source);
	void removeNotVisible(QWidget* source);
	void beforeStructuralChange(QWidget* toChange = nullptr);
	void beforeRemoveWidget(QWidget* toRemove);
	void objectSelectionRequested(const QString objectID);
	void sigCreateCurve(QString property, QString curve, QVariant value);
    void sigCreateCurveAndBinding(QString property, QString curve, QVariant value);
};

}  // namespace raco::property_browser
