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

#include "components/DataChangeDispatcher.h"

#include <QAbstractTableModel>

namespace raco::application {
class RaCoApplication;
}

namespace raco::common_widgets {

class PerformanceModel : public QAbstractTableModel {
	Q_OBJECT

	using base = QAbstractTableModel;

	enum class Columns : int {
		ID = 0,
		Current = 1,
		Average = 2,
		Maximum = 3,
		COLUMN_COUNT
	};

public:
	explicit PerformanceModel(application::RaCoApplication* application, QObject* parent = nullptr);

	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	QVariant data(const QModelIndex& index, int role) const override;

	std::string objectID(const QModelIndex& index) const;

private:
	void rebuildItems();

	struct Item {
		std::string objectID;
		std::string objectName;
		std::chrono::microseconds time_cur;
		std::chrono::microseconds time_avg;
		std::chrono::microseconds time_max;
	};

	application::RaCoApplication* application_;

	std::vector<Item> items_;
	bool dirty_ = false;

	components::Subscription lifeCycleSubscription_;
	components::Subscription afterDispatchSubscription_;
};

}  // namespace raco::common_widgets