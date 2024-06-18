/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "common_widgets/PerformanceModel.h"

#include "application/RaCoApplication.h"
#include "core/Project.h"

namespace raco::common_widgets {

PerformanceModel::PerformanceModel(application::RaCoApplication* application, QObject* parent)
	: QAbstractTableModel(parent),
	  application_(application) {
	QObject::connect(application, &application::RaCoApplication::performanceStatisticsUpdated, this, [this]() {
		dirty_ = true;
	});

	lifeCycleSubscription_ = application->dataChangeDispatcher()->registerOnObjectsLifeCycle(
		[this](auto SEditorObject) { dirty_ = true; },
		[this](auto SEditorObject) { dirty_ = true; });

	afterDispatchSubscription_ = application->dataChangeDispatcher()->registerOnAfterDispatch([this]() {
		if (dirty_) {
			rebuildItems();
		}
	});
}

void PerformanceModel::rebuildItems() {
	beginResetModel();
	items_.clear();
	items_.reserve(application_->getLogicStats().getTimingData().size());
	for (const auto& [objectID, timings] : application_->getLogicStats().getTimingData()) {
		if (auto obj = application_->activeRaCoProject().project()->getInstanceByID(objectID)) {
			auto current = timings.back();
			auto max = *std::max_element(timings.begin(), timings.end());
			auto avg = std::accumulate(timings.begin(), timings.end(), std::chrono::microseconds{0}) / timings.size();
			items_.emplace_back(Item{objectID, obj->objectName(), current, avg, max});
		}
	}
	endResetModel();
	dirty_ = false;
}

int PerformanceModel::rowCount(const QModelIndex& parent) const {
	return items_.size();
}

int PerformanceModel::columnCount(const QModelIndex& parent) const {
	return static_cast<int>(Columns::COLUMN_COUNT);
}

QVariant PerformanceModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if (role == Qt::DisplayRole) {
		switch (static_cast<Columns>(section)) {
			case Columns::ID:
				return {"Name"};
			case Columns::Current:
				return {"Current (us)"};
			case Columns::Average:
				return {"Average (us)"};
			case Columns::Maximum:
				return {"Maximum (us)"};
		}
	}
	return base::headerData(section, orientation, role);
}

QVariant PerformanceModel::data(const QModelIndex& index, int role) const {
	if (role == Qt::DisplayRole) {
		switch (static_cast<Columns>(index.column())) {
			case Columns::ID:
				return QVariant(QString::fromStdString(items_[index.row()].objectName));
			case Columns::Current:
				return QVariant(static_cast<qlonglong>(items_[index.row()].time_cur.count()));
			case Columns::Average:
				return QVariant(static_cast<qlonglong>(items_[index.row()].time_avg.count()));
			case Columns::Maximum:
				return QVariant(static_cast<qlonglong>(items_[index.row()].time_max.count()));
		}
	}
	return {};
}

std::string PerformanceModel::objectID(const QModelIndex& index) const {
	return items_[index.row()].objectID;
}

}  // namespace raco::common_widgets