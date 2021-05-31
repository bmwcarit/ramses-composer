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

namespace raco::common_widgets {

// A wrapper class around all QT layout classes, that removes the layout margins.
template <typename LayoutType>
class NoContentMarginsLayout : public LayoutType {
public:
	explicit NoContentMarginsLayout(QWidget* parent) : LayoutType{parent} {
		this->setContentsMargins(0, 0, 0, 0);
	}
};

}  // namespace raco::common_widgets