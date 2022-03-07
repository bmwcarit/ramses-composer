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

#include "common_widgets/log_model/LogViewModel.h"
#include "log_system/log.h"

#include <memory>
#include <mutex>
#include <spdlog/sinks/base_sink.h>

namespace raco::common_widgets {

class LogViewModel;

class LogViewSink final : public spdlog::sinks::base_sink<std::mutex> {
public:
	explicit LogViewSink(LogViewModel* model);

	void sink_it_(const spdlog::details::log_msg& msg) override;
	void flush_() override;

private:
	LogViewModel* model_;
};

}  // namespace raco::common_widgets