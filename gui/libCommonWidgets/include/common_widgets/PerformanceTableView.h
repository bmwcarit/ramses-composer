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
#include "application/RaCoApplication.h"

#include <QWidget>

namespace raco::common_widgets {

class PerformanceTableView : public QWidget {
	Q_OBJECT
public:
	explicit PerformanceTableView(application::RaCoApplication* application, components::SDataChangeDispatcher dispatcher, QWidget* parent);

Q_SIGNALS:
	void objectSelectionRequested(const QString& objectID);

private:
	static inline const auto ROW_HEIGHT = 22;

	application::RaCoApplication* application_;
	components::SDataChangeDispatcher dispatcher_;
};

}  // namespace raco::common_widgets
