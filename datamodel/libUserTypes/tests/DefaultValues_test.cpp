/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include <gtest/gtest.h>

#include "user_types/DefaultValues.h"
#include <ramses/client/Appearance.h>
#include <ramses/client/Effect.h>
#include <ramses/client/EffectDescription.h>
#include <ramses/client/RamsesClient.h>
#include <ramses/client/Scene.h>
#include <ramses/framework/RamsesFramework.h>

constexpr const char* emptyVertexShader =
	"#version 300 es\n\
		precision mediump float;\n\
		void main() {}";

constexpr const char* emptyFragmentShader =
	"#version 300 es\n\
		precision mediump float;\n\
		void main() {}";

ramses::Appearance* createAppearance(ramses::Scene* scene) {
	ramses::EffectDescription desc{};
	desc.setVertexShader(emptyVertexShader);
	desc.setFragmentShader(emptyFragmentShader);
	ramses::Effect* effect = scene->createEffect(desc);
	return scene->createAppearance(*effect);
}

TEST(DefaultValues, Appearance) {
	ramses::RamsesFramework ramsesFramework{ramses::RamsesFrameworkConfig{ramses::EFeatureLevel::EFeatureLevel_01}};
	ramses::RamsesClient& client = *ramsesFramework.createClient("client");
	ramses::Scene* scene = client.createScene(ramses::sceneId_t(123u), "scene");
	auto* appearance = createAppearance(scene);

	{
		ramses::EBlendFactor srcColor, destColor, srcAlpha, destAlpha;
		appearance->getBlendingFactors(srcColor, destColor, srcAlpha, destAlpha);

		using namespace raco::user_types;
		ASSERT_EQ(static_cast<int>(srcColor), DEFAULT_VALUE_MATERIAL_BLEND_FACTOR_SRC_COLOR);
		ASSERT_EQ(static_cast<int>(destColor), DEFAULT_VALUE_MATERIAL_BLEND_FACTOR_DEST_COLOR);
		ASSERT_EQ(static_cast<int>(srcAlpha), DEFAULT_VALUE_MATERIAL_BLEND_FACTOR_SRC_ALPHA);
		ASSERT_EQ(static_cast<int>(destAlpha), DEFAULT_VALUE_MATERIAL_BLEND_FACTOR_DEST_ALPHA);
	}
	{
		ramses::EBlendOperation colorOp, alphaOp;
		appearance->getBlendingOperations(colorOp, alphaOp);

		using namespace raco::user_types;
		ASSERT_EQ(static_cast<int>(colorOp), DEFAULT_VALUE_MATERIAL_BLEND_OPERATION_COLOR);
		ASSERT_EQ(static_cast<int>(alphaOp), DEFAULT_VALUE_MATERIAL_BLEND_OPERATION_ALPHA);
	}
	{
		ramses::ECullMode cullMode;
		appearance->getCullingMode(cullMode);

		using namespace raco::user_types;
		ASSERT_EQ(static_cast<int>(cullMode), DEFAULT_VALUE_MATERIAL_CULL_MODE);
	}
	{
		ramses::EDepthFunc depthFunc;
		appearance->getDepthFunction(depthFunc);

		using namespace raco::user_types;
		ASSERT_EQ(static_cast<int>(depthFunc), DEFAULT_VALUE_MATERIAL_DEPTH_FUNCTION);
	}
}