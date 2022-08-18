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

#include "core/BasicAnnotations.h"

#include "user_types/Node.h"
#include "user_types/Prefab.h"

namespace raco::user_types {

class PrefabInstance;
using SPrefabInstance = std::shared_ptr<PrefabInstance>;

class PrefabInstance : public Node {
public:
	static inline const TypeDescriptor typeDescription = { "PrefabInstance", false };
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	PrefabInstance(PrefabInstance const& other) : Node(other), template_(other.template_) {
		fillPropertyDescription();
	}

	PrefabInstance(std::string name = std::string(), std::string id = std::string())
		: Node(name, id)
	{
				fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("template", &template_);
	}

	Property<SPrefab, DisplayNameAnnotation> template_{nullptr, DisplayNameAnnotation("Prefab Template")};

	static std::string mapObjectIDToInstance(SEditorObject obj, SPrefab prefab, SPrefabInstance instance);
	static std::string mapObjectIDFromInstance(SEditorObject obj, SPrefab prefab, SPrefabInstance instance);
};

}