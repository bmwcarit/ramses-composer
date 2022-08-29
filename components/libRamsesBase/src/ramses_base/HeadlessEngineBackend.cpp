/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_base/HeadlessEngineBackend.h"

namespace raco::ramses_base {

ramses::RamsesFrameworkConfig& ramsesFrameworkConfig() noexcept {
	char const* argv[] = {"RamsesComposer.exe",
		"--log-level-contexts-filter", "trace:RAPI,off:RPER,debug:RRND,off:RFRA,off:RDSM,info:RCOM",
		"--log-level-console", "warn",
		"--log-level-dlt", "warn"};
	static ramses::RamsesFrameworkConfig config(sizeof(argv) / sizeof(argv[0]), argv);
	return config;
}

HeadlessEngineBackend::HeadlessEngineBackend(rlogic::EFeatureLevel featureLevel)
	: BaseEngineBackend{featureLevel, ramsesFrameworkConfig()} {
	connect();
}

}  // namespace raco::ramses_base
