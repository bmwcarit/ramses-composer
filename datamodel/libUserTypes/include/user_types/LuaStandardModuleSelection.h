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

#include "user_types/BaseObject.h"

namespace raco::user_types {

class LuaStandardModuleSelection : public StructBase {
public:
	static inline const TypeDescriptor typeDescription = {"LuaStandardModuleSelection", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return true;
	}

	LuaStandardModuleSelection() : StructBase(getProperties()) {}

	LuaStandardModuleSelection(const LuaStandardModuleSelection& other, std::function<SEditorObject(SEditorObject)>* translateRef = nullptr)
		: StructBase(getProperties()),
		  base_(other.base_),
		  string_(other.string_),
		  table_(other.table_),
		  math_(other.math_),
		  debug_(other.debug_) {}

	LuaStandardModuleSelection& operator=(const LuaStandardModuleSelection& other) {
		base_ = other.base_;
		string_ = other.string_;
		table_ = other.table_;
		math_ = other.math_;
		debug_ = other.debug_;
		return *this;
	}

	void copyAnnotationData(const LuaStandardModuleSelection& other) {
		base_.copyAnnotationData(other.base_);
		string_.copyAnnotationData(other.string_);
		table_.copyAnnotationData(other.table_);
		math_.copyAnnotationData(other.math_);
		debug_.copyAnnotationData(other.debug_);
	}

	std::vector<std::pair<std::string, ValueBase*>> getProperties() {
		return {
			{"base", &base_},
			{"string", &string_},
			{"table", &table_},
			{"math", &math_},
			{"debug", &debug_}};
	}

	std::vector<std::string> activeModules() const {
		std::vector<std::string> result;
		for (size_t index = 0; index < size(); index++) {
			if (get(index)->as<bool>()) {
				result.emplace_back(name(index));
			}
		}
		return result;
	}

	Property<bool, DisplayNameAnnotation> base_{true, {"Base"}};
	Property<bool, DisplayNameAnnotation> string_{true, {"String"}};
	Property<bool, DisplayNameAnnotation> table_{true, {"Table"}};
	Property<bool, DisplayNameAnnotation> math_{true, {"Math"}};
	Property<bool, DisplayNameAnnotation> debug_{true, {"Debug"}};
};

}  // namespace raco::user_types