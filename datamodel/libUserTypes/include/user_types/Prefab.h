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

class Prefab : public BaseObject {
public:
	static inline const TypeDescriptor typeDescription = { "Prefab", false };
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	Prefab(Prefab const& other) : BaseObject(other) {
		fillPropertyDescription();
	}

	Prefab(std::string name = std::string(), std::string id = std::string())
		: BaseObject(name, id)
	{
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
	}


	void onBeforeRemoveReferenceToThis(ValueHandle const& sourceReferenceProperty) const override;
	void onAfterAddReferenceToThis(ValueHandle const& sourceReferenceProperty) const override;

	// volatile
	mutable std::set<WEditorObject, std::owner_less<WEditorObject>> instances_;
};

using SPrefab = std::shared_ptr<Prefab>;

}