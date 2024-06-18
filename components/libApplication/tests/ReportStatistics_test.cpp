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

#include "testing/TestUtil.h"

using namespace raco::application;

using Snapshot = ReportStatistics::Snapshot;
using ValueType = std::chrono::microseconds;

class ReportStatisticsTest : public RacoBaseTest<> {
};

TEST_F(ReportStatisticsTest, addReportTest) {
	const Snapshot snapshot1{{"node1", ValueType{1}}};
	const Snapshot snapshot2{{"node2", ValueType{2}}};
	const Snapshot snapshot3{{"node1", ValueType{11}}};

	ReportStatistics stat{};
	stat.addSnapshot(snapshot1);
	const auto& timings = stat.getTimingData();
	ASSERT_EQ(timings.size(), 1);
	ASSERT_EQ(timings.at("node1"), std::deque<ValueType>({ValueType(1)}));

	stat.addSnapshot(snapshot2);
	ASSERT_EQ(timings.size(), 2);
	ASSERT_EQ(timings.at("node1"), std::deque<ValueType>({ValueType(1), ValueType(0)}));
	ASSERT_EQ(timings.at("node2"), std::deque<ValueType>({ValueType(0), ValueType(2)}));

	stat.addSnapshot(snapshot3);
	ASSERT_EQ(timings.size(), 2);
	ASSERT_EQ(timings.at("node1"), std::deque<ValueType>({ValueType(1), ValueType(0), ValueType(11)}));
	ASSERT_EQ(timings.at("node2"), std::deque<ValueType>({ValueType(0), ValueType(2), ValueType(0)}));
}

TEST_F(ReportStatisticsTest, lengthTest) {
	const Snapshot snapshot1{{"node1", ValueType{1}}};
	const Snapshot snapshot2{{"node2", ValueType{2}}};

	ReportStatistics stat{10};
	ASSERT_EQ(stat.getTimingData().size(), 0);

	stat.addSnapshot(snapshot1);
	for (int i = 0; i < 20; i++) {
		stat.addSnapshot(snapshot2);
	}

	ASSERT_EQ(stat.getTimingData().size(), 2);
	ASSERT_EQ(stat.getTimingData().at("node1").size(), 10);
	ASSERT_EQ(stat.getTimingData().at("node2").size(), 10);
}
