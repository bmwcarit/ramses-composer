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
#include "ramses_adaptor/utilities.h"
#include "user_types/AnimationChannel.h"
#include "user_types/Prefab.h"
#include "user_types/PrefabInstance.h"

using namespace raco::user_types;

class AnimationAdaptorTest : public RamsesBaseFixture<> {};

TEST_F(AnimationAdaptorTest, defaultConstruction) {
	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");

	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::AnimationNode>().size(), 1);

	auto rlogicID = sceneContext.logicEngine().getCollection<rlogic::AnimationNode>().begin()->getUserId();
	ASSERT_EQ(rlogicID, (std::pair<uint64_t, uint64_t>{0, 0}));
	ASSERT_STREQ(sceneContext.logicEngine().getCollection<rlogic::AnimationNode>().begin()->getName().data(), raco::ramses_adaptor::defaultAnimationName);
}

TEST_F(AnimationAdaptorTest, defaultConstruction_2_empty_anims) {
	context.createObject(Animation::typeDescription.typeName, "Animation Name");
	context.createObject(Animation::typeDescription.typeName, "Animation Name 2");

	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::AnimationNode>().size(), 1);
	ASSERT_STREQ(sceneContext.logicEngine().getCollection<rlogic::AnimationNode>().begin()->getName().data(), raco::ramses_adaptor::defaultAnimationName);
}

TEST_F(AnimationAdaptorTest, animNode_Creation) {
	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");

	dispatch();

	std::string uriPath{(test_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string()};
	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::uri_}, uriPath);
	dispatch();

	context.set({animChannel, &raco::user_types::AnimationChannel::animationIndex_}, 4);
	context.set({anim, {"animationChannels", "Channel 0"}}, animChannel);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::AnimationNode>().size(), 1);
	ASSERT_NE(select<rlogic::AnimationNode>(sceneContext.logicEngine(), "Animation Name"), nullptr);
}

TEST_F(AnimationAdaptorTest, animNode_Deletion) {
	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");

	dispatch();

	std::string uriPath{(test_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string()};
	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::uri_}, uriPath);
	dispatch();

	context.set({animChannel, &raco::user_types::AnimationChannel::animationIndex_}, 4);
	context.set({anim, {"animationChannels", "Channel 0"}}, animChannel);
	dispatch();

	context.set({anim, {"animationChannels", "Channel 0"}}, SEditorObject{});
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::AnimationNode>().size(), 1);
	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::AnimationNode>().begin()->getUserId(), (std::pair<uint64_t, uint64_t>(0, 0)));
	ASSERT_STREQ(sceneContext.logicEngine().getCollection<rlogic::AnimationNode>().begin()->getName().data(), raco::ramses_adaptor::defaultAnimationName);
}

TEST_F(AnimationAdaptorTest, animNode_animName) {
	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");

	dispatch();

	std::string uriPath{(test_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string()};
	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::uri_}, uriPath);
	dispatch();

	context.set({animChannel, &raco::user_types::AnimationChannel::animationIndex_}, 4);
	context.set({anim, {"animationChannels", "Channel 0"}}, animChannel);
	dispatch();

	context.set({anim, {"objectName"}}, "Changed");
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::AnimationNode>().size(), 1);
	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::AnimationNode>().begin()->getUserId(), anim->objectIDAsRamsesLogicID());
	ASSERT_NE(select<rlogic::AnimationNode>(sceneContext.logicEngine(), "Changed"), nullptr);
}

TEST_F(AnimationAdaptorTest, prefab_noAnimNode) {
	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");

	dispatch();

	std::string uriPath{(test_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string()};
	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::uri_}, uriPath);
	dispatch();

	context.set({animChannel, &raco::user_types::AnimationChannel::animationIndex_}, 4);
	context.set({anim, {"animationChannels", "Channel 0"}}, animChannel);
	dispatch();

	context.set({anim, {"objectName"}}, "Changed");
	dispatch();

	auto prefab = create<Prefab>("Prefab");
	dispatch();

	commandInterface.moveScenegraphChildren({anim}, prefab);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::AnimationNode>().size(), 0);
}

TEST_F(AnimationAdaptorTest, animNode_multiple_channels) {
	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	auto animChannel1 = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name 1");
	auto animChannel2 = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name 2");
	auto animChannel3 = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name 3");

	dispatch();

	std::string uriPath{(test_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string()};
	commandInterface.set({animChannel1, &raco::user_types::AnimationChannel::uri_}, uriPath);
	commandInterface.set({animChannel2, &raco::user_types::AnimationChannel::uri_}, uriPath);
	commandInterface.set({animChannel3, &raco::user_types::AnimationChannel::uri_}, uriPath);
	dispatch();

	context.set({animChannel1, &raco::user_types::AnimationChannel::animationIndex_}, 4);
	context.set({animChannel2, &raco::user_types::AnimationChannel::animationIndex_}, 2);
	context.set({animChannel3, &raco::user_types::AnimationChannel::animationIndex_}, 3);

	context.set({anim, {"animationChannels", "Channel 0"}}, animChannel1);
	dispatch();

	context.set({anim, {"animationChannels", "Channel 1"}}, animChannel2);
	dispatch();

	context.set({anim, {"animationChannels", "Channel 2"}}, animChannel3);
	dispatch();

	ASSERT_EQ(anim->get("animationOutputs")->asTable().size(), 3);
	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name 1.keyframes"), nullptr);
	ASSERT_EQ(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name 1.keyframes")->getUserId(), animChannel1->objectIDAsRamsesLogicID());
	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name 1.timestamps"), nullptr);
	ASSERT_EQ(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name 1.timestamps")->getUserId(), animChannel1->objectIDAsRamsesLogicID());

	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name 2.keyframes"), nullptr);
	ASSERT_EQ(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name 2.keyframes")->getUserId(), animChannel2->objectIDAsRamsesLogicID());
	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name 2.timestamps"), nullptr);
	ASSERT_EQ(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name 2.timestamps")->getUserId(), animChannel2->objectIDAsRamsesLogicID());

	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name 3.keyframes"), nullptr);
	ASSERT_EQ(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name 3.keyframes")->getUserId(), animChannel3->objectIDAsRamsesLogicID());
	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name 3.timestamps"), nullptr);
	ASSERT_EQ(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name 3.timestamps")->getUserId(), animChannel3->objectIDAsRamsesLogicID());
}

TEST_F(AnimationAdaptorTest, afterSync_dataArrays_get_cleaned_up) {
	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");

	dispatch();

	std::string uriPath{(test_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string()};
	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::uri_}, uriPath);
	dispatch();

	context.set({animChannel, &raco::user_types::AnimationChannel::animationIndex_}, 4);
	context.set({anim, {"animationChannels", "Channel 0"}}, animChannel);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::DataArray>().size(), 4);
	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name.keyframes"), nullptr);
	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name.timestamps"), nullptr);
	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name.tangentIn"), nullptr);
	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name.tangentOut"), nullptr);

	// moving from cubic to non-cubic anim: tangent data arrays should be deallocated
	context.set({animChannel, &raco::user_types::AnimationChannel::animationIndex_}, 3);

	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::DataArray>().size(), 2);
	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name.keyframes"), nullptr);
	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name.timestamps"), nullptr);
	ASSERT_EQ(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name.tangentIn"), nullptr);
	ASSERT_EQ(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name.tangentOut"), nullptr);
}

TEST_F(AnimationAdaptorTest, link_with_luascript_output) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");

	std::string uriPath{(test_path() / "script.lua").string()};
	raco::utils::file::write(uriPath, R"(
function interface(IN,OUT)
	IN.in_value = Type:Float()
	OUT.out_value = Type:Float()
end

function run(IN,OUT)
	OUT.out_value = IN.in_value
end

)");
	context.set({luaScript, &raco::user_types::LuaScript::uri_}, uriPath);
	dispatch();

	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	dispatch();

	commandInterface.addLink({luaScript, {"luaOutputs", "out_value"}}, {anim, &raco::user_types::Animation::progress_});
	dispatch();

	context.set({luaScript, {"luaInputs", "in_value"}}, 2.0);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::AnimationNode>().begin()->getInputs()->getChild("progress")->get<float>(), 2.0);
}

TEST_F(AnimationAdaptorTest, link_with_meshNode_mesh_changed) {
	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");
	auto mesh = context.createObject(Mesh::typeDescription.typeName, "Mesh");
	auto meshNode = context.createObject(MeshNode::typeDescription.typeName, "MeshNode");
	dispatch();

	std::string uriPath{(test_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string()};
	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::uri_}, uriPath);
	commandInterface.set({mesh, &raco::user_types::Mesh::uri_}, uriPath);
	dispatch();

	commandInterface.set({anim, {"animationChannels", "Channel 0"}}, animChannel);
	commandInterface.set({mesh, &raco::user_types::Mesh::bakeMeshes_}, false);
	commandInterface.set({meshNode, &raco::user_types::MeshNode::mesh_}, mesh);
	dispatch();

	commandInterface.addLink({anim, {"animationOutputs", "Ch0.Animation Sampler Name"}}, {meshNode, {"translation"}});
	dispatch();

	commandInterface.set({mesh, &raco::user_types::Mesh::uri_}, (test_path() / "meshes" / "CesiumMilkTruck" / "CesiumMilkTruck.gltf").string());
	dispatch();

	ASSERT_EQ(commandInterface.project()->links().size(), 1);
	ASSERT_TRUE(commandInterface.project()->links().front()->isValid());
}

TEST_F(AnimationAdaptorTest, link_with_meshNode_submesh_index_changed) {
	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");
	auto mesh = context.createObject(Mesh::typeDescription.typeName, "Mesh");
	auto meshNode = context.createObject(MeshNode::typeDescription.typeName, "MeshNode");
	dispatch();

	std::string uriPath{(test_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string()};
	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::uri_}, uriPath);
	commandInterface.set({mesh, &raco::user_types::Mesh::uri_}, uriPath);
	dispatch();

	commandInterface.set({anim, {"animationChannels", "Channel 0"}}, animChannel);
	commandInterface.set({mesh, &raco::user_types::Mesh::bakeMeshes_}, false);
	commandInterface.set({meshNode, &raco::user_types::MeshNode::mesh_}, mesh);
	dispatch();

	commandInterface.addLink({anim, {"animationOutputs", "Ch0.Animation Sampler Name"}}, {meshNode, {"translation"}});
	dispatch();

	commandInterface.set({mesh, {"meshIndex"}}, 1);
	dispatch();

	ASSERT_EQ(commandInterface.project()->links().size(), 1);
	ASSERT_TRUE(commandInterface.project()->links().front()->isValid());
}

TEST_F(AnimationAdaptorTest, link_with_meshNode_channel_data_changed_valid_type) {
	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");
	auto mesh = context.createObject(Mesh::typeDescription.typeName, "Mesh");
	auto meshNode = context.createObject(MeshNode::typeDescription.typeName, "MeshNode");
	dispatch();

	std::string uriPath{(test_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string()};
	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::uri_}, uriPath);
	commandInterface.set({mesh, &raco::user_types::Mesh::uri_}, uriPath);
	dispatch();

	commandInterface.set({anim, {"animationChannels", "Channel 0"}}, animChannel);
	commandInterface.set({mesh, &raco::user_types::Mesh::bakeMeshes_}, false);
	commandInterface.set({meshNode, &raco::user_types::MeshNode::mesh_}, mesh);
	dispatch();

	commandInterface.addLink({anim, {"animationOutputs", "Ch0.Animation Sampler Name"}}, {meshNode, {"translation"}});
	dispatch();

	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::animationIndex_}, 1);
	dispatch();

	ASSERT_EQ(commandInterface.project()->links().size(), 1);
	ASSERT_TRUE(commandInterface.project()->links().front()->isValid());
}

TEST_F(AnimationAdaptorTest, link_with_meshNode_channel_data_changed_invalid_type) {
	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");
	auto mesh = context.createObject(Mesh::typeDescription.typeName, "Mesh");
	auto meshNode = context.createObject(MeshNode::typeDescription.typeName, "MeshNode");
	dispatch();

	std::string uriPath{(test_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string()};
	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::uri_}, uriPath);
	commandInterface.set({mesh, &raco::user_types::Mesh::uri_}, uriPath);
	dispatch();

	commandInterface.set({anim, {"animationChannels", "Channel 0"}}, animChannel);
	commandInterface.set({mesh, &raco::user_types::Mesh::bakeMeshes_}, false);
	commandInterface.set({meshNode, &raco::user_types::MeshNode::mesh_}, mesh);
	dispatch();

	commandInterface.addLink({anim, {"animationOutputs", "Ch0.Animation Sampler Name"}}, {meshNode, {"translation"}});
	dispatch();

	// changing from vec3f output to vec4f output
	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::animationIndex_}, 3);
	dispatch();

	ASSERT_EQ(commandInterface.project()->links().size(), 1);
	ASSERT_FALSE(commandInterface.project()->links().front()->isValid());
}

TEST_F(AnimationAdaptorTest, link_with_meshNode_channel_removed) {
	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");
	auto mesh = context.createObject(Mesh::typeDescription.typeName, "Mesh");
	auto meshNode = context.createObject(MeshNode::typeDescription.typeName, "MeshNode");
	dispatch();

	std::string uriPath{(test_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string()};
	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::uri_}, uriPath);
	commandInterface.set({mesh, &raco::user_types::Mesh::uri_}, uriPath);
	dispatch();

	commandInterface.set({anim, {"animationChannels", "Channel 0"}}, animChannel);
	commandInterface.set({mesh, &raco::user_types::Mesh::bakeMeshes_}, false);
	commandInterface.set({meshNode, &raco::user_types::MeshNode::mesh_}, mesh);
	dispatch();

	commandInterface.addLink({anim, {"animationOutputs", "Ch0.Animation Sampler Name"}}, {meshNode, {"translation"}});
	dispatch();

	commandInterface.set({anim, {"animationChannels", "Channel 0"}}, SEditorObject());
	dispatch();

	ASSERT_EQ(commandInterface.project()->links().size(), 1);
	ASSERT_FALSE(commandInterface.project()->links().front()->isValid());

	commandInterface.set({anim, {"animationChannels", "Channel 0"}}, animChannel);
	dispatch();

	ASSERT_EQ(commandInterface.project()->links().size(), 1);
	ASSERT_TRUE(commandInterface.project()->links().front()->isValid());
}

TEST_F(AnimationAdaptorTest, anim_in_prefab_prefabinstance_link_inside_prefabinstance) {
	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");
	auto mesh = context.createObject(Mesh::typeDescription.typeName, "Mesh");
	auto meshNode = context.createObject(MeshNode::typeDescription.typeName, "MeshNode");
	auto prefab = context.createObject(Prefab::typeDescription.typeName, "Prefab");
	auto prefabInstance = context.createObject(PrefabInstance::typeDescription.typeName, "PrefabInstance");
	dispatch();

	std::string uriPath{(test_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string()};
	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::uri_}, uriPath);
	commandInterface.set({mesh, &raco::user_types::Mesh::uri_}, uriPath);
	dispatch();

	commandInterface.moveScenegraphChildren({anim}, prefab);
	commandInterface.moveScenegraphChildren({meshNode}, prefab);
	commandInterface.set({anim, {"animationChannels", "Channel 1"}}, animChannel);
	commandInterface.set({mesh, &raco::user_types::Mesh::bakeMeshes_}, false);
	commandInterface.set({meshNode, &raco::user_types::MeshNode::mesh_}, mesh);

	commandInterface.addLink({anim, {"animationOutputs", "Ch1.Animation Sampler Name"}}, {meshNode, {"translation"}});
	dispatch();

	ASSERT_EQ(commandInterface.project()->links().size(), 1);
	ASSERT_TRUE(commandInterface.project()->links().front()->isValid());

	commandInterface.set({prefabInstance, {"template"}}, prefab);
	dispatch();

	ASSERT_EQ(commandInterface.project()->links().size(), 2);
	ASSERT_TRUE(commandInterface.project()->links().front()->isValid());
	ASSERT_TRUE(commandInterface.project()->links().back()->isValid());

	commandInterface.set({prefabInstance, {"template"}}, SEditorObject());
	dispatch();

	ASSERT_EQ(commandInterface.project()->links().size(), 1);
	ASSERT_TRUE(commandInterface.project()->links().front()->isValid());
}
