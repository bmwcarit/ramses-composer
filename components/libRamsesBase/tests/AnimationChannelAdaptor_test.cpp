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
#include "ramses_adaptor/AnimationAdaptor.h"
#include "ramses_adaptor/AnimationChannelAdaptor.h"

using namespace raco::user_types;

class AnimationChannelAdaptorTest : public RamsesBaseFixture<> {};

TEST_F(AnimationChannelAdaptorTest, defaultConstruction) {
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");

	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::DataArray>().size(), 0);
}

TEST_F(AnimationChannelAdaptorTest, validAnim_validSampler_dataArrays) {
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");

	dispatch();

	std::string uriPath{(cwd_path() / "meshes" / "CesiumMilkTruck" / "CesiumMilkTruck.gltf").string()};
	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::uri_}, uriPath);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::DataArray>().size(), 2);
	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name.keyframes"), nullptr);
	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name.timestamps"), nullptr);
}

TEST_F(AnimationChannelAdaptorTest, validAnim_validSampler_dataArrays_rename_obj) {
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");

	dispatch();

	std::string uriPath{(cwd_path() / "meshes" / "CesiumMilkTruck" / "CesiumMilkTruck.gltf").string()};
	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::uri_}, uriPath);
	dispatch();

	context.set({animChannel, {"objectName"}}, std::string("Changed"));
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::DataArray>().size(), 2);
	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Changed.keyframes"), nullptr);
	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Changed.timestamps"), nullptr);
}

TEST_F(AnimationChannelAdaptorTest, validAnim_validSampler_animAssigned_dataArrays_rename_obj) {
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");

	dispatch();

	std::string uriPath{(cwd_path() / "meshes" / "CesiumMilkTruck" / "CesiumMilkTruck.gltf").string()};
	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::uri_}, uriPath);
	dispatch();

	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	context.set({anim, {"animationChannels", "Channel 0"}}, animChannel);
	dispatch();

	context.set({animChannel, {"objectName"}}, std::string("Changed"));
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::DataArray>().size(), 2);
	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Changed.keyframes"), nullptr);
	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Changed.timestamps"), nullptr);
}

TEST_F(AnimationChannelAdaptorTest, validAnim_invalidSampler_noDataArrays) {
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");

	dispatch();

	std::string uriPath{(cwd_path() / "meshes" / "CesiumMilkTruck" / "CesiumMilkTruck.gltf").string()};
	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::uri_}, uriPath);
	dispatch();

	context.set({animChannel, &raco::user_types::AnimationChannel::samplerIndex_}, -1);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::DataArray>().size(), 0);
}

TEST_F(AnimationChannelAdaptorTest, validAnim_invalidSampler_animAssigned_defaultDataArrays) {
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");

	dispatch();

	std::string uriPath{(cwd_path() / "meshes" / "CesiumMilkTruck" / "CesiumMilkTruck.gltf").string()};
	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::uri_}, uriPath);
	dispatch();

	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	context.set({anim, {"animationChannels", "Channel 0"}}, animChannel);
	dispatch();

	context.set({animChannel, &raco::user_types::AnimationChannel::samplerIndex_}, -1);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::DataArray>().size(), 2);
	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), raco::ramses_adaptor::defaultAnimationChannelKeyframesName), nullptr);
	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), raco::ramses_adaptor::defaultAnimationChannelTimestampsName), nullptr);
}

TEST_F(AnimationChannelAdaptorTest, invalidAnim_invalidSampler_noDataArrays) {
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");

	dispatch();

	std::string uriPath{(cwd_path() / "meshes" / "CesiumMilkTruck" / "CesiumMilkTruck.gltf").string()};
	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::uri_}, uriPath);
	dispatch();

	context.set({animChannel, &raco::user_types::AnimationChannel::animationIndex_}, -1);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::DataArray>().size(), 0);
}

TEST_F(AnimationChannelAdaptorTest, noAnim_noDataArrays) {
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");

	dispatch();

	std::string uriPath{(cwd_path() / "meshes" / "Duck.glb").string()};
	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::uri_}, uriPath);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::DataArray>().size(), 0);
}

TEST_F(AnimationChannelAdaptorTest, interpolationTest_dynamicDataArrays) {
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");

	dispatch();

	std::string uriPath{(cwd_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string()};
	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::uri_}, uriPath);
	dispatch();

	context.set({animChannel, &raco::user_types::AnimationChannel::animationIndex_}, 4);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::DataArray>().size(), 4);
	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name.keyframes"), nullptr);
	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name.timestamps"), nullptr);
	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name.tangentIn"), nullptr);
	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name.tangentOut"), nullptr);

	context.set({animChannel, &raco::user_types::AnimationChannel::animationIndex_}, 3);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::DataArray>().size(), 2);
	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name.keyframes"), nullptr);
	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name.timestamps"), nullptr);
	ASSERT_EQ(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name.tangentIn"), nullptr);
	ASSERT_EQ(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name.tangentOut"), nullptr);
}


TEST_F(AnimationChannelAdaptorTest, mesh_baked_flag_true_anim_data_gets_imported) {
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");
	auto mesh = context.createObject(Mesh::typeDescription.typeName, "Mesh");
	commandInterface.set({mesh, {"bakeMeshes"}}, true);
	dispatch();

	std::string uriPath{(cwd_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string()};
	commandInterface.set({mesh, &raco::user_types::AnimationChannel::uri_}, uriPath);
	dispatch();

	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::uri_}, uriPath);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::DataArray>().size(), 2);
	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name.keyframes"), nullptr);
	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name.timestamps"), nullptr);
}