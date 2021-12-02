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

#include "core/EditorObject.h"
#include "data_storage/BasicTypes.h"

namespace raco::core {

class ProjectSettings : public EditorObject {
public:
	static inline const TypeDescriptor typeDescription{"ProjectSettings", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	ProjectSettings(ProjectSettings const& other) : EditorObject(other), sceneId_(other.sceneId_), viewport_(other.viewport_), backgroundColor_(other.backgroundColor_), runTimer_(other.runTimer_), enableTimerFlag_(other.enableTimerFlag_) {
		fillPropertyDescription();
	}

	ProjectSettings(const std::string& name, const std::string& id = std::string()) : EditorObject(name, id) {
		fillPropertyDescription();
		backgroundColor_.asVec4f().w = 1.0;
	}

	ProjectSettings() : ProjectSettings("Main") {}

	void fillPropertyDescription() {
		properties_.emplace_back("sceneId", &sceneId_);
		properties_.emplace_back("viewport", &viewport_);
		properties_.emplace_back("backgroundColor", &backgroundColor_);
		properties_.emplace_back("enableTimerFlag", &enableTimerFlag_);
		properties_.emplace_back("runTimer", &runTimer_);
	}

	Property<int, DisplayNameAnnotation, RangeAnnotation<int>> sceneId_{123u, DisplayNameAnnotation("Scene Id"), {1, 1024}};
	Property<Vec2i, DisplayNameAnnotation> viewport_{{{1440, 720}, 0, 4096}, {"Display Size"}};
	Property<Vec4f, DisplayNameAnnotation> backgroundColor_{{}, {"Display Background Color"}};

	// Properties related to timer running hack - remove these properties and all related code when proper animations have been implemented
	Property<bool, HiddenProperty> enableTimerFlag_{false, HiddenProperty()};
	Property<bool, HiddenProperty> runTimer_{false, HiddenProperty()};
};

}  // namespace raco::core
