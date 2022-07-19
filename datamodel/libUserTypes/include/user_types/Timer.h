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

class TimerInput : public StructBase {
public:
	static inline const TypeDescriptor typeDescription = {"TimerInput", false};

	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	bool serializationRequired() const override {
		return true;
	}

	TimerInput(const TimerInput& other, std::function<SEditorObject(SEditorObject)>* translateRef = nullptr)
		: StructBase(),
		  ticker_us_(other.ticker_us_) {
		fillPropertyDescription();
	}

	TimerInput() : StructBase() {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("ticker_us", &ticker_us_);
	}

	void copyAnnotationData(const TimerInput& other) {
		ticker_us_.copyAnnotationData(other.ticker_us_);
	}

	TimerInput& operator=(const TimerInput& other) {
		ticker_us_ = other.ticker_us_;
		return *this;
	}

public:
	Property<int64_t, DisplayNameAnnotation, LinkEndAnnotation> ticker_us_{false, {"Ticker Input"}, {}};
};

class TimerOutput : public StructBase {
public:
	static inline const TypeDescriptor typeDescription = {"TimerOutput", false};

	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	bool serializationRequired() const override {
		return true;
	}

	TimerOutput(const TimerOutput& other, std::function<SEditorObject(SEditorObject)>* translateRef = nullptr)
		: StructBase(),
		  ticker_us_(other.ticker_us_) {
		fillPropertyDescription();
	}

	TimerOutput() : StructBase() {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("ticker_us", &ticker_us_);
	}

	void copyAnnotationData(const TimerOutput& other) {
		ticker_us_.copyAnnotationData(other.ticker_us_);
	}

	TimerOutput& operator=(const TimerOutput& other) {
		ticker_us_ = other.ticker_us_;
		return *this;
	}

public:
	Property<int64_t, DisplayNameAnnotation, LinkStartAnnotation> ticker_us_{false, {"Ticker Output"}, {}};
};


class Timer : public BaseObject {
public:
	static inline const TypeDescriptor typeDescription = {"Timer", true};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	Timer(Timer const& other) : BaseObject(other), inputs_(other.inputs_), outputs_(other.outputs_) {
		fillPropertyDescription();
	}

	Timer(std::string name = std::string(), std::string id = std::string())
		: BaseObject(name, id) {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("inputs", &inputs_);
		properties_.emplace_back("outputs", &outputs_);
	}

	void onAfterValueChanged(BaseContext& context, ValueHandle const& value) override;


	// use single struct instead
	Property<TimerInput, DisplayNameAnnotation> inputs_{{}, DisplayNameAnnotation("Inputs")};
	Property<TimerOutput, DisplayNameAnnotation> outputs_{{}, DisplayNameAnnotation("Outputs")};

};

using STimer = std::shared_ptr<Timer>;

}
