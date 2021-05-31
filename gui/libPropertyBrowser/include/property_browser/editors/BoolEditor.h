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

class QCheckBox;

namespace raco::property_browser {

class PropertyBrowserItem;

class BoolEditor final : public QWidget {
public:
	explicit BoolEditor(
		PropertyBrowserItem* item,
		QWidget* parent = nullptr);

protected:
	QCheckBox *checkBox_;
};

}  // namespace raco::property_browser
