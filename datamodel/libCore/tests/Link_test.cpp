/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/Context.h"
#include "core/Handles.h"
#include "core/PropertyDescriptor.h"
#include "core/MeshCacheInterface.h"
#include "core/Project.h"
#include "core/Queries.h"
#include "ramses_base/HeadlessEngineBackend.h"
#include "testing/RacoBaseTest.h"
#include "testing/TestEnvironmentCore.h"
#include "user_types/UserObjectFactory.h"

#include "user_types/Mesh.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include "user_types/Prefab.h"
#include "user_types/PrefabInstance.h"
#include "user_types/PerspectiveCamera.h"
#include "user_types/OrthographicCamera.h"

#include "utils/FileUtils.h"

#include "gtest/gtest.h"

#include <algorithm>
#include <functional>

using namespace raco::core;
using namespace raco::user_types;

class LinkTest : public TestEnvironmentCore {
public:
	void change_uri(SEditorObject obj, const std::string& newvalue) {
		commandInterface.set({obj, {"uri"}}, (cwd_path() / newvalue).string());
	}

};

TEST_F(LinkTest, fail_create_invalid_handles) {
	auto start = create_lua("start", "scripts/types-scalar.lua");
	auto end = create_lua("end", "scripts/types-scalar.lua");

	EXPECT_EQ(nullptr, commandInterface.addLink(ValueHandle{start, {"luaOutputs", "ofloat"}}, ValueHandle{end, {"luaInputs", "INVALID"}}));
	EXPECT_EQ(nullptr, commandInterface.addLink(ValueHandle{start, {"luaOutputs", "INVALID"}}, ValueHandle{end, {"luaInputs", "float"}}));

	EXPECT_EQ(nullptr, commandInterface.addLink(ValueHandle{start, {"luaOutputs", "ofloat"}}, ValueHandle{end, { "INVALID"}}));
	EXPECT_EQ(nullptr, commandInterface.addLink(ValueHandle{start, {"INVALID"}}, ValueHandle{end, {"luaInputs", "float"}}));

	EXPECT_EQ(nullptr, commandInterface.addLink(ValueHandle{start, {"luaOutputs", "ofloat"}}, ValueHandle{end}));
	EXPECT_EQ(nullptr, commandInterface.addLink(ValueHandle{start}, ValueHandle{end, {"luaInputs", "float"}}));

	EXPECT_EQ(nullptr, commandInterface.addLink(ValueHandle{start, {"luaOutputs", "ofloat"}}, ValueHandle{}));
	EXPECT_EQ(nullptr, commandInterface.addLink(ValueHandle{}, ValueHandle{end, {"luaInputs", "float"}}));
}

TEST_F(LinkTest, lua_lua_scalar) {
	auto start = create_lua("start", "scripts/types-scalar.lua");
	auto end = create_lua("end", "scripts/types-scalar.lua");

	checkUndoRedoMultiStep<2>(
		{[this, start, end]() { commandInterface.addLink(ValueHandle{start, {"luaOutputs", "ofloat"}}, ValueHandle{end, {"luaInputs", "float"}}); },
			[this, end]() { commandInterface.removeLink({end, {"luaInputs", "float"}}); }},
		{
			[this]() {
				EXPECT_EQ(project.links().size(), 0);
			},
			[this, start, end]() {
				std::vector<Link> refLinks{{{{start, {"luaOutputs", "ofloat"}}, {end, {"luaInputs", "float"}}}}};
				checkLinks(refLinks);
			},
			[this]() {
				EXPECT_EQ(project.links().size(), 0);
			},
		});
}

TEST_F(LinkTest, lua_lua_recorder_merge) {
	auto start = create_lua("start", "scripts/types-scalar.lua");
	auto end = create_lua("end", "scripts/types-scalar.lua");

	EXPECT_EQ(project.links().size(), 0);
	commandInterface.addLink(ValueHandle{start, {"luaOutputs", "ofloat"}}, ValueHandle{end, {"luaInputs", "float"}});

	std::vector<Link> refLinks{{{{start, {"luaOutputs", "ofloat"}}, {end, {"luaInputs", "float"}}}}};
	checkLinks(refLinks);
	EXPECT_EQ(recorder.getAddedLinks().size(), 1);

	commandInterface.removeLink({end, {"luaInputs", "float"}});

	EXPECT_EQ(project.links().size(), 0);
	EXPECT_EQ(recorder.getAddedLinks().size(), 0);
	EXPECT_EQ(recorder.getRemovedLinks().size(), 0);
}

TEST_F(LinkTest, lua_lua_struct_simple) {
	auto start = create_lua("start", "scripts/struct-simple.lua");
	auto end = create_lua("end", "scripts/struct-simple.lua");

	checkUndoRedoMultiStep<2>(
		{[this, start, end]() { commandInterface.addLink(ValueHandle{start, {"luaOutputs", "s", "float"}}, ValueHandle{end, {"luaInputs", "s", "float"}}); },
			[this, start, end]() { commandInterface.addLink(ValueHandle{start, {"luaOutputs", "s"}}, ValueHandle{end, {"luaInputs", "s"}}); }},
		{[this]() {
			 EXPECT_EQ(project.links().size(), 0);
		 },
			[this, start, end]() {
				std::vector<Link> refLinks{{{{start, {"luaOutputs", "s", "float"}}, {end, {"luaInputs", "s", "float"}}}}};
				checkLinks(refLinks);
			},
			[this, start, end]() {
				std::vector<Link> refLinks{{{{start, {"luaOutputs", "s"}}, {end, {"luaInputs", "s"}}}}};
				checkLinks(refLinks);
			}});
}

TEST_F(LinkTest, lua_persp_camera_struct_viewport_frustum) {
	auto lua = create_lua("start", "scripts/camera-control.lua");
	auto camera = create<PerspectiveCamera>("camera");

	checkUndoRedoMultiStep<4>(
		{[this, lua, camera]() {
			 commandInterface.addLink(ValueHandle{lua, {"luaOutputs", "viewport", "offsetX"}}, ValueHandle{camera, {"viewport", "offsetX"}});
		 },
			[this, lua, camera]() {
				commandInterface.addLink(ValueHandle{lua, {"luaOutputs", "perspFrustum", "nearPlane"}}, ValueHandle{camera, {"frustum", "nearPlane"}});
			},
			[this, lua, camera]() {
				commandInterface.addLink(ValueHandle{lua, {"luaOutputs", "viewport"}}, ValueHandle{camera, {"viewport"}});
			},
			[this, lua, camera]() {
				commandInterface.addLink(ValueHandle{lua, {"luaOutputs", "perspFrustum"}}, ValueHandle{camera, {"frustum"}});
			}},
		{[this]() {
			 EXPECT_EQ(project.links().size(), 0);
		 },
			[this, lua, camera]() {
				checkLinks({{{lua, {"luaOutputs", "viewport", "offsetX"}}, {camera, {"viewport", "offsetX"}}, true}});
			},
			[this, lua, camera]() {
				checkLinks({{{lua, {"luaOutputs", "viewport", "offsetX"}}, {camera, {"viewport", "offsetX"}}, true},
					{{lua, {"luaOutputs", "perspFrustum", "nearPlane"}}, {camera, {"frustum", "nearPlane"}}, true}});
			},
			[this, lua, camera]() {
				checkLinks({{{lua, {"luaOutputs", "viewport"}}, {camera, {"viewport"}}, true},
					{{lua, {"luaOutputs", "perspFrustum", "nearPlane"}}, {camera, {"frustum", "nearPlane"}}, true}});
			},
			[this, lua, camera]() {
				checkLinks({{{lua, {"luaOutputs", "viewport"}}, {camera, {"viewport"}}, true},
					{{lua, {"luaOutputs", "perspFrustum"}}, {camera, {"frustum"}}, true}});
			}});
}

TEST_F(LinkTest, lua_ortho_camera_struct_viewport_frustum) {
	auto lua = create_lua("start", "scripts/camera-control.lua");
	auto camera = create<OrthographicCamera>("camera");

	checkUndoRedoMultiStep<4>(
		{[this, lua, camera]() {
			 commandInterface.addLink(ValueHandle{lua, {"luaOutputs", "viewport", "offsetY"}}, ValueHandle{camera, {"viewport", "offsetY"}});
		 },
			[this, lua, camera]() {
				commandInterface.addLink(ValueHandle{lua, {"luaOutputs", "orthoFrustum", "leftPlane"}}, ValueHandle{camera, {"frustum", "leftPlane"}});
			},
			[this, lua, camera]() {
				commandInterface.addLink(ValueHandle{lua, {"luaOutputs", "viewport"}}, ValueHandle{camera, {"viewport"}});
			},
			[this, lua, camera]() {
				commandInterface.addLink(ValueHandle{lua, {"luaOutputs", "orthoFrustum"}}, ValueHandle{camera, {"frustum"}});
			}},
		{[this]() {
			 EXPECT_EQ(project.links().size(), 0);
		 },
			[this, lua, camera]() {
				checkLinks({{{lua, {"luaOutputs", "viewport", "offsetY"}}, {camera, {"viewport", "offsetY"}}, true}});
			},
			[this, lua, camera]() {
				checkLinks({{{lua, {"luaOutputs", "viewport", "offsetY"}}, {camera, {"viewport", "offsetY"}}, true},
					{{lua, {"luaOutputs", "orthoFrustum", "leftPlane"}}, {camera, {"frustum", "leftPlane"}}, true}});
			},
			[this, lua, camera]() {
				checkLinks({{{lua, {"luaOutputs", "viewport"}}, {camera, {"viewport"}}, true},
					{{lua, {"luaOutputs", "orthoFrustum", "leftPlane"}}, {camera, {"frustum", "leftPlane"}}, true}});
			},
			[this, lua, camera]() {
				checkLinks({{{lua, {"luaOutputs", "viewport"}}, {camera, {"viewport"}}, true},
					{{lua, {"luaOutputs", "orthoFrustum"}}, {camera, {"frustum"}}, true}});
			}});
}

TEST_F(LinkTest, lua_lua_compatibility_float) {
	auto start = create_lua("start", "scripts/types-scalar.lua");
	auto end = create_lua("end", "scripts/struct-simple.lua");

	auto allowed = Queries::allowedLinkStartProperties(project, ValueHandle(end, {"luaInputs", "s", "float"}));
	std::set<ValueHandle> refAllowed{
		{start, {"luaOutputs", "ofloat"}},
		{start, {"luaOutputs", "foo"}},
		{start, {"luaOutputs", "bar"}}};

	EXPECT_EQ(allowed, refAllowed);
}

TEST_F(LinkTest, lua_lua_compatibility_struct) {
	auto start = create_lua("start", "scripts/struct-simple.lua");
	auto end = create_lua("end", "scripts/struct-simple.lua");

	auto allowed = Queries::allowedLinkStartProperties(project, ValueHandle(end, {"luaInputs", "s"}));

	EXPECT_EQ(allowed, std::set<ValueHandle>({{start, {"luaOutputs", "s"}}}));
}

TEST_F(LinkTest, lua_loop_detection) {
	auto start = create_lua("start", "scripts/types-scalar.lua");
	auto end = create_lua("end", "scripts/types-scalar.lua");
	auto [sprop, eprop] = link(start, {"luaOutputs", "ofloat"}, end, {"luaInputs", "float"});
	checkLinks({{sprop, eprop, true}});

	// 2 lua scripts, connected graph -> no links allowed
	{
		auto allowed = Queries::allowedLinkStartProperties(project, ValueHandle(start, {"luaInputs", "float"}));
		EXPECT_EQ(allowed.size(), 0);
	}

	auto start2 = create_lua("start 2", "scripts/types-scalar.lua");
	auto end2 = create_lua("end 2", "scripts/types-scalar.lua");
	context.addLink(ValueHandle{start2, {"luaOutputs", "ofloat"}}, ValueHandle{end2, {"luaInputs", "float"}});
	EXPECT_EQ(project.links().size(), 2);

	// 4 lua scripts, graph not connected, links allowed between the 2 disconnected subgraphs
	{
		auto allowed = Queries::allowedLinkStartProperties(project, ValueHandle(start, {"luaInputs", "float"}));
		std::set<ValueHandle> refAllowed{
			{start2, {"luaOutputs", "ofloat"}},
			{start2, {"luaOutputs", "foo"}},
			{start2, {"luaOutputs", "bar"}},
			{end2, {"luaOutputs", "ofloat"}},
			{end2, {"luaOutputs", "foo"}},
			{end2, {"luaOutputs", "bar"}}};
		EXPECT_EQ(allowed, refAllowed);
	}

	context.addLink(ValueHandle{end, {"luaOutputs", "ofloat"}}, ValueHandle{start2, {"luaInputs", "float"}});
	EXPECT_EQ(project.links().size(), 3);

	// 4 lua scripts, graph connected again, no links allowed
	{
		auto allowed = Queries::allowedLinkStartProperties(project, ValueHandle(start, {"luaInputs", "float"}));
		EXPECT_EQ(allowed.size(), 0);
	}
}

TEST_F(LinkTest, removal_del_start_obj) {
	auto start = create_lua("start", "scripts/types-scalar.lua");
	auto end = create_lua("end", "scripts/types-scalar.lua");
	auto [sprop, eprop] = link(start, {"luaOutputs", "ofloat"}, end, {"luaInputs", "float"});
	checkLinks({{sprop, eprop, true}});

	commandInterface.deleteObjects({start});
	checkLinks({});
}
TEST_F(LinkTest, removal_del_end_obj) {
	auto start = create_lua("start", "scripts/types-scalar.lua");
	auto end = create_lua("end", "scripts/types-scalar.lua");
	auto [sprop, eprop] = link(start, {"luaOutputs", "ofloat"}, end, {"luaInputs", "float"});
	checkLinks({{sprop, eprop, true}});

	commandInterface.deleteObjects({end});
	checkLinks({});
}

TEST_F(LinkTest, removal_del_start_obj_invalid_link) {
	auto start = create_lua("start", "scripts/types-scalar.lua");
	auto end = create_lua("end", "scripts/SimpleScript.lua");
	auto [sprop, eprop] = link(start, {"luaOutputs", "ofloat"}, end, {"luaInputs", "in_float"});
	checkLinks({{sprop, eprop, true}});

	change_uri(start, "scripts/SimpleScript.lua");
	checkLinks({{sprop, eprop, false}});

	commandInterface.deleteObjects({start});
	checkLinks({});
}

TEST_F(LinkTest, removal_del_end_obj_invalid_link) {
	auto start = create_lua("start", "scripts/types-scalar.lua");
	auto end = create_lua("end", "scripts/SimpleScript.lua");
	auto [sprop, eprop] = link(start, {"luaOutputs", "ofloat"}, end, {"luaInputs", "in_float"});
	checkLinks({{sprop, eprop, true}});

	change_uri(start, "scripts/SimpleScript.lua");
	checkLinks({{sprop, eprop, false}});

	commandInterface.deleteObjects({end});
	checkLinks({});
}

TEST_F(LinkTest, removal_sync_lua_simple) {
	TextFile script_1 = makeFile("script-1.lua",
		R"(
function interface()
	IN.v = FLOAT
	IN.flag = BOOL
	OUT.v = FLOAT
	OUT.flag = BOOL
end

function run()
end

)");

	TextFile script_2 = makeFile("script-2.lua",
		R"(
function interface()
	IN.v = INT
	IN.flag = BOOL
	OUT.v = INT
	OUT.flag = BOOL
end

function run()
end

)");

	auto start = create_lua("start", script_1);
	auto end = create_lua("end", script_1);

	context.addLink({start, {"luaOutputs", "v"}}, {end, {"luaInputs", "v"}});
	context.addLink({start, {"luaOutputs", "flag"}}, {end, {"luaInputs", "flag"}});
	checkLinks({{{start, {"luaOutputs", "flag"}}, {end, {"luaInputs", "flag"}}, true},
		{{start, {"luaOutputs", "v"}}, {end, {"luaInputs", "v"}}, true}});

	context.set({start, {"uri"}}, script_2);
	checkLinks({{{start, {"luaOutputs", "flag"}}, {end, {"luaInputs", "flag"}}, true},
		{{start, {"luaOutputs", "v"}}, {end, {"luaInputs", "v"}}, false}});
}

TEST_F(LinkTest, removal_sync_lua_struct_remove_property_whole) {
	TextFile script_1 = makeFile("script-1.lua",
		R"(
function interface()
	st = { x = FLOAT, y = FLOAT }
	IN.s = st
	OUT.s = st
end

function run()
end

)");

	TextFile script_2 = makeFile("script-2.lua",
		R"(
function interface()
	st = { x = FLOAT}
	IN.s = st
	OUT.s = st
end

function run()
end

)");

	auto start = create_lua("start", script_1);
	auto end = create_lua("end", script_1);

	auto [sprop, eprop] = link(start, {"luaOutputs", "s"}, end, {"luaInputs", "s"});
	checkLinks({{sprop, eprop, true}});

	context.set({start, {"uri"}}, script_2);
	checkLinks({{sprop, eprop, false}});
}

TEST_F(LinkTest, removal_sync_lua_struct_remove_property_member) {
	TextFile script_1 = makeFile("script-1.lua",
		R"(
function interface()
	st = { x = FLOAT, y = FLOAT, z = FLOAT }
	IN.s = st
	OUT.s = st
end

function run()
end

)");

	TextFile script_2 = makeFile("script-2.lua",
		R"(
function interface()
	st = { x = FLOAT, z = INT}
	IN.s = st
	OUT.s = st
end

function run()
end

)");

	auto start = create_lua("start", script_1);
	auto end = create_lua("end", script_1);

	context.addLink({start, {"luaOutputs", "s", "x"}}, {end, {"luaInputs", "s", "x"}});
	context.addLink({start, {"luaOutputs", "s", "y"}}, {end, {"luaInputs", "s", "y"}});
	context.addLink({start, {"luaOutputs", "s", "z"}}, {end, {"luaInputs", "s", "z"}});

	checkLinks({{{start, {"luaOutputs", "s", "x"}}, {end, {"luaInputs", "s", "x"}}, true},
		{{start, {"luaOutputs", "s", "y"}}, {end, {"luaInputs", "s", "y"}}, true},
		{{start, {"luaOutputs", "s", "z"}}, {end, {"luaInputs", "s", "z"}}, true}});

	context.set({start, {"uri"}}, script_2);

	checkLinks({{{start, {"luaOutputs", "s", "x"}}, {end, {"luaInputs", "s", "x"}}, true},
		{{start, {"luaOutputs", "s", "y"}}, {end, {"luaInputs", "s", "y"}}, false},
		{{start, {"luaOutputs", "s", "z"}}, {end, {"luaInputs", "s", "z"}}, false}});
}

TEST_F(LinkTest, removal_sync_lua_struct_add_property_whole) {
	TextFile script_1 = makeFile("script-1.lua",
		R"(
function interface()
	st = { x = FLOAT, y = FLOAT }
	IN.s = st
	OUT.s = st
end

function run()
end

)");

	TextFile script_2 = makeFile("script-2.lua",
		R"(
function interface()
	st = { x = FLOAT, y = FLOAT, z = INT}
	IN.s = st
	OUT.s = st
end

function run()
end

)");

	auto start = create_lua("start", script_1);
	auto end = create_lua("end", script_1);

	auto [sprop, eprop] = link(start, {"luaOutputs", "s"}, end, {"luaInputs", "s"});
	checkLinks({{sprop, eprop, true}});

	context.set({start, {"uri"}}, script_2);
	checkLinks({{sprop, eprop, false}});
}

TEST_F(LinkTest, removal_sync_lua_struct_add_property_member) {
	TextFile script_1 = makeFile("script-1.lua",
		R"(
function interface()
	st = { x = FLOAT, y = FLOAT }
	IN.s = st
	OUT.s = st
end

function run()
end

)");

	TextFile script_2 = makeFile("script-2.lua",
		R"(
function interface()
	st = { x = FLOAT, y = INT, z = INT}
	IN.s = st
	OUT.s = st
end

function run()
end

)");

	auto start = create_lua("start", script_1);
	auto end = create_lua("end", script_1);

	context.addLink({start, {"luaOutputs", "s", "x"}}, {end, {"luaInputs", "s", "x"}});
	context.addLink({start, {"luaOutputs", "s", "y"}}, {end, {"luaInputs", "s", "y"}});
	checkLinks({{{{start, {"luaOutputs", "s", "x"}}, {end, {"luaInputs", "s", "x"}}, true},
		{{start, {"luaOutputs", "s", "y"}}, {end, {"luaInputs", "s", "y"}}, true}}});

	context.set({start, {"uri"}}, script_2);
	checkLinks({{{{start, {"luaOutputs", "s", "x"}}, {end, {"luaInputs", "s", "x"}}, true},
		{{start, {"luaOutputs", "s", "y"}}, {end, {"luaInputs", "s", "y"}}, false}}});
}


TEST_F(LinkTest, removal_sync_lua_struct_member) {
	TextFile script_1 = makeFile("script-1.lua",
		R"(
function interface()
	st = { x = FLOAT, y = FLOAT }
	IN.s = st
	OUT.s = st
end

function run()
end

)");

	TextFile script_2 = makeFile("script-2.lua",
		R"(
function interface()
	IN.s = FLOAT
	OUT.s = FLOAT
end

function run()
end

)");

	auto start = create_lua("start", script_1);
	auto end = create_lua("end", script_1);

	auto [sprop, eprop] = link(start, {"luaOutputs", "s", "x"}, end, {"luaInputs", "s", "x"});
	checkLinks({{sprop, eprop, true}});

	context.set({start, {"uri"}}, script_2);
	checkLinks({{sprop, eprop, false}});
}

TEST_F(LinkTest, removal_move_start_lua_lua) {
	auto start = create_lua("start", "scripts/types-scalar.lua");
	auto end = create_lua("end", "scripts/types-scalar.lua");
	auto node = create<Node>("node");

	auto [sprop, eprop] = link(start, {"luaOutputs", "ofloat"}, end, {"luaInputs", "float"});

	commandInterface.moveScenegraphChild(start, node);
	ASSERT_EQ(project.links().size(), 0);
}

TEST_F(LinkTest, removal_move_end_lua_lua) {
	auto start = create_lua("start", "scripts/types-scalar.lua");
	auto end = create_lua("end", "scripts/types-scalar.lua");
	auto node = create<Node>("node");
	auto meshnode = create<MeshNode>("meshnode");

	auto [sprop, eprop] = link(start, {"luaOutputs", "ofloat"}, end, {"luaInputs", "float"});

	commandInterface.moveScenegraphChild(end, node);
	checkLinks({{sprop, eprop, true}});

	commandInterface.moveScenegraphChild(start, node);
	checkLinks({{sprop, eprop, true}});

	commandInterface.moveScenegraphChild(start, meshnode);
	checkLinks({{sprop, eprop, true}});

	commandInterface.moveScenegraphChild(end, nullptr);
	ASSERT_EQ(project.links().size(), 0);
}

TEST_F(LinkTest, removal_move_start_lua_mat) {
	auto lua = create_lua("base", "scripts/types-scalar.lua");
	auto material = create_material("material", "shaders/basic.vert", "shaders/basic.frag");
	auto node = create<Node>("node");

	auto [sprop, eprop] = link(lua, {"luaOutputs", "ovector3f"}, material, {"uniforms", "u_color"});
	checkLinks({{sprop, eprop, true}});

	commandInterface.moveScenegraphChild(lua, node);
	ASSERT_EQ(project.links().size(), 0);
}

TEST_F(LinkTest, removal_move_middle_lua_lua_mat) {
	auto start = create_lua("start", "scripts/types-scalar.lua");
	auto end = create_lua("end", "scripts/types-scalar.lua");
	auto material = create_material("material", "shaders/basic.vert", "shaders/basic.frag");
	auto node = create<Node>("node");

	auto [sprop, iprop] = link(start, {"luaOutputs", "ofloat"}, end, {"luaInputs", "float"});
	auto [oprop, eprop] = link(end, {"luaOutputs", "ovector3f"}, material, {"uniforms", "u_color"});
	checkLinks({{sprop, iprop, true}, {oprop, eprop, true}});

	commandInterface.moveScenegraphChild(end, node);
	checkLinks({{sprop, iprop, true}});
}

TEST_F(LinkTest, lua_restore_after_reselecting_output_uri) {
	auto linkBase = create_lua("base", "scripts/types-scalar.lua");
	auto linkRecipient = create_lua("recipient", "scripts/SimpleScript.lua");
	auto [sprop, eprop] = link(linkBase, {"luaOutputs", "ofloat"}, linkRecipient, {"luaInputs", "in_float"});
	checkLinks({{sprop, eprop, true}});

	change_uri(linkBase, "scripts/SimpleScript.lua");
	checkLinks({{sprop, eprop, false}});

	change_uri(linkBase, "scripts/types-scalar.lua");
	checkLinks({{sprop, eprop, true}});
}

TEST_F(LinkTest, lua_restore_only_when_property_types_fit) {
	TextFile leftScript1 = makeFile("leftScript1.lua",
		R"(
function interface()
    OUT.a = FLOAT
end

function run()
end
)");

	TextFile leftScript2 = makeFile("leftScript2.lua",
		R"(
function interface()
    OUT.a = INT
end

function run()
end
)");

	TextFile rightScript1 = makeFile("rightScript1.lua",
		R"(
function interface()
    IN.b = FLOAT
end

function run()
end
)");

	TextFile rightScript2 = makeFile("rightScript2.lua",
		R"(
function interface()
    IN.b = INT
end

function run()
end
)");

	auto right = create_lua("right", rightScript1);
	auto left = create_lua("left", leftScript1);

	auto [sprop, eprop] = link(left, {"luaOutputs", "a"}, right, {"luaInputs", "b"});
	checkLinks({{sprop, eprop, true}});

	commandInterface.set({left, {"uri"}}, leftScript2);
	checkLinks({{sprop, eprop, false}});

	commandInterface.set({right, {"uri"}}, rightScript2);
	checkLinks({{sprop, eprop, true}});

	commandInterface.set({right, {"uri"}}, rightScript1);
	checkLinks({{sprop, eprop, false}});

	commandInterface.set({left, {"uri"}}, leftScript1);
	checkLinks({{sprop, eprop, true}});
}

TEST_F(LinkTest, lua_restore_after_reselecting_input_uri) {
	auto linkBase = create_lua("base", "scripts/types-scalar.lua");
	auto linkRecipient = create_lua("recipient", "scripts/SimpleScript.lua");
	auto [sprop, eprop] = link(linkBase, {"luaOutputs", "ofloat"}, linkRecipient, {"luaInputs", "in_float"});
	checkLinks({{sprop, eprop, true}});

	change_uri(linkRecipient, "scripts/types-scalar.lua");
	checkLinks({{sprop, eprop, false}});

	change_uri(linkRecipient, "scripts/SimpleScript.lua");
	checkLinks({{sprop, eprop, true}});
}

TEST_F(LinkTest, lua_restore_two_scripts_prevent_loop_when_changing_output_uri) {
	auto linkBase = create_lua("base", "scripts/types-scalar.lua");
	auto linkRecipient = create_lua("recipient", "scripts/SimpleScript.lua");
	auto [sprop, eprop] = link(linkBase, {"luaOutputs", "ofloat"}, linkRecipient, {"luaInputs", "in_float"});
	checkLinks({{sprop, eprop, true}});

	change_uri(linkBase, "scripts/SimpleScript.lua");
	checkLinks({{sprop, eprop, false}});

	// Broken link hinders further links and thus loops
	auto allowed = Queries::allowedLinkStartProperties(project, ValueHandle(linkBase, {"luaOutputs", "out_float"}));
	EXPECT_EQ(allowed.size(), 0);

	change_uri(linkBase, "scripts/types-scalar.lua");
	checkLinks({{sprop, eprop, true}});
}

TEST_F(LinkTest, lua_restore_three_scripts_prevent_duplicate_links_with_same_property_name) {
	TextFile rightScript = makeFile("right.lua",
		R"(
function interface()
	IN.a = FLOAT
end

function run()
end

)");

	TextFile leftScript1 = makeFile("left-1.lua",
		R"(
function interface()
	OUT.b = FLOAT
end

function run()
end

)");

	TextFile leftScript2 = makeFile("left-2.lua",
		R"(
function interface()
	OUT.c = FLOAT
end

function run()
end

)");

	TextFile leftScript3 = makeFile("left-3.lua",
		R"(
function interface()
	OUT.b = FLOAT
	OUT.c = FLOAT
end

function run()
end

)");

	auto start = create_lua("start", leftScript1);
	auto end = create_lua("mid", rightScript);

	commandInterface.addLink({start, {"luaOutputs", "b"}}, {end, {"luaInputs", "a"}});
	checkLinks({{{start, {"luaOutputs", "b"}}, {end, {"luaInputs", "a"}}, true}});

	commandInterface.set({start, {"uri"}}, leftScript2);
	checkLinks({{{start, {"luaOutputs", "b"}}, {end, {"luaInputs", "a"}}, false}});

	commandInterface.addLink({start, {"luaOutputs", "c"}}, {end, {"luaInputs", "a"}});
	// BaseContext::addLink() replaces broken link with new link
	checkLinks({{{start, {"luaOutputs", "c"}}, {end, {"luaInputs", "a"}}, true}});

	commandInterface.set({start, {"uri"}}, leftScript3);
	checkLinks({{{start, {"luaOutputs", "c"}}, {end, {"luaInputs", "a"}}, true}});
}

TEST_F(LinkTest, lua_restore_two_scripts_prevent_multiple_active_links_on_same_output) {
	auto linkBase = create_lua("base", "scripts/types-scalar.lua");
	auto linkRecipient = create_lua("recipient", "scripts/SimpleScript.lua");
	auto [sprop, eprop] = link(linkBase, {"luaOutputs", "ofloat"}, linkRecipient, {"luaInputs", "in_float"});
	checkLinks({{sprop, eprop, true}});

	change_uri(linkRecipient, "scripts/types-scalar.lua");
	checkLinks({{sprop, eprop, false}});

	auto [sprop2, eprop2] = link(linkBase, {"luaOutputs", "ofloat"}, linkRecipient, {"luaInputs", "float"});
	checkLinks({{sprop, eprop, false},
		{sprop2, eprop2, true}});

	change_uri(linkRecipient, "scripts/SimpleScript.lua");
	checkLinks({{sprop, eprop, true},
		{sprop2, eprop2, false}});
}

TEST_F(LinkTest, lua_restore_nested_struct_link_from_other_struct) {
	auto linkBase = create_lua("base", "scripts/struct-nested.lua");
	auto linkRecipient = create_lua("recipient", "scripts/struct-nested.lua");
	auto [sprop, eprop] = link(linkBase, {"luaOutputs", "nested_out", "inner"}, linkRecipient, {"luaInputs", "nested", "inner"});
	checkLinks({{sprop, eprop, true}});

	change_uri(linkBase, "scripts/SimpleScript.lua");
	checkLinks({{sprop, eprop, false}});

	change_uri(linkBase, "scripts/struct-nested.lua");
	checkLinks({{sprop, eprop, true}});
}

TEST_F(LinkTest, lua_restore_nested_struct_link_from_empty_uri) {
	auto linkBase = create_lua("base", "scripts/struct-nested.lua");
	auto linkRecipient = create_lua("recipient", "scripts/struct-nested.lua");
	auto [sprop, eprop] = link(linkBase, {"luaOutputs", "nested_out", "inner"}, linkRecipient, {"luaInputs", "nested", "inner"});
	checkLinks({{sprop, eprop, true}});

	commandInterface.set(ValueHandle{linkBase, {"uri"}}, std::string());
	checkLinks({{sprop, eprop, false}});

	change_uri(linkBase, "scripts/struct-nested.lua");
	checkLinks({{sprop, eprop, true}});
}

TEST_F(LinkTest, lua_restore_nested_struct_link_only_when_struct_property_types_match) {
	TextFile script1 = makeFile("script-1.lua",
		R"(
function interface()
	s = { x = FLOAT}
	complex = {
		struct = s
    }
	IN.s = complex
	IN.nested = {
		inner = complex
	}
	OUT.nested_out = {
		inner = complex
	}
end


function run()
end

)");

	TextFile script2 = makeFile("script-2.lua",
		R"(
function interface()
	s = { x = INT}
	complex = {
		struct = s
    }
	IN.s = complex
	IN.nested = {
		inner = complex
	}
	OUT.nested_out = {
		inner = complex
	}
end


function run()
end

)");

	auto linkBase = create_lua("base", script1);
	auto linkRecipient = create_lua("recipient", script1);
	auto [sprop, eprop] = link(linkBase, {"luaOutputs", "nested_out", "inner", "struct"}, linkRecipient, {"luaInputs", "nested", "inner", "struct"});
	checkLinks({{sprop, eprop, true}});

	commandInterface.set(ValueHandle{linkBase, {"uri"}}, script2);
	checkLinks({{sprop, eprop, false}});

	commandInterface.set(ValueHandle{linkBase, {"uri"}}, script1);
	checkLinks({{sprop, eprop, true}});
}


TEST_F(LinkTest, lua_restore_nested_struct_property_link_after_reselecting_output_uri) {
	auto linkBase = create_lua("base", "scripts/types-scalar.lua");
	auto linkRecipient = create_lua("recipient", "scripts/struct-nested.lua");
	auto [sprop, eprop] = link(linkBase, {"luaOutputs", "ovector4f"}, linkRecipient, {"luaInputs", "nested", "inner", "vector4f"});
	checkLinks({{sprop, eprop, true}});

	change_uri(linkBase, "scripts/SimpleScript.lua");
	checkLinks({{sprop, eprop, false}});

	change_uri(linkBase, "scripts/types-scalar.lua");
	checkLinks({{sprop, eprop, true}});
}

TEST_F(LinkTest, lua_restore_nested_struct_property_link_after_reselecting_input_uri) {
	auto linkBase = create_lua("base", "scripts/types-scalar.lua");
	auto linkRecipient = create_lua("recipient", "scripts/struct-nested.lua");
	auto [sprop, eprop] = link(linkBase, {"luaOutputs", "ovector4f"}, linkRecipient, {"luaInputs", "nested", "inner", "vector4f"});
	checkLinks({{sprop, eprop, true}});

	change_uri(linkRecipient, "scripts/SimpleScript.lua");
	checkLinks({{sprop, eprop, false}});

	change_uri(linkRecipient, "scripts/struct-nested.lua");
	checkLinks({{sprop, eprop, true}});
}

TEST_F(LinkTest, lua_restore_struct_link_from_other_struct) {
	auto linkBase = create_lua("start", "scripts/struct-simple.lua");
	auto linkRecipient = create_lua("end", "scripts/struct-simple.lua");
	auto [sprop, eprop] = link(linkBase, {"luaOutputs", "s"}, linkRecipient, {"luaInputs", "s"});
	checkLinks({{sprop, eprop, true}});

	change_uri(linkBase, "scripts/struct-nested.lua");
	checkLinks({{sprop, eprop, false}});

	change_uri(linkBase, "scripts/struct-simple.lua");
	checkLinks({{sprop, eprop, true}});
}

TEST_F(LinkTest, lua_restore_struct_link_from_empty_uri) {
	auto linkBase = create_lua("start", "scripts/struct-simple.lua");
	auto linkRecipient = create_lua("end", "scripts/struct-simple.lua");
	auto [sprop, eprop] = link(linkBase, {"luaOutputs", "s"}, linkRecipient, {"luaInputs", "s"});
	checkLinks({{sprop, eprop, true}});

	commandInterface.set(ValueHandle{linkBase, {"uri"}}, std::string());
	checkLinks({{sprop, eprop, false}});

	change_uri(linkBase, "scripts/struct-simple.lua");
	checkLinks({{sprop, eprop, true}});
}

TEST_F(LinkTest, lua_restore_from_proper_base_script) {
	auto linkBase = create_lua("base", "scripts/types-scalar.lua");
	auto notLinkBase = create_lua("notbase", "scripts/types-scalar.lua");
	auto linkRecipient = create_lua("recipient", "scripts/SimpleScript.lua");
	auto [sprop, eprop] = link(linkBase, {"luaOutputs", "ofloat"}, linkRecipient, {"luaInputs", "in_float"});
	checkLinks({{sprop, eprop, true}});

	change_uri(linkBase, "scripts/SimpleScript.lua");
	checkLinks({{sprop, eprop, false}});

	change_uri(notLinkBase, "scripts/SimpleScript.lua");
	change_uri(notLinkBase, "scripts/types-scalar.lua");
	checkLinks({{sprop, eprop, false}});

	change_uri(linkBase, "scripts/types-scalar.lua");
	checkLinks({{sprop, eprop, true}});
}

TEST_F(LinkTest, lua_restore_single_output_property_multiple_links) {
	auto linkBase = create_lua("base", "scripts/types-scalar.lua");
	auto simpleScript = create_lua("base", "scripts/SimpleScript.lua");
	auto structSimple = create_lua("base", "scripts/struct-simple.lua");
	auto node = create<Node>("node");

	auto ofloat = PropertyDescriptor{linkBase, {"luaOutputs", "ofloat"}};
	auto ovector3f = PropertyDescriptor{linkBase, {"luaOutputs", "ovector3f"}};
	auto in_float = PropertyDescriptor{simpleScript, {"luaInputs", "in_float"}};
	auto s_vector3f = PropertyDescriptor{structSimple, {"luaInputs", "s", "vector3f"}};
	auto rotation = PropertyDescriptor{node, {"rotation"}};

	commandInterface.addLink(ofloat, in_float);
	commandInterface.addLink(ovector3f, s_vector3f);
	commandInterface.addLink(ovector3f, rotation);
	checkLinks({{ofloat, in_float, true},
		{ovector3f, s_vector3f, true},
		{ovector3f, rotation, true}});

	change_uri(linkBase, "scripts/SimpleScript.lua");
	checkLinks({{ofloat, in_float, false},
		{ovector3f, s_vector3f, false},
		{ovector3f, rotation, false}});

	change_uri(linkBase, "scripts/types-scalar.lua");
	checkLinks({{ofloat, in_float, true},
		{ovector3f, s_vector3f, true},
		{ovector3f, rotation, true}});
}

TEST_F(LinkTest, lua_restore_from_broken_script) {
	TextFile script = makeFile("script-1.lua",
		R"(
function interface()
	IN.v = FLOAT
	IN.flag = BOOL
	OUT.v = FLOAT
	OUT.flag = BOOL
end

function run()
end

)");

	TextFile scriptBroken = makeFile("script-2.lua",
		R"(
function interface()
	IN.v = FLOATa
	IN.flag = BOOL
	OUT.v = FLOAT
	OUT.flag = BOOL
ed

function run()

)");

	auto start = create_lua("base", script);
	auto end = create_lua("recipient", script);

	auto [sprop, eprop] = link(start, {"luaOutputs", "v"}, end, {"luaInputs", "v"});
	checkLinks({{sprop, eprop, true}});

	commandInterface.set({start, {"uri"}}, scriptBroken);
	checkLinks({{sprop, eprop, false}});

	commandInterface.set({start, {"uri"}}, script);
	checkLinks({{sprop, eprop, true}});
}

TEST_F(LinkTest, lua_restore_link_chain_from_two_broken_scripts) {
	TextFile script1 = makeFile("script-1.lua",
		R"(
function interface()
	OUT.f = FLOAT
end

function run()
end

)");

	TextFile script1Broken = makeFile("script-1-broken.lua",
		R"(
function interface()
	OUT.f = FLOATa
end

function run()
end

)");

	TextFile script2 = makeFile("script-2.lua",
		R"(
function interface()
	IN.f = FLOAT
	OUT.vec = VEC3F
end

function run()
end

)");

	TextFile script2Broken = makeFile("script-2-broken.lua",
		R"(
function interface()
	IN.f = FLOTA
	OUT.vec = VEC3
end

function run()
end

)");

	auto start = create_lua("start", script1);
	auto mid = create_lua("mid", script2);
	auto end = create<Node>("end");

	commandInterface.addLink({start, {"luaOutputs", "f"}}, {mid, {"luaInputs", "f"}});
	commandInterface.addLink({mid, {"luaOutputs", "vec"}}, {end, {"translation"}});

	commandInterface.set({start, {"uri"}}, script1Broken);
	commandInterface.set({mid, {"uri"}}, script2Broken);
	checkLinks({{{start, {"luaOutputs", "f"}}, {mid, {"luaInputs", "f"}}, false},
		{{mid, {"luaOutputs", "vec"}}, {end, {"translation"}}, false}});

	commandInterface.set({mid, {"uri"}}, script2);
	commandInterface.set({start, {"uri"}}, script1);
	checkLinks({{{start, {"luaOutputs", "f"}}, {mid, {"luaInputs", "f"}}}, 
		{{mid, {"luaOutputs", "vec"}}, {end, {"translation"}}}});
}

TEST_F(LinkTest, material_restore_uniform_links) {
	auto luaScript = create_lua("base", "scripts/types-scalar.lua");
	auto mesh = create_mesh("mesh", "meshes/Duck.glb");
	auto material = create_material("material", "shaders/basic.vert", "shaders/basic.frag");
	auto meshNode = create_meshnode("meshnode", mesh, material);
	auto emptyMaterial = create<Material>("emptymat");
	commandInterface.set(meshNode->getMaterialPrivateHandle(0), true);

	auto [sprop, eprop] = link(luaScript, {"luaOutputs", "ovector3f"}, meshNode, {"materials", "material", "uniforms", "u_color"});
	checkLinks({{sprop, eprop, true}});

	commandInterface.set(ValueHandle{meshNode, {"materials", "material", "material"}}, emptyMaterial);
	checkLinks({{sprop, eprop, false}});

	commandInterface.set(ValueHandle{meshNode, {"materials", "material", "material"}}, material);
	checkLinks({{sprop, eprop, true}});
}

TEST_F(LinkTest, restore_meshnode_uniform_switching_shared) {
	auto luaScript = create_lua("base", "scripts/types-scalar.lua");
	auto mesh = create_mesh("mesh", "meshes/Duck.glb");
	auto material = create_material("material", "shaders/basic.vert", "shaders/basic.frag");
	auto meshNode = create_meshnode("meshnode", mesh, material);
	commandInterface.set(meshNode->getMaterialPrivateHandle(0), true);

	auto [sprop, eprop] = link(luaScript, {"luaOutputs", "ovector3f"}, meshNode, {"materials", "material", "uniforms", "u_color"});
	checkLinks({{sprop, eprop, true}});

	commandInterface.set(meshNode->getMaterialPrivateHandle(0), false);
	checkLinks({{sprop, eprop, false}});

	commandInterface.set(meshNode->getMaterialPrivateHandle(0), true);
	checkLinks({{sprop, eprop, true}});
}
