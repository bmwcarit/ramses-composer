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

#include <QObject>

#include <optional>
#include <string>
#include <map>

namespace raco::property_browser {

class PropertyBrowserCache : public QObject {
	Q_OBJECT

public:
	static PropertyBrowserCache& instance() noexcept;
	
	std::optional<bool> getCachedExpandedState(const std::string& propertyName);
	void cacheExpandedState(const std::string& propertyName, bool expanded);

Q_SIGNALS:
	void newExpandedStateCached(const std::string& propertyName, bool expanded);

private:
	PropertyBrowserCache(){};

	std::map<std::string, bool> cachedExpandedStates_;
};

}  // namespace raco::property_browser
