/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include <gtest/gtest.h>

#include "RamsesBaseFixture.h"
#include "ramses_adaptor/MaterialAdaptor.h"

class MaterialAdaptorTest : public RamsesBaseFixture<> {};

TEST_F(MaterialAdaptorTest, context_scene_effect_name_change) {
	auto isNameInArray = [](const char* name, const std::vector<ramses::Effect*>& arrayOfEffects) {
		return std::find_if(arrayOfEffects.begin(), arrayOfEffects.end(), [name](ramses::Effect* effect) {
			return std::strcmp(effect->getName(), name) == 0;
		}) != arrayOfEffects.end();
	};

	auto node = context.createObject(raco::user_types::Material::typeDescription.typeName, "Material Name");

	dispatch();

	auto effects{select<ramses::Effect>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_Effect)};

	// we need to consider default effects of a new scene
	EXPECT_EQ(effects.size(), 1);
	ASSERT_TRUE(isNameInArray("Material Name", effects));

	context.set({node, {"objectName"}}, std::string("Changed"));
	dispatch();

	effects = select<ramses::Effect>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_Effect);
	EXPECT_STREQ("Changed", effects[0]->getName());
	ASSERT_TRUE(isNameInArray("Changed", effects));
}
