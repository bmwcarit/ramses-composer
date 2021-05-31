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

#include <iostream>
#include <log_system/log.h>

using raco::log_system::COMMON;
template <typename T>
class DebugInstanceCounter {
#ifdef RACO_USE_DEBUG_INSTANCE_COUNTER
private:
	static inline uint64_t counter{0};

public:
	DebugInstanceCounter() {
		counter++;
		LOG_DEBUG(COMMON, "count: {}", counter);
	}
	~DebugInstanceCounter() {
		counter--;
		LOG_DEBUG(COMMON, "count: {}", counter);
	}
#endif
};

#define DEBUG_INSTANCE_COUNTER(TYPE) \
private:                             \
	DebugInstanceCounter<TYPE> debugInstanceCounter_
