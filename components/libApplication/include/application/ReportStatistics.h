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

#include <chrono>
#include <deque>
#include <map>
#include <string>

namespace raco::application {

class ReportStatistics {
public:
	using NodeDescriptor = std::string;
	using ValueType = std::chrono::microseconds;

	using Snapshot = std::map<NodeDescriptor, ValueType>;
	using TimingSeries = std::deque<ValueType>;
	using TimingSeriesStore = std::map<NodeDescriptor, TimingSeries>;

	ReportStatistics(typename TimingSeries::size_type maxLength = 100) : maxLength_(maxLength) {
	}

	/**
	 * @brief Add a snapshot of timings to to statistics.
	 * 
	 * All timing series in the store will always be of the same length. 
	 * Missing values in the snapshot for existing nodes are filled with 0.
	 * Keeps at most maxLength_ values discarding the oldest one if size overflows.
	 * 
	 * @param snapshot Snapshot of timing data from last update. Indexed by object id
	*/
	void addSnapshot(const Snapshot& snapshot);

	const TimingSeriesStore& getTimingData() const;

private:
	void addValue(TimingSeries& timings, ValueType value);

	typename TimingSeries::size_type maxLength_;

	std::map<NodeDescriptor, TimingSeries> store_;
};

}  // namespace raco::application
