/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "application/ReportStatistics.h"

#include <algorithm>
#include <cassert>

namespace raco::application {

void ReportStatistics::addSnapshot(const Snapshot& snapshot) {
	auto curLength = store_.empty() ? 0 : store_.begin()->second.size();
	for (const auto& [key, value] : snapshot) {
		if (store_.find(key) == store_.end()) {
			store_[key] = TimingSeries{curLength};
		}
		addValue(store_[key], value);
	}
	for (const auto& [key, value] : store_) {
		if (snapshot.find(key) == snapshot.end()) {
			addValue(store_[key], {});
		}
	}
	auto newLength = store_.empty() ? 0 : std::min(maxLength_, curLength + 1);
	assert(std::all_of(store_.begin(), store_.end(), [newLength](const auto& item) {
		return item.second.size() == newLength;
	}));
}

const ReportStatistics::TimingSeriesStore& ReportStatistics::getTimingData() const {
	return store_;
}

void ReportStatistics::addValue(TimingSeries& timings, ValueType value) {
	timings.push_back(value);
	while (timings.size() > maxLength_) {
		timings.pop_front();
	}
}

}  // namespace raco::application::stats
