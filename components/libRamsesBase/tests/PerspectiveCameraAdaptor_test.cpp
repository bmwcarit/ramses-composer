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
#include "ramses_adaptor/PerspectiveCameraAdaptor.h"
#include "user_types/PerspectiveCamera.h"

using namespace raco;
using raco::core::ValueHandle;
using raco::user_types::LuaScript;
using raco::user_types::PerspectiveCamera;

class PerspectiveCameraAdaptorTest : public RamsesBaseFixture<> {};

TEST_F(PerspectiveCameraAdaptorTest, quaternion_link_camera_still_active) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");

	std::string uriPath{(test_path() / "script.lua").string()};
	raco::utils::file::write(uriPath, R"(
function interface()
	IN.in_value = VEC4F
	OUT.out_value = VEC4F
end

function run()
	OUT.out_value = IN.in_value
end

)");
	context.set({luaScript, {"uri"}}, uriPath);
	context.set({luaScript, {"luaInputs", "in_value", "x"}}, 5.0f);
	dispatch();

	auto camera = commandInterface.createObject(PerspectiveCamera::typeDescription.typeName, "Camera");
	dispatch();

	commandInterface.addLink({luaScript, {"luaOutputs", "out_value"}}, {camera, {"rotation"}});
	dispatch();

	commandInterface.set({camera, {"frustum", "aspectRatio"}}, 7.0);
	dispatch();

	auto cameras{select<ramses::PerspectiveCamera>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_PerspectiveCamera)};
	ASSERT_EQ(cameras.size(), 1);
	ASSERT_FLOAT_EQ(cameras.front()->getAspectRatio(), 7.0F);
}
