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

#include "core/Context.h"
#include "core/Handles.h"
#include "components/DataChangeDispatcher.h"
#include <QObject>
#include <utility>
#include <vector>

namespace raco::core {
class CommandInterface;
}

namespace raco::property_browser {

class PropertyBrowserItem;

class PropertyBrowserRef final : public QObject {
	Q_OBJECT
public:
	struct RefItem {
		QString objName;
		QString objId;
		QString tooltipText;
	};

	using RefItems = std::vector<RefItem>;

	static inline auto EMPTY_REF_INDEX{0};

	explicit PropertyBrowserRef(PropertyBrowserItem* parent);

	const RefItems& items() const noexcept;
	int currentIndex() noexcept;

	Q_SIGNAL void indexChanged(int index);
	Q_SIGNAL void itemsChanged(const RefItems& items);

	Q_SLOT void setIndex(int index) noexcept;

protected:
	void update() noexcept;
	void updateItems() noexcept;
	void updateIndex() noexcept;

private:
	RefItems items_{};
	int index_{0};
	PropertyBrowserItem* parent_;
	raco::components::Subscription subscription_;
	raco::components::Subscription lifecycleSub_;
	raco::components::Subscription childMoveSub_;
	std::vector<raco::components::Subscription> objectNames_;
};

}  // namespace raco::property_browser
