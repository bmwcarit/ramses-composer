/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
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
#include "application/RaCoApplication.h"
#include "ramses_base/HeadlessEngineBackend.h"
#include "ramses_adaptor/SceneBackend.h"

#include "user_types/Mesh.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include "user_types/Prefab.h"
#include "user_types/PrefabInstance.h"
#include "user_types/PerspectiveCamera.h"
#include "user_types/OrthographicCamera.h"
#include "user_types/Timer.h"

#include "utils/FileUtils.h"

#include "gtest/gtest.h"

#include <algorithm>
#include <functional>

using namespace raco::core;
using namespace raco::user_types;

class LinkTest : public TestEnvironmentCore {
public:
	void change_uri(SEditorObject obj, const std::string& newvalue) {
		commandInterface.set({obj, {"uri"}}, (test_path() / newvalue).string());
	}

};

TEST_F(LinkTest, fail_create_invalid_handles) {
	auto start = create_lua("start", "scripts/types-scalar.lua");
	auto end = create_lua("end", "scripts/types-scalar.lua");

	EXPECT_THROW(commandInterface.addLink(ValueHandle{start, {"outputs", "ofloat"}}, ValueHandle{end, {"inputs", "INVALID"}}), std::runtime_error);
	EXPECT_THROW(commandInterface.addLink(ValueHandle{start, {"outputs", "INVALID"}}, ValueHandle{end, {"inputs", "float"}}), std::runtime_error);

	EXPECT_THROW(commandInterface.addLink(ValueHandle{start, {"outputs", "ofloat"}}, ValueHandle{end, {"INVALID"}}), std::runtime_error);
	EXPECT_THROW(commandInterface.addLink(ValueHandle{start, {"INVALID"}}, ValueHandle{end, {"inputs", "float"}}), std::runtime_error);

	EXPECT_THROW(commandInterface.addLink(ValueHandle{start, {"outputs", "ofloat"}}, ValueHandle{end}), std::runtime_error);
	EXPECT_THROW(commandInterface.addLink(ValueHandle{start}, ValueHandle{end, {"inputs", "float"}}), std::runtime_error);

	EXPECT_THROW(commandInterface.addLink(ValueHandle{start, {"outputs", "ofloat"}}, ValueHandle{}), std::runtime_error);
	EXPECT_THROW(commandInterface.addLink(ValueHandle{}, ValueHandle{end, {"inputs", "float"}}), std::runtime_error);
}

TEST_F(LinkTest, lua_lua_scalar) {
	auto start = create_lua("start", "scripts/types-scalar.lua");
	auto end = create_lua("end", "scripts/types-scalar.lua");

	checkUndoRedoMultiStep<2>(
		{[this, start, end]() { commandInterface.addLink(ValueHandle{start, {"outputs", "ofloat"}}, ValueHandle{end, {"inputs", "float"}}); },
			[this, end]() { commandInterface.removeLink({end, {"inputs", "float"}}); }},
		{
			[this]() {
				EXPECT_EQ(project.links().size(), 0);
			},
			[this, start, end]() {
				std::vector<Link> refLinks{{{{start, {"outputs", "ofloat"}}, {end, {"inputs", "float"}}}}};
				checkLinks(refLinks);
			},
			[this]() {
				EXPECT_EQ(project.links().size(), 0);
			},
		});
}

TEST_F(LinkTest, remove_valid) {
	auto start = create_lua("start", "scripts/types-scalar.lua");
	auto end = create_lua("end", "scripts/types-scalar.lua");

	auto [sprop, eprop] = link(start, {"outputs", "ofloat"}, end, {"inputs", "float"});
	checkLinks({{sprop, eprop, true}});
	commandInterface.removeLink(eprop);
	checkLinks({});
}

TEST_F(LinkTest, remove_invalid) {
	auto start = create_lua("start", "scripts/types-scalar.lua");
	auto end = create_lua("end", "scripts/types-scalar.lua");

	auto [sprop, eprop] = link(start, {"outputs", "ofloat"}, end, {"inputs", "float"});
	checkLinks({{sprop, eprop, true}});

	commandInterface.set({end, {"uri"}}, std::string());
	checkLinks({{sprop, eprop, false}});

	commandInterface.removeLink(eprop);
	checkLinks({});
}

TEST_F(LinkTest, lua_lua_recorder_merge) {
	auto start = create_lua("start", "scripts/types-scalar.lua");
	auto end = create_lua("end", "scripts/types-scalar.lua");

	EXPECT_EQ(project.links().size(), 0);
	commandInterface.addLink(ValueHandle{start, {"outputs", "ofloat"}}, ValueHandle{end, {"inputs", "float"}});

	std::vector<Link> refLinks{{{{start, {"outputs", "ofloat"}}, {end, {"inputs", "float"}}}}};
	checkLinks(refLinks);
	EXPECT_EQ(recorder.getAddedLinks().size(), 1);

	commandInterface.removeLink({end, {"inputs", "float"}});

	EXPECT_EQ(project.links().size(), 0);
	EXPECT_EQ(recorder.getAddedLinks().size(), 0);
	EXPECT_EQ(recorder.getRemovedLinks().size(), 0);
}

TEST_F(LinkTest, lua_lua_struct_simple) {
	auto start = create_lua("start", "scripts/struct-simple.lua");
	auto end = create_lua("end", "scripts/struct-simple.lua");

	checkUndoRedoMultiStep<2>(
		{[this, start, end]() { commandInterface.addLink(ValueHandle{start, {"outputs", "s", "float"}}, ValueHandle{end, {"inputs", "s", "float"}}); },
			[this, start, end]() { commandInterface.addLink(ValueHandle{start, {"outputs", "s"}}, ValueHandle{end, {"inputs", "s"}}); }},
		{[this]() {
			 EXPECT_EQ(project.links().size(), 0);
		 },
			[this, start, end]() {
				std::vector<Link> refLinks{{{{start, {"outputs", "s", "float"}}, {end, {"inputs", "s", "float"}}}}};
				checkLinks(refLinks);
			},
			[this, start, end]() {
				std::vector<Link> refLinks{{{{start, {"outputs", "s"}}, {end, {"inputs", "s"}}}}};
				checkLinks(refLinks);
			}});
}

TEST_F(LinkTest, lua_persp_camera_struct_viewport_frustum) {
	auto lua = create_lua("start", "scripts/camera-control.lua");
	auto camera = create<PerspectiveCamera>("camera");

	checkUndoRedoMultiStep<4>(
		{[this, lua, camera]() {
			 commandInterface.addLink(ValueHandle{lua, {"outputs", "viewport", "offsetX"}}, ValueHandle{camera, {"viewport", "offsetX"}});
		 },
			[this, lua, camera]() {
				commandInterface.addLink(ValueHandle{lua, {"outputs", "perspFrustum", "nearPlane"}}, ValueHandle{camera, {"frustum", "nearPlane"}});
			},
			[this, lua, camera]() {
				commandInterface.addLink(ValueHandle{lua, {"outputs", "viewport"}}, ValueHandle{camera, {"viewport"}});
			},
			[this, lua, camera]() {
				commandInterface.addLink(ValueHandle{lua, {"outputs", "perspFrustum"}}, ValueHandle{camera, {"frustum"}});
			}},
		{[this]() {
			 EXPECT_EQ(project.links().size(), 0);
		 },
			[this, lua, camera]() {
				checkLinks({{{lua, {"outputs", "viewport", "offsetX"}}, {camera, {"viewport", "offsetX"}}, true}});
			},
			[this, lua, camera]() {
				checkLinks({{{lua, {"outputs", "viewport", "offsetX"}}, {camera, {"viewport", "offsetX"}}, true},
					{{lua, {"outputs", "perspFrustum", "nearPlane"}}, {camera, {"frustum", "nearPlane"}}, true}});
			},
			[this, lua, camera]() {
				checkLinks({{{lua, {"outputs", "viewport"}}, {camera, {"viewport"}}, true},
					{{lua, {"outputs", "perspFrustum", "nearPlane"}}, {camera, {"frustum", "nearPlane"}}, true}});
			},
			[this, lua, camera]() {
				checkLinks({{{lua, {"outputs", "viewport"}}, {camera, {"viewport"}}, true},
					{{lua, {"outputs", "perspFrustum"}}, {camera, {"frustum"}}, true}});
			}});
}

TEST_F(LinkTest, lua_ortho_camera_struct_viewport_frustum) {
	auto lua = create_lua("start", "scripts/camera-control.lua");
	auto camera = create<OrthographicCamera>("camera");

	checkUndoRedoMultiStep<4>(
		{[this, lua, camera]() {
			 commandInterface.addLink(ValueHandle{lua, {"outputs", "viewport", "offsetY"}}, ValueHandle{camera, {"viewport", "offsetY"}});
		 },
			[this, lua, camera]() {
				commandInterface.addLink(ValueHandle{lua, {"outputs", "orthoFrustum", "leftPlane"}}, ValueHandle{camera, {"frustum", "leftPlane"}});
			},
			[this, lua, camera]() {
				commandInterface.addLink(ValueHandle{lua, {"outputs", "viewport"}}, ValueHandle{camera, {"viewport"}});
			},
			[this, lua, camera]() {
				commandInterface.addLink(ValueHandle{lua, {"outputs", "orthoFrustum"}}, ValueHandle{camera, {"frustum"}});
			}},
		{[this]() {
			 EXPECT_EQ(project.links().size(), 0);
		 },
			[this, lua, camera]() {
				checkLinks({{{lua, {"outputs", "viewport", "offsetY"}}, {camera, {"viewport", "offsetY"}}, true}});
			},
			[this, lua, camera]() {
				checkLinks({{{lua, {"outputs", "viewport", "offsetY"}}, {camera, {"viewport", "offsetY"}}, true},
					{{lua, {"outputs", "orthoFrustum", "leftPlane"}}, {camera, {"frustum", "leftPlane"}}, true}});
			},
			[this, lua, camera]() {
				checkLinks({{{lua, {"outputs", "viewport"}}, {camera, {"viewport"}}, true},
					{{lua, {"outputs", "orthoFrustum", "leftPlane"}}, {camera, {"frustum", "leftPlane"}}, true}});
			},
			[this, lua, camera]() {
				checkLinks({{{lua, {"outputs", "viewport"}}, {camera, {"viewport"}}, true},
					{{lua, {"outputs", "orthoFrustum"}}, {camera, {"frustum"}}, true}});
			}});
}

TEST_F(LinkTest, lua_lua_compatibility_int64) {
	auto start = create_lua("start", "scripts/types-scalar.lua");
	auto end = create_lua("end", "scripts/struct-simple.lua");

	auto allowed64 = Queries::allLinkStartProperties(project, ValueHandle(end, {"inputs", "s", "integer64"}));
	std::set<std::tuple<ValueHandle, bool, bool>> refAllowed64{
		{{start, {"outputs", "ointeger64"}}, true, true}};

	EXPECT_EQ(allowed64, refAllowed64);

	auto allowed32 = Queries::allLinkStartProperties(project, ValueHandle(end, {"inputs", "s", "integer"}));
	std::set<std::tuple<ValueHandle, bool, bool>> refAllowed32{
		{{start, {"outputs", "ointeger"}}, true, true}};

	EXPECT_EQ(allowed32, refAllowed32);
}

TEST_F(LinkTest, lua_lua_compatibility_float) {
	auto start = create_lua("start", "scripts/types-scalar.lua");
	auto end = create_lua("end", "scripts/struct-simple.lua");

	auto allowed = Queries::allLinkStartProperties(project, ValueHandle(end, {"inputs", "s", "float"}));
	std::set<std::tuple<ValueHandle, bool, bool>> refAllowed{
		{{start, {"outputs", "ofloat"}}, true, true},
		{{start, {"outputs", "foo"}}, true, true},
		{{start, {"outputs", "bar"}}, true, true}};

	EXPECT_EQ(allowed, refAllowed);
}

TEST_F(LinkTest, lua_lua_compatibility_struct) {
	auto start = create_lua("start", "scripts/struct-simple.lua");
	auto end = create_lua("end", "scripts/struct-simple.lua");

	auto allowed = Queries::allLinkStartProperties(project, ValueHandle(end, {"inputs", "s"}));

	std::set<std::tuple<ValueHandle, bool, bool>> refAllowed{{{start, {"outputs", "s"}}, true, true}};
	EXPECT_EQ(allowed, refAllowed);
}

TEST_F(LinkTest, loop_weak_after_strong) {
	auto start = create_lua("start", "scripts/types-scalar.lua");
	auto end = create_lua("end", "scripts/types-scalar.lua");
	auto [start_out, end_in] = link(start, {"outputs", "ofloat"}, end, {"inputs", "float"});
	checkLinks({{start_out, end_in, true, false}});

	auto [end_out, start_in] = link(end, {"outputs", "ofloat"}, start, {"inputs", "float"}, true);
	checkLinks({{start_out, end_in, true, false},
		{end_out, start_in, true, true}});
}

TEST_F(LinkTest, loop_strong_after_weak) {
	auto start = create_lua("start", "scripts/types-scalar.lua");
	auto end = create_lua("end", "scripts/types-scalar.lua");
	auto [start_out, end_in] = link(start, {"outputs", "ofloat"}, end, {"inputs", "float"}, true);
	checkLinks({{start_out, end_in, true, true}});

	auto [end_out, start_in] = link(end, {"outputs", "ofloat"}, start, {"inputs", "float"}, false);
	checkLinks({{start_out, end_in, true, true},
		{end_out, start_in, true, false}});
}


TEST_F(LinkTest, lua_loop_detection) {
	auto start = create_lua("start", "scripts/types-scalar.lua");
	auto end = create_lua("end", "scripts/types-scalar.lua");
	auto [sprop, eprop] = link(start, {"outputs", "ofloat"}, end, {"inputs", "float"});
	checkLinks({{sprop, eprop, true}});

	// 2 lua scripts, connected graph -> no links allowed
	{
		auto allowed = Queries::allLinkStartProperties(project, ValueHandle(start, {"inputs", "float"}));
		std::set<std::tuple<ValueHandle, bool, bool>> refAllowed{
			{{end, {"outputs", "ofloat"}}, false, true},
			{{end, {"outputs", "foo"}}, false, true},
			{{end, {"outputs", "bar"}}, false, true}};

		EXPECT_EQ(allowed, refAllowed);
	}

	auto start2 = create_lua("start 2", "scripts/types-scalar.lua");
	auto end2 = create_lua("end 2", "scripts/types-scalar.lua");
	context.addLink(ValueHandle{start2, {"outputs", "ofloat"}}, ValueHandle{end2, {"inputs", "float"}});
	EXPECT_EQ(project.links().size(), 2);

	// 4 lua scripts, graph not connected, links allowed between the 2 disconnected subgraphs
	{
		auto allowed = Queries::allLinkStartProperties(project, ValueHandle(start, {"inputs", "float"}));
		std::set<std::tuple<ValueHandle, bool, bool>> refAllowed{
			{{end, {"outputs", "ofloat"}}, false, true},
			{{end, {"outputs", "foo"}}, false, true},
			{{end, {"outputs", "bar"}}, false, true},
			{{start2, {"outputs", "ofloat"}}, true, true},
			{{start2, {"outputs", "foo"}}, true, true},
			{{start2, {"outputs", "bar"}}, true, true},
			{{end2, {"outputs", "ofloat"}}, true, true},
			{{end2, {"outputs", "foo"}}, true, true},
			{{end2, {"outputs", "bar"}}, true, true}};
		EXPECT_EQ(allowed, refAllowed);
	}

	context.addLink(ValueHandle{end, {"outputs", "ofloat"}}, ValueHandle{start2, {"inputs", "float"}});
	EXPECT_EQ(project.links().size(), 3);

	// 4 lua scripts, graph connected again, no links allowed
	{
		auto allowed = Queries::allLinkStartProperties(project, ValueHandle(start, {"inputs", "float"}));
		std::set<std::tuple<ValueHandle, bool, bool>> refAllowed{
			{{end, {"outputs", "ofloat"}}, false, true},
			{{end, {"outputs", "foo"}}, false, true},
			{{end, {"outputs", "bar"}}, false, true},
			{{start2, {"outputs", "ofloat"}}, false, true},
			{{start2, {"outputs", "foo"}}, false, true},
			{{start2, {"outputs", "bar"}}, false, true},
			{{end2, {"outputs", "ofloat"}}, false, true},
			{{end2, {"outputs", "foo"}}, false, true},
			{{end2, {"outputs", "bar"}}, false, true}};
		EXPECT_EQ(allowed, refAllowed);
	}
}

TEST_F(LinkTest, lua_loop_detection_after_remove_link) {
	auto start = create_lua("start", "scripts/types-scalar.lua");
	auto end = create_lua("end", "scripts/types-scalar.lua");
	auto [sprop1, eprop1] = link(start, {"outputs", "ofloat"}, end, {"inputs", "float"});
	auto [sprop2, eprop2] = link(start, {"outputs", "ointeger"}, end, {"inputs", "integer"});
	checkLinks({{sprop1, eprop1, true},
		{sprop2, eprop2, true}});

	{
		auto allowed = Queries::allLinkStartProperties(project, ValueHandle(start, {"inputs", "float"}));
		std::set<std::tuple<ValueHandle, bool, bool>> refAllowed{
			{{end, {"outputs", "ofloat"}}, false, true},
			{{end, {"outputs", "foo"}}, false, true},
			{{end, {"outputs", "bar"}}, false, true}};

		EXPECT_EQ(allowed, refAllowed);
	}

	context.removeLink(eprop2);
	{
		auto allowed = Queries::allLinkStartProperties(project, ValueHandle(start, {"inputs", "float"}));
		std::set<std::tuple<ValueHandle, bool, bool>> refAllowed{
			{{end, {"outputs", "ofloat"}}, false, true},
			{{end, {"outputs", "foo"}}, false, true},
			{{end, {"outputs", "bar"}}, false, true}};

		EXPECT_EQ(allowed, refAllowed);
	}
	
	context.removeLink(eprop1);
	{
		auto allowed = Queries::allLinkStartProperties(project, ValueHandle(start, {"inputs", "float"}));
		std::set<std::tuple<ValueHandle, bool, bool>> refAllowed{
			{{end, {"outputs", "ofloat"}}, true, true},
			{{end, {"outputs", "foo"}}, true, true},
			{{end, {"outputs", "bar"}}, true, true}};

		EXPECT_EQ(allowed, refAllowed);
	}
}


TEST_F(LinkTest, removal_del_start_obj) {
	auto start = create_lua("start", "scripts/types-scalar.lua");
	auto end = create_lua("end", "scripts/types-scalar.lua");
	auto [sprop, eprop] = link(start, {"outputs", "ofloat"}, end, {"inputs", "float"});
	checkLinks({{sprop, eprop, true}});

	commandInterface.deleteObjects({start});
	checkLinks({});
}
TEST_F(LinkTest, removal_del_end_obj) {
	auto start = create_lua("start", "scripts/types-scalar.lua");
	auto end = create_lua("end", "scripts/types-scalar.lua");
	auto [sprop, eprop] = link(start, {"outputs", "ofloat"}, end, {"inputs", "float"});
	checkLinks({{sprop, eprop, true}});

	commandInterface.deleteObjects({end});
	checkLinks({});
}

TEST_F(LinkTest, removal_del_start_obj_invalid_link) {
	auto start = create_lua("start", "scripts/types-scalar.lua");
	auto end = create_lua("end", "scripts/SimpleScript.lua");
	auto [sprop, eprop] = link(start, {"outputs", "ofloat"}, end, {"inputs", "in_float"});
	checkLinks({{sprop, eprop, true}});

	change_uri(start, "scripts/SimpleScript.lua");
	checkLinks({{sprop, eprop, false}});

	commandInterface.deleteObjects({start});
	checkLinks({});
}

TEST_F(LinkTest, removal_del_end_obj_invalid_link) {
	auto start = create_lua("start", "scripts/types-scalar.lua");
	auto end = create_lua("end", "scripts/SimpleScript.lua");
	auto [sprop, eprop] = link(start, {"outputs", "ofloat"}, end, {"inputs", "in_float"});
	checkLinks({{sprop, eprop, true}});

	change_uri(start, "scripts/SimpleScript.lua");
	checkLinks({{sprop, eprop, false}});

	commandInterface.deleteObjects({end});
	checkLinks({});
}

TEST_F(LinkTest, removal_sync_lua_simple) {
	TextFile script_1 = makeFile("script-1.lua",
		R"(
function interface(IN,OUT)
	IN.v = Type:Float()
	IN.flag = Type:Bool()
	OUT.v = Type:Float()
	OUT.flag = Type:Bool()
end

function run(IN,OUT)
end

)");

	TextFile script_2 = makeFile("script-2.lua",
		R"(
function interface(IN,OUT)
	IN.v = Type:Int32()
	IN.flag = Type:Bool()
	OUT.v = Type:Int32()
	OUT.flag = Type:Bool()
end

function run(IN,OUT)
end

)");

	auto start = create_lua("start", script_1);
	auto end = create_lua("end", script_1);

	context.addLink({start, {"outputs", "v"}}, {end, {"inputs", "v"}});
	context.addLink({start, {"outputs", "flag"}}, {end, {"inputs", "flag"}});
	checkLinks({{{start, {"outputs", "flag"}}, {end, {"inputs", "flag"}}, true},
		{{start, {"outputs", "v"}}, {end, {"inputs", "v"}}, true}});

	context.set({start, {"uri"}}, script_2);
	checkLinks({{{start, {"outputs", "flag"}}, {end, {"inputs", "flag"}}, true},
		{{start, {"outputs", "v"}}, {end, {"inputs", "v"}}, false}});
}

TEST_F(LinkTest, removal_sync_lua_struct_remove_property_whole) {
	TextFile script_1 = makeFile("script-1.lua",
		R"(
function interface(IN,OUT)
	local st = { x = Type:Float(), y = Type:Float() }
	IN.s = st
	OUT.s = st
end

function run(IN,OUT)
end

)");

	TextFile script_2 = makeFile("script-2.lua",
		R"(
function interface(IN,OUT)
	local st = { x = Type:Float()}
	IN.s = st
	OUT.s = st
end

function run(IN,OUT)
end

)");

	auto start = create_lua("start", script_1);
	auto end = create_lua("end", script_1);

	auto [sprop, eprop] = link(start, {"outputs", "s"}, end, {"inputs", "s"});
	checkLinks({{sprop, eprop, true}});

	context.set({start, {"uri"}}, script_2);
	checkLinks({{sprop, eprop, false}});
}

TEST_F(LinkTest, removal_sync_lua_struct_remove_property_member) {
	TextFile script_1 = makeFile("script-1.lua",
		R"(
function interface(IN,OUT)
	local st = { x = Type:Float(), y = Type:Float(), z = Type:Float() }
	IN.s = st
	OUT.s = st
end

function run(IN,OUT)
end

)");

	TextFile script_2 = makeFile("script-2.lua",
		R"(
function interface(IN,OUT)
	local st = { x = Type:Float(), z = Type:Int32()}
	IN.s = st
	OUT.s = st
end

function run(IN,OUT)
end

)");

	auto start = create_lua("start", script_1);
	auto end = create_lua("end", script_1);

	context.addLink({start, {"outputs", "s", "x"}}, {end, {"inputs", "s", "x"}});
	context.addLink({start, {"outputs", "s", "y"}}, {end, {"inputs", "s", "y"}});
	context.addLink({start, {"outputs", "s", "z"}}, {end, {"inputs", "s", "z"}});

	checkLinks({{{start, {"outputs", "s", "x"}}, {end, {"inputs", "s", "x"}}, true},
		{{start, {"outputs", "s", "y"}}, {end, {"inputs", "s", "y"}}, true},
		{{start, {"outputs", "s", "z"}}, {end, {"inputs", "s", "z"}}, true}});

	context.set({start, {"uri"}}, script_2);

	checkLinks({{{start, {"outputs", "s", "x"}}, {end, {"inputs", "s", "x"}}, true},
		{{start, {"outputs", "s", "y"}}, {end, {"inputs", "s", "y"}}, false},
		{{start, {"outputs", "s", "z"}}, {end, {"inputs", "s", "z"}}, false}});
}

TEST_F(LinkTest, removal_sync_lua_struct_add_property_whole) {
	TextFile script_1 = makeFile("script-1.lua",
		R"(
function interface(IN,OUT)
	local st = { x = Type:Float(), y = Type:Float() }
	IN.s = st
	OUT.s = st
end

function run(IN,OUT)
end

)");

	TextFile script_2 = makeFile("script-2.lua",
		R"(
function interface(IN,OUT)
	local st = { x = Type:Float(), y = Type:Float(), z = Type:Int32()}
	IN.s = st
	OUT.s = st
end

function run(IN,OUT)
end

)");

	auto start = create_lua("start", script_1);
	auto end = create_lua("end", script_1);

	auto [sprop, eprop] = link(start, {"outputs", "s"}, end, {"inputs", "s"});
	checkLinks({{sprop, eprop, true}});

	context.set({start, {"uri"}}, script_2);
	checkLinks({{sprop, eprop, false}});
}

TEST_F(LinkTest, removal_sync_lua_struct_add_property_member) {
	TextFile script_1 = makeFile("script-1.lua",
		R"(
function interface(IN,OUT)
	local st = { x = Type:Float(), y = Type:Float() }
	IN.s = st
	OUT.s = st
end

function run(IN,OUT)
end

)");

	TextFile script_2 = makeFile("script-2.lua",
		R"(
function interface(IN,OUT)
	local st = { x = Type:Float(), y = Type:Int32(), z = Type:Int32()}
	IN.s = st
	OUT.s = st
end

function run(IN,OUT)
end

)");

	auto start = create_lua("start", script_1);
	auto end = create_lua("end", script_1);

	context.addLink({start, {"outputs", "s", "x"}}, {end, {"inputs", "s", "x"}});
	context.addLink({start, {"outputs", "s", "y"}}, {end, {"inputs", "s", "y"}});
	checkLinks({{{{start, {"outputs", "s", "x"}}, {end, {"inputs", "s", "x"}}, true},
		{{start, {"outputs", "s", "y"}}, {end, {"inputs", "s", "y"}}, true}}});

	context.set({start, {"uri"}}, script_2);
	checkLinks({{{{start, {"outputs", "s", "x"}}, {end, {"inputs", "s", "x"}}, true},
		{{start, {"outputs", "s", "y"}}, {end, {"inputs", "s", "y"}}, false}}});
}


TEST_F(LinkTest, removal_sync_lua_struct_member) {
	TextFile script_1 = makeFile("script-1.lua",
		R"(
function interface(IN,OUT)
	local st = { x = Type:Float(), y = Type:Float() }
	IN.s = st
	OUT.s = st
end

function run(IN,OUT)
end

)");

	TextFile script_2 = makeFile("script-2.lua",
		R"(
function interface(IN,OUT)
	IN.s = Type:Float()
	OUT.s = Type:Float()
end

function run(IN,OUT)
end

)");

	auto start = create_lua("start", script_1);
	auto end = create_lua("end", script_1);

	auto [sprop, eprop] = link(start, {"outputs", "s", "x"}, end, {"inputs", "s", "x"});
	checkLinks({{sprop, eprop, true}});

	context.set({start, {"uri"}}, script_2);
	checkLinks({{sprop, eprop, false}});
}

TEST_F(LinkTest, removal_move_start_lua_lua) {
	auto start = create_lua("start", "scripts/types-scalar.lua");
	auto end = create_lua("end", "scripts/types-scalar.lua");
	auto node = create<Node>("node");

	auto [sprop, eprop] = link(start, {"outputs", "ofloat"}, end, {"inputs", "float"});

	commandInterface.moveScenegraphChildren({start}, node);
	ASSERT_EQ(project.links().size(), 0);
}

TEST_F(LinkTest, move_scenegraph_multi_keep_link) {
	auto start = create_lua("start", "scripts/types-scalar.lua");
	auto end = create_lua("end", "scripts/types-scalar.lua");
	auto node = create<Node>("node");

	auto [sprop, eprop] = link(start, {"outputs", "ofloat"}, end, {"inputs", "float"});

	commandInterface.moveScenegraphChildren({start, end}, node);
	ASSERT_EQ(project.links().size(), 1);
}

TEST_F(LinkTest, removal_move_end_lua_lua) {
	auto start = create_lua("start", "scripts/types-scalar.lua");
	auto end = create_lua("end", "scripts/types-scalar.lua");
	auto node = create<Node>("node");
	auto meshnode = create<MeshNode>("meshnode");

	auto [sprop, eprop] = link(start, {"outputs", "ofloat"}, end, {"inputs", "float"});

	commandInterface.moveScenegraphChildren({end}, node);
	checkLinks({{sprop, eprop, true}});

	commandInterface.moveScenegraphChildren({start}, node);
	checkLinks({{sprop, eprop, true}});

	commandInterface.moveScenegraphChildren({start}, meshnode);
	checkLinks({{sprop, eprop, true}});

	commandInterface.moveScenegraphChildren({end}, nullptr);
	ASSERT_EQ(project.links().size(), 0);
}

TEST_F(LinkTest, removal_move_start_lua_mat) {
	auto lua = create_lua("base", "scripts/types-scalar.lua");
	auto material = create_material("material", "shaders/basic.vert", "shaders/basic.frag");
	auto node = create<Node>("node");

	auto [sprop, eprop] = link(lua, {"outputs", "ovector3f"}, material, {"uniforms", "u_color"});
	checkLinks({{sprop, eprop, true}});

	commandInterface.moveScenegraphChildren({lua}, node);
	ASSERT_EQ(project.links().size(), 0);
}

TEST_F(LinkTest, removal_move_middle_lua_lua_mat) {
	auto start = create_lua("start", "scripts/types-scalar.lua");
	auto end = create_lua("end", "scripts/types-scalar.lua");
	auto material = create_material("material", "shaders/basic.vert", "shaders/basic.frag");
	auto node = create<Node>("node");

	auto [sprop, iprop] = link(start, {"outputs", "ofloat"}, end, {"inputs", "float"});
	auto [oprop, eprop] = link(end, {"outputs", "ovector3f"}, material, {"uniforms", "u_color"});
	checkLinks({{sprop, iprop, true}, {oprop, eprop, true}});

	commandInterface.moveScenegraphChildren({end}, node);
	checkLinks({{sprop, iprop, true}});
}

TEST_F(LinkTest, lua_restore_after_reselecting_output_uri) {
	auto linkBase = create_lua("base", "scripts/types-scalar.lua");
	auto linkRecipient = create_lua("recipient", "scripts/SimpleScript.lua");
	auto [sprop, eprop] = link(linkBase, {"outputs", "ofloat"}, linkRecipient, {"inputs", "in_float"});
	checkLinks({{sprop, eprop, true}});

	change_uri(linkBase, "scripts/SimpleScript.lua");
	checkLinks({{sprop, eprop, false}});

	change_uri(linkBase, "scripts/types-scalar.lua");
	checkLinks({{sprop, eprop, true}});
}

TEST_F(LinkTest, lua_restore_only_when_property_types_fit) {
	TextFile leftScript1 = makeFile("leftScript1.lua",
		R"(
function interface(IN,OUT)
    OUT.a = Type:Float()
end

function run(IN,OUT)
end
)");

	TextFile leftScript2 = makeFile("leftScript2.lua",
		R"(
function interface(IN,OUT)
    OUT.a = Type:Int32()
end

function run(IN,OUT)
end
)");

	TextFile rightScript1 = makeFile("rightScript1.lua",
		R"(
function interface(IN,OUT)
    IN.b = Type:Float()
end

function run(IN,OUT)
end
)");

	TextFile rightScript2 = makeFile("rightScript2.lua",
		R"(
function interface(IN,OUT)
    IN.b = Type:Int32()
end

function run(IN,OUT)
end
)");

	auto right = create_lua("right", rightScript1);
	auto left = create_lua("left", leftScript1);

	auto [sprop, eprop] = link(left, {"outputs", "a"}, right, {"inputs", "b"});
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
	auto [sprop, eprop] = link(linkBase, {"outputs", "ofloat"}, linkRecipient, {"inputs", "in_float"});
	checkLinks({{sprop, eprop, true}});

	change_uri(linkRecipient, "scripts/types-scalar.lua");
	checkLinks({{sprop, eprop, false}});

	change_uri(linkRecipient, "scripts/SimpleScript.lua");
	checkLinks({{sprop, eprop, true}});
}

TEST_F(LinkTest, lua_restore_two_scripts_prevent_loop_when_changing_output_uri) {
	auto linkBase = create_lua("base", "scripts/types-scalar.lua");
	auto linkRecipient = create_lua("recipient", "scripts/SimpleScript.lua");
	auto [sprop, eprop] = link(linkBase, {"outputs", "ofloat"}, linkRecipient, {"inputs", "in_float"});
	checkLinks({{sprop, eprop, true}});

	change_uri(linkBase, "scripts/SimpleScript.lua");
	checkLinks({{sprop, eprop, false}});

	// Broken link hinders further links and thus loops
	auto allowed = Queries::allLinkStartProperties(project, ValueHandle(linkBase, {"outputs", "out_float"}));
	EXPECT_EQ(allowed.size(), 0);

	change_uri(linkBase, "scripts/types-scalar.lua");
	checkLinks({{sprop, eprop, true}});
}

TEST_F(LinkTest, lua_restore_three_scripts_prevent_duplicate_links_with_same_property_name) {
	TextFile rightScript = makeFile("right.lua",
		R"(
function interface(IN,OUT)
	IN.a = Type:Float()
end

function run(IN,OUT)
end

)");

	TextFile leftScript1 = makeFile("left-1.lua",
		R"(
function interface(IN,OUT)
	OUT.b = Type:Float()
end

function run(IN,OUT)
end

)");

	TextFile leftScript2 = makeFile("left-2.lua",
		R"(
function interface(IN,OUT)
	OUT.c = Type:Float()
end

function run(IN,OUT)
end

)");

	TextFile leftScript3 = makeFile("left-3.lua",
		R"(
function interface(IN,OUT)
	OUT.b = Type:Float()
	OUT.c = Type:Float()
end

function run(IN,OUT)
end

)");

	auto start = create_lua("start", leftScript1);
	auto end = create_lua("mid", rightScript);

	commandInterface.addLink({start, {"outputs", "b"}}, {end, {"inputs", "a"}});
	checkLinks({{{start, {"outputs", "b"}}, {end, {"inputs", "a"}}, true}});

	commandInterface.set({start, {"uri"}}, leftScript2);
	checkLinks({{{start, {"outputs", "b"}}, {end, {"inputs", "a"}}, false}});

	commandInterface.addLink({start, {"outputs", "c"}}, {end, {"inputs", "a"}});
	// BaseContext::addLink() replaces broken link with new link
	checkLinks({{{start, {"outputs", "c"}}, {end, {"inputs", "a"}}, true}});

	commandInterface.set({start, {"uri"}}, leftScript3);
	checkLinks({{{start, {"outputs", "c"}}, {end, {"inputs", "a"}}, true}});
}

TEST_F(LinkTest, lua_restore_two_scripts_prevent_multiple_active_links_on_same_output) {
	auto linkBase = create_lua("base", "scripts/types-scalar.lua");
	auto linkRecipient = create_lua("recipient", "scripts/SimpleScript.lua");
	auto [sprop, eprop] = link(linkBase, {"outputs", "ofloat"}, linkRecipient, {"inputs", "in_float"});
	checkLinks({{sprop, eprop, true}});

	change_uri(linkRecipient, "scripts/types-scalar.lua");
	checkLinks({{sprop, eprop, false}});

	auto [sprop2, eprop2] = link(linkBase, {"outputs", "ofloat"}, linkRecipient, {"inputs", "float"});
	checkLinks({{sprop, eprop, false},
		{sprop2, eprop2, true}});

	change_uri(linkRecipient, "scripts/SimpleScript.lua");
	checkLinks({{sprop, eprop, true},
		{sprop2, eprop2, false}});
}

TEST_F(LinkTest, lua_restore_nested_struct_link_from_other_struct) {
	auto linkBase = create_lua("base", "scripts/struct-nested.lua");
	auto linkRecipient = create_lua("recipient", "scripts/struct-nested.lua");
	auto [sprop, eprop] = link(linkBase, {"outputs", "nested_out", "inner"}, linkRecipient, {"inputs", "nested", "inner"});
	checkLinks({{sprop, eprop, true}});

	change_uri(linkBase, "scripts/SimpleScript.lua");
	checkLinks({{sprop, eprop, false}});

	change_uri(linkBase, "scripts/struct-nested.lua");
	checkLinks({{sprop, eprop, true}});
}

TEST_F(LinkTest, lua_restore_nested_struct_link_from_empty_uri) {
	auto linkBase = create_lua("base", "scripts/struct-nested.lua");
	auto linkRecipient = create_lua("recipient", "scripts/struct-nested.lua");
	auto [sprop, eprop] = link(linkBase, {"outputs", "nested_out", "inner"}, linkRecipient, {"inputs", "nested", "inner"});
	checkLinks({{sprop, eprop, true}});

	commandInterface.set(ValueHandle{linkBase, {"uri"}}, std::string());
	checkLinks({{sprop, eprop, false}});

	change_uri(linkBase, "scripts/struct-nested.lua");
	checkLinks({{sprop, eprop, true}});
}

TEST_F(LinkTest, lua_restore_nested_struct_link_only_when_struct_property_types_match) {
	TextFile script1 = makeFile("script-1.lua",
		R"(
function interface(IN,OUT)
	local s = { x = Type:Float()}
	local complex = {
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


function run(IN,OUT)
end

)");

	TextFile script2 = makeFile("script-2.lua",
		R"(
function interface(IN,OUT)
	local s = { x = Type:Int32()}
	local complex = {
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


function run(IN,OUT)
end

)");

	auto linkBase = create_lua("base", script1);
	auto linkRecipient = create_lua("recipient", script1);
	auto [sprop, eprop] = link(linkBase, {"outputs", "nested_out", "inner", "struct"}, linkRecipient, {"inputs", "nested", "inner", "struct"});
	checkLinks({{sprop, eprop, true}});

	commandInterface.set(ValueHandle{linkBase, {"uri"}}, script2);
	checkLinks({{sprop, eprop, false}});

	commandInterface.set(ValueHandle{linkBase, {"uri"}}, script1);
	checkLinks({{sprop, eprop, true}});
}


TEST_F(LinkTest, lua_restore_nested_struct_property_link_after_reselecting_output_uri) {
	auto linkBase = create_lua("base", "scripts/types-scalar.lua");
	auto linkRecipient = create_lua("recipient", "scripts/struct-nested.lua");
	auto [sprop, eprop] = link(linkBase, {"outputs", "ovector4f"}, linkRecipient, {"inputs", "nested", "inner", "vector4f"});
	checkLinks({{sprop, eprop, true}});

	change_uri(linkBase, "scripts/SimpleScript.lua");
	checkLinks({{sprop, eprop, false}});

	change_uri(linkBase, "scripts/types-scalar.lua");
	checkLinks({{sprop, eprop, true}});
}

TEST_F(LinkTest, lua_restore_nested_struct_property_link_after_reselecting_input_uri) {
	auto linkBase = create_lua("base", "scripts/types-scalar.lua");
	auto linkRecipient = create_lua("recipient", "scripts/struct-nested.lua");
	auto [sprop, eprop] = link(linkBase, {"outputs", "ovector4f"}, linkRecipient, {"inputs", "nested", "inner", "vector4f"});
	checkLinks({{sprop, eprop, true}});

	change_uri(linkRecipient, "scripts/SimpleScript.lua");
	checkLinks({{sprop, eprop, false}});

	change_uri(linkRecipient, "scripts/struct-nested.lua");
	checkLinks({{sprop, eprop, true}});
}

TEST_F(LinkTest, lua_restore_struct_link_from_other_struct) {
	auto linkBase = create_lua("start", "scripts/struct-simple.lua");
	auto linkRecipient = create_lua("end", "scripts/struct-simple.lua");
	auto [sprop, eprop] = link(linkBase, {"outputs", "s"}, linkRecipient, {"inputs", "s"});
	checkLinks({{sprop, eprop, true}});

	change_uri(linkBase, "scripts/struct-nested.lua");
	checkLinks({{sprop, eprop, false}});

	change_uri(linkBase, "scripts/struct-simple.lua");
	checkLinks({{sprop, eprop, true}});
}

TEST_F(LinkTest, lua_restore_struct_link_from_empty_uri) {
	auto linkBase = create_lua("start", "scripts/struct-simple.lua");
	auto linkRecipient = create_lua("end", "scripts/struct-simple.lua");
	auto [sprop, eprop] = link(linkBase, {"outputs", "s"}, linkRecipient, {"inputs", "s"});
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
	auto [sprop, eprop] = link(linkBase, {"outputs", "ofloat"}, linkRecipient, {"inputs", "in_float"});
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

	auto ofloat = PropertyDescriptor{linkBase, {"outputs", "ofloat"}};
	auto ovector3f = PropertyDescriptor{linkBase, {"outputs", "ovector3f"}};
	auto in_float = PropertyDescriptor{simpleScript, {"inputs", "in_float"}};
	auto s_vector3f = PropertyDescriptor{structSimple, {"inputs", "s", "vector3f"}};
	auto rotation = PropertyDescriptor{node, {"rotation"}};

	commandInterface.addLink(ValueHandle(ofloat), ValueHandle(in_float));
	commandInterface.addLink(ValueHandle(ovector3f), ValueHandle(s_vector3f));
	commandInterface.addLink(ValueHandle(ovector3f), ValueHandle(rotation));
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
function interface(IN,OUT)
	IN.v = Type:Float()
	IN.flag = Type:Bool()
	OUT.v = Type:Float()
	OUT.flag = Type:Bool()
end

function run(IN,OUT)
end

)");

	TextFile scriptBroken = makeFile("script-2.lua",
		R"(
function interface(IN,OUT)
	IN.v = Type:Float()a
	IN.flag = Type:Bool()
	OUT.v = Type:Float()
	OUT.flag = Type:Bool()
ed

function run(IN,OUT)

)");

	auto start = create_lua("base", script);
	auto end = create_lua("recipient", script);

	auto [sprop, eprop] = link(start, {"outputs", "v"}, end, {"inputs", "v"});
	checkLinks({{sprop, eprop, true}});

	commandInterface.set({start, {"uri"}}, scriptBroken);
	checkLinks({{sprop, eprop, false}});

	commandInterface.set({start, {"uri"}}, script);
	checkLinks({{sprop, eprop, true}});
}

TEST_F(LinkTest, lua_restore_link_chain_from_two_broken_scripts) {
	TextFile script1 = makeFile("script-1.lua",
		R"(
function interface(IN,OUT)
	OUT.f = Type:Float()
end

function run(IN,OUT)
end

)");

	TextFile script1Broken = makeFile("script-1-broken.lua",
		R"(
function interface(IN,OUT)
	OUT.f = Type:Float()a
end

function run(IN,OUT)
end

)");

	TextFile script2 = makeFile("script-2.lua",
		R"(
function interface(IN,OUT)
	IN.f = Type:Float()
	OUT.vec = Type:Vec3f()
end

function run(IN,OUT)
end

)");

	TextFile script2Broken = makeFile("script-2-broken.lua",
		R"(
function interface(IN,OUT)
	IN.f = FLOTA
	OUT.vec = VEC3
end

function run(IN,OUT)
end

)");

	auto start = create_lua("start", script1);
	auto mid = create_lua("mid", script2);
	auto end = create<Node>("end");

	commandInterface.addLink({start, {"outputs", "f"}}, {mid, {"inputs", "f"}});
	commandInterface.addLink({mid, {"outputs", "vec"}}, {end, {"translation"}});

	commandInterface.set({start, {"uri"}}, script1Broken);
	commandInterface.set({mid, {"uri"}}, script2Broken);
	checkLinks({{{start, {"outputs", "f"}}, {mid, {"inputs", "f"}}, false},
		{{mid, {"outputs", "vec"}}, {end, {"translation"}}, false}});

	commandInterface.set({mid, {"uri"}}, script2);
	commandInterface.set({start, {"uri"}}, script1);
	checkLinks({{{start, {"outputs", "f"}}, {mid, {"inputs", "f"}}},
		{{mid, {"outputs", "vec"}}, {end, {"translation"}}}});
}

TEST_F(LinkTest, material_restore_uniform_links) {
	auto luaScript = create_lua("base", "scripts/types-scalar.lua");
	auto mesh = create_mesh("mesh", "meshes/Duck.glb");
	auto material = create_material("material", "shaders/basic.vert", "shaders/basic.frag");
	auto meshNode = create_meshnode("meshnode", mesh, material);
	auto emptyMaterial = create<Material>("emptymat");
	commandInterface.set(meshNode->getMaterialPrivateHandle(0), true);

	auto [sprop, eprop] = link(luaScript, {"outputs", "ovector3f"}, meshNode, {"materials", "material", "uniforms", "u_color"});
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

	auto [sprop, eprop] = link(luaScript, {"outputs", "ovector3f"}, meshNode, {"materials", "material", "uniforms", "u_color"});
	checkLinks({{sprop, eprop, true}});

	commandInterface.set(meshNode->getMaterialPrivateHandle(0), false);
	checkLinks({{sprop, eprop, false}});

	commandInterface.set(meshNode->getMaterialPrivateHandle(0), true);
	checkLinks({{sprop, eprop, true}});
}

TEST_F(LinkTest, lua_to_quaternion) {
	auto luaScript = create_lua("base", "scripts/types-scalar.lua");
	auto node = create<raco::user_types::Node>("node", nullptr);

	auto [sprop, eprop] = link(luaScript, {"outputs", "ovector4f"}, node, {"rotation"});
	checkLinks({{sprop, eprop, true}});
}

TEST_F(LinkTest, lua_to_euler_after_quaternion) {
	auto luaScript = create_lua("base", "scripts/types-scalar.lua");
	auto node = create<raco::user_types::Node>("node", nullptr);

	auto [sprop, eprop] = link(luaScript, {"outputs", "ovector4f"}, node, {"rotation"});
	auto [sprop2, eprop2] = link(luaScript, {"outputs", "ovector3f"}, node, {"rotation"});
	checkLinks({{sprop2, eprop2, true}});
}

TEST_F(LinkTest, remove_link_keeps_value_with_undo_redo) {
	raco::ramses_base::HeadlessEngineBackend backend{raco::ramses_base::BaseEngineBackend::maxFeatureLevel};
	raco::application::RaCoApplication app{backend};
	auto& cmd = *app.activeRaCoProject().commandInterface();

	auto start = create_lua(cmd, "lua_start", "scripts/SimpleScript.lua");
	auto end = create_lua(cmd, "lua_end", "scripts/SimpleScript.lua");

	cmd.set({start, {"inputs", "in_float"}}, 2.0);
	cmd.set({end, {"inputs", "in_float"}}, 42.0);

	PropertyDescriptor sprop{start, {"outputs", "out_float"}};
	PropertyDescriptor eprop{end, {"inputs", "in_float"}};
	EXPECT_EQ(ValueHandle(eprop).asDouble(), 42.0);

	cmd.addLink(ValueHandle(sprop), ValueHandle(eprop));
	checkLinks(*app.activeRaCoProject().project(), {{sprop, eprop, true}});
	app.doOneLoop();
	EXPECT_EQ(ValueHandle(eprop).asDouble(), 2.0);

	cmd.removeLink(eprop);
	checkLinks(*app.activeRaCoProject().project(), {});
	app.doOneLoop();
	EXPECT_EQ(ValueHandle(eprop).asDouble(), 2.0);

	cmd.undoStack().undo();
	app.doOneLoop();
	EXPECT_EQ(ValueHandle(eprop).asDouble(), 2.0);

	cmd.undoStack().undo();
	app.doOneLoop();
	EXPECT_EQ(ValueHandle(eprop).asDouble(), 42.0);

	cmd.undoStack().redo();
	app.doOneLoop();
	EXPECT_EQ(ValueHandle(eprop).asDouble(), 2.0);

	cmd.undoStack().redo();
	app.doOneLoop();
	EXPECT_EQ(ValueHandle(eprop).asDouble(), 2.0);
}


TEST_F(LinkTest, break_link_keeps_value_with_undo_redo) {
	raco::ramses_base::HeadlessEngineBackend backend{raco::ramses_base::BaseEngineBackend::maxFeatureLevel};
	raco::application::RaCoApplication app{backend};
	auto& cmd = *app.activeRaCoProject().commandInterface();

	auto start = create_lua(cmd, "lua_start", "scripts/SimpleScript.lua");
	auto end = create_lua(cmd, "lua_end", "scripts/SimpleScript.lua");

	cmd.set({start, {"inputs", "in_float"}}, 2.0);
	cmd.set({end, {"inputs", "in_float"}}, 42.0);

	PropertyDescriptor sprop{start, {"outputs", "out_float"}};
	PropertyDescriptor eprop{end, {"inputs", "in_float"}};
	EXPECT_EQ(ValueHandle(eprop).asDouble(), 42.0);

	cmd.addLink(ValueHandle(sprop), ValueHandle(eprop));
	checkLinks(*app.activeRaCoProject().project(), {{sprop, eprop, true}});
	app.doOneLoop();
	EXPECT_EQ(ValueHandle(eprop).asDouble(), 2.0);

	cmd.set({start, {"uri"}}, std::string());
	checkLinks(*app.activeRaCoProject().project(), {{sprop, eprop, false}});
	app.doOneLoop();
	EXPECT_EQ(ValueHandle(eprop).asDouble(), 2.0);

	cmd.undoStack().undo();
	app.doOneLoop();
	EXPECT_EQ(ValueHandle(eprop).asDouble(), 2.0);

	cmd.undoStack().undo();
	app.doOneLoop();
	EXPECT_EQ(ValueHandle(eprop).asDouble(), 42.0);

	cmd.undoStack().redo();
	app.doOneLoop();
	EXPECT_EQ(ValueHandle(eprop).asDouble(), 2.0);

	cmd.undoStack().redo();
	app.doOneLoop();
	EXPECT_EQ(ValueHandle(eprop).asDouble(), 2.0);
}

TEST_F(LinkTest, break_link_remove_child_prop_keeps_value_with_undo_redo) {
	raco::ramses_base::HeadlessEngineBackend backend{raco::ramses_base::BaseEngineBackend::maxFeatureLevel};
	raco::application::RaCoApplication app{backend};
	auto& cmd = *app.activeRaCoProject().commandInterface();

	TextFile script_1 = makeFile("script-1.lua",
		R"(
function interface(IN,OUT)
	local st = { x = Type:Float(), y = Type:Float() }
	IN.s = st
	OUT.s = st
end

function run(IN,OUT)
	OUT.s = IN.s
end

)");

	TextFile script_2 = makeFile("script-2.lua",
		R"(
function interface(IN,OUT)
	local st = { x = Type:Float()}
	IN.s = st
	OUT.s = st
end

function run(IN,OUT)
	OUT.s = IN.s
end

)");

	auto start = create_lua(cmd, "start", script_1);
	auto end = create_lua(cmd, "end", script_1);

	cmd.set({start, {"inputs", "s", "x"}}, 2.0);
	cmd.set({end, {"inputs", "s", "x"}}, 42.0);

	PropertyDescriptor sprop{start, {"outputs", "s"}};
	PropertyDescriptor eprop{end, {"inputs", "s"}};
	EXPECT_EQ(ValueHandle(end, {"inputs", "s", "x"}).asDouble(), 42.0);

	cmd.addLink(ValueHandle(sprop), ValueHandle(eprop));
	checkLinks(*app.activeRaCoProject().project(), {{sprop, eprop, true}});
	app.doOneLoop();
	EXPECT_EQ(ValueHandle(end, {"inputs", "s", "x"}).asDouble(), 2.0);

	cmd.set({start, {"uri"}}, script_2);
	checkLinks(*app.activeRaCoProject().project(), {{sprop, eprop, false}});
	app.doOneLoop();
	EXPECT_EQ(ValueHandle(end, {"inputs", "s", "x"}).asDouble(), 2.0);

	cmd.undoStack().undo();
	app.doOneLoop();
	EXPECT_EQ(ValueHandle(end, {"inputs", "s", "x"}).asDouble(), 2.0);

	cmd.undoStack().undo();
	app.doOneLoop();
	EXPECT_EQ(ValueHandle(end, {"inputs", "s", "x"}).asDouble(), 42.0);

	cmd.undoStack().redo();
	app.doOneLoop();
	EXPECT_EQ(ValueHandle(end, {"inputs", "s", "x"}).asDouble(), 2.0);

	cmd.undoStack().redo();
	app.doOneLoop();
	EXPECT_EQ(ValueHandle(end, {"inputs", "s", "x"}).asDouble(), 2.0);
}

TEST_F(LinkTest, dont_crash_when_object_is_deleted_after_property_with_link_was_removed) {
	auto luaScript = create_lua("base", "scripts/types-scalar.lua");
	auto mesh = create_mesh("mesh", "meshes/Duck.glb");
	auto material = create_material("material", "shaders/basic.vert", "shaders/basic.frag");
	auto meshNode = create_meshnode("meshnode", mesh, material);
	auto emptyMaterial = create<Material>("emptymat");
	commandInterface.set(meshNode->getMaterialPrivateHandle(0), true);

	auto [sprop, eprop] = link(luaScript, {"outputs", "ovector3f"}, meshNode, {"materials", "material", "uniforms", "u_color"});
	checkLinks({{sprop, eprop, true}});

	commandInterface.set(ValueHandle{meshNode, {"materials", "material", "material"}}, emptyMaterial);
	checkLinks({{sprop, eprop, false}});

	// Delete the object - this caused a crash in RAOS-682
	commandInterface.deleteObjects({meshNode});
}

TEST_F(LinkTest, timer_link) {
	raco::ramses_base::HeadlessEngineBackend backend{raco::ramses_base::BaseEngineBackend::maxFeatureLevel};
	raco::application::RaCoApplication app{backend};
	auto& cmd = *app.activeRaCoProject().commandInterface();

	TextFile script = makeFile("script.lua",
		R"(
function interface(IN, OUT)
	IN.integer64 = Type:Int64()
	OUT.ointeger64 = Type:Int64()
end
function run(IN, OUT)
	OUT.ointeger64 = 2*IN.integer64
end
)");

	auto timer = create<Timer>(cmd, "timer");
	app.doOneLoop();

	auto luaScript = create_lua(cmd, "base", script);
	app.doOneLoop();

	PropertyDescriptor sprop{timer, {"outputs", "ticker_us"}};
	PropertyDescriptor eprop{luaScript, {"inputs", "integer64"}};

	cmd.addLink(ValueHandle(sprop), ValueHandle(eprop));
	checkLinks(*app.activeRaCoProject().project(), {{sprop, eprop, true}});

	app.doOneLoop();

	checkLinks(*app.activeRaCoProject().project(), {{sprop, eprop, true}});
	ASSERT_NE(ValueHandle(luaScript, {"inputs", "integer64"}).asInt64(), int64_t{0});

	cmd.set({timer, {"inputs", "ticker_us"}}, int64_t{1});
	app.doOneLoop();

	checkLinks(*app.activeRaCoProject().project(), {{sprop, eprop, true}});
	ASSERT_EQ(ValueHandle(luaScript, {"inputs", "integer64"}).asInt64(), int64_t{1});

	app.doOneLoop();
	ASSERT_EQ(ValueHandle(luaScript, {"inputs", "integer64"}).asInt64(), int64_t{1});

	cmd.set({timer, {"inputs", "ticker_us"}}, int64_t{0});
	app.doOneLoop();

	checkLinks(*app.activeRaCoProject().project(), {{sprop, eprop, true}});
	ASSERT_NE(ValueHandle(luaScript, {"inputs", "integer64"}).asInt64(), int64_t{0});
}

TEST_F(LinkTest, timer_link_to_input) {
	raco::ramses_base::HeadlessEngineBackend backend{raco::ramses_base::BaseEngineBackend::maxFeatureLevel};
	raco::application::RaCoApplication app{backend};
	auto& cmd = *app.activeRaCoProject().commandInterface();

	TextFile script = makeFile("script.lua",
		R"(
function interface(IN, OUT)
	IN.integer64 = Type:Int64()
	OUT.ointeger64 = Type:Int64()
end
function run(IN, OUT)
	OUT.ointeger64 = 2*IN.integer64
end
)");

	auto timer = create<Timer>(cmd, "timer");
	app.doOneLoop();

	auto luaScript = create_lua(cmd, "base", script);
	app.doOneLoop();

	PropertyDescriptor eprop{timer, {"inputs", "ticker_us"}};
	PropertyDescriptor sprop{luaScript, {"outputs", "ointeger64"}};

	cmd.addLink(ValueHandle(sprop), ValueHandle(eprop));
	checkLinks(*app.activeRaCoProject().project(), {{sprop, eprop, true}});

	cmd.set({luaScript, {"inputs", "integer64"}}, int64_t{22});
	app.doOneLoop();

	ASSERT_EQ(ValueHandle(timer, {"inputs", "ticker_us"}).asInt64(), int64_t{44});
	ASSERT_EQ(ValueHandle(timer, {"outputs", "ticker_us"}).asInt64(), int64_t{44});
}

TEST_F(LinkTest, timer_link_different_type) {
	raco::ramses_base::HeadlessEngineBackend backend{raco::ramses_base::BaseEngineBackend::maxFeatureLevel};
	raco::application::RaCoApplication app{backend};
	auto& cmd = *app.activeRaCoProject().commandInterface();

	TextFile script = makeFile("script.lua",
		R"(
function interface(IN, OUT)
	IN.integer64 = Type:Int64()
	OUT.ointeger64 = Type:Int64()
end
function run(IN, OUT)
	OUT.ointeger64 = 2*IN.integer64
end
)");

	TextFile script2 = makeFile("script2.lua",
		R"(
function interface(IN, OUT)
	IN.integer64 = Type:Float()
	OUT.ointeger64 = Type:Float()
end
function run(IN, OUT)
	OUT.ointeger64 = 2*IN.integer64
end
)");

	auto timer = create<Timer>(cmd, "timer");
	app.doOneLoop();

	auto luaScript = create_lua(cmd, "base", script);
	app.doOneLoop();

	PropertyDescriptor sprop{timer, {"outputs", "ticker_us"}};
	PropertyDescriptor eprop{luaScript, {"inputs", "integer64"}};

	cmd.addLink(ValueHandle(sprop), ValueHandle(eprop));
	checkLinks(*app.activeRaCoProject().project(), {{sprop, eprop, true}});

	app.doOneLoop();

	cmd.set({luaScript, &LuaScript::uri_}, script2);
	app.doOneLoop();

	checkLinks(*app.activeRaCoProject().project(), {{sprop, eprop, false}});

	cmd.set({luaScript, &LuaScript::uri_}, script);
	app.doOneLoop();
	checkLinks(*app.activeRaCoProject().project(), {{sprop, eprop, true}});
}

TEST_F(LinkTest, uniform_array) {
	auto mat = create_material("material", "shaders/uniform-array.vert", "shaders/uniform-array.frag");
	auto lua = create_lua("lua", "scripts/array.lua");

	auto [spropel, epropel] = link(lua, {"outputs", "float_array", "1"}, mat, {"uniforms", "fvec", "2"});
	checkLinks({{spropel, epropel, true}});

	auto [sproparr, eproparr] = link(lua, {"outputs", "float_array"}, mat, {"uniforms", "fvec"});
	checkLinks({{sproparr, eproparr, true}});
}


TEST_F(LinkTest, vec_struct) {
	TextFile script = makeFile("script.lua",
		R"(
function interface(INOUT)
	local struct_3f = { x = Type:Float(), y = Type:Float(), z = Type:Float() }
	INOUT.s3f = struct_3f
	INOUT.v3f = Type:Vec3f()

	local struct_3i = { i1 = Type:Int32(), i2 = Type:Int32(), i3 = Type:Int32() }
	INOUT.s3i = struct_3i
	INOUT.v3i = Type:Vec3i()
end
)");

	auto start = create_lua_interface("start", script);
	auto end = create_lua_interface("end", script);

	auto allowed_vec_3f = Queries::allLinkStartProperties(project, ValueHandle(end, {"inputs", "v3f"}));
	std::set<std::tuple<ValueHandle, bool, bool>> ref_allowed_vec_3f{
		{{start, {"inputs", "v3f"}}, true, false}};
	EXPECT_EQ(allowed_vec_3f, ref_allowed_vec_3f);

	auto allowed_vec_3i = Queries::allLinkStartProperties(project, ValueHandle(end, {"inputs", "v3i"}));
	std::set<std::tuple<ValueHandle, bool, bool>> ref_allowed_vec_3i{
		{{start, {"inputs", "v3i"}}, true, false}};
	EXPECT_EQ(allowed_vec_3i, ref_allowed_vec_3i);


	auto allowed_struct_3f  = Queries::allLinkStartProperties(project, ValueHandle(end, {"inputs", "s3f"}));
	std::set<std::tuple<ValueHandle, bool, bool>> ref_allowed_struct_3f{
		{{start, {"inputs", "s3f"}}, true, false}};
	EXPECT_EQ(allowed_struct_3f, ref_allowed_struct_3f);

	auto allowed_struct_3i = Queries::allLinkStartProperties(project, ValueHandle(end, {"inputs", "s3i"}));
	std::set<std::tuple<ValueHandle, bool, bool>> ref_allowed_struct_3i{
		{{start, {"inputs", "s3i"}}, true, false}};
	EXPECT_EQ(allowed_struct_3i, ref_allowed_struct_3i);
}
