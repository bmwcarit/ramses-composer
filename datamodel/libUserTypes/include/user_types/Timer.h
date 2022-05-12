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

#include "user_types/BaseObject.h"


namespace raco::user_types {

class Timer : public BaseObject {
public:
	static inline const TypeDescriptor typeDescription = { "Timer", true };
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	Timer(Timer const& other) : BaseObject(other), tickerInput_(other.tickerInput_), tickerOutput_(other.tickerOutput_) {
		fillPropertyDescription();
	}

	Timer(std::string name = std::string(), std::string id = std::string())
		: BaseObject(name, id) {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("tickerInput", &tickerInput_);
		properties_.emplace_back("tickerOutput", &tickerOutput_);
	}

	void onAfterValueChanged(BaseContext& context, ValueHandle const& value) override;

	Property<int64_t, DisplayNameAnnotation, LinkEndAnnotation> tickerInput_{false, {"Ticker Input"}, {}};
	Property<int64_t, DisplayNameAnnotation, LinkStartAnnotation> tickerOutput_{false, {"Ticker Output"}, {}};

};

using STimer = std::shared_ptr<Timer>;

}
