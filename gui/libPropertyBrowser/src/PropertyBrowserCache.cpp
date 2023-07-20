/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */


#include "property_browser/PropertyBrowserCache.h"

namespace raco::property_browser {

PropertyBrowserCache& PropertyBrowserCache::instance() noexcept {
	static PropertyBrowserCache instance_{};
	return instance_;
}

std::optional<bool> PropertyBrowserCache::getCachedExpandedState(const std::string& propertyName) {
	if (cachedExpandedStates_.find(propertyName) != cachedExpandedStates_.end()) {
		return cachedExpandedStates_[propertyName];
	}

	return std::optional<bool>{};
}

void PropertyBrowserCache::cacheExpandedState(const std::string& propertyName, bool expanded) {
	cachedExpandedStates_[propertyName] = expanded;
	instance().Q_EMIT newExpandedStateCached(propertyName, expanded);
}

}  // namespace raco::property_browser
