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

#include <QList>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QWidget>

#include "property_browser/PropertyBrowserItem.h"
#include "property_browser/PropertyBrowserLayouts.h"

namespace raco::property_browser {

class PropertySubtreeChildrenContainer final : public QWidget {
	Q_OBJECT
public:
	explicit PropertySubtreeChildrenContainer(PropertyBrowserItem* item, QWidget* parent);
	void addWidget(QWidget* child);
	void removeWidget(QWidget* child);
	void insertWidget(size_t index, QWidget* child);
public Q_SLOTS:
	void setOffset(int offset);

private:
	PropertyBrowserVBoxLayout* layout_;
};

}  // namespace raco::property_browser
