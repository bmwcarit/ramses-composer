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
#include "testing/TestEnvironmentCore.h"
#include "user_types/Animation.h"
#include "user_types/AnimationChannel.h"
#include "user_types/LuaScript.h"
#include "user_types/Node.h"
#include "utils/FileUtils.h"
#include <QCoreApplication>
#include <fstream>
#include <gtest/gtest.h>
#include <thread>
#include <tuple>

namespace raco {

template <typename T, typename I>
inline std::shared_ptr<T> select(const std::vector<I>& vec) {
	auto it = std::find_if(vec.begin(), vec.end(), [](const auto& i) {
		return std::dynamic_pointer_cast<T>(i);
	});
	if (it != vec.end()) {
		return std::dynamic_pointer_cast<T>(*it);
	}
	return {};
}

template <typename T, typename I>
inline std::shared_ptr<T> select(const std::vector<I>& vec, std::string_view name) {
	auto it = std::find_if(vec.begin(), vec.end(), [name](const auto& obj) {
		return obj->objectName() == name && std::dynamic_pointer_cast<T>(obj);
	});
	if (it != vec.end()) {
		return std::dynamic_pointer_cast<T>(*it);
	}
	return {};
}

inline auto createLinkedScene(raco::core::CommandInterface& context, const raco::utils::u8path& path) {
	const auto luaScript{context.createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua_script")};
	const auto node{context.createObject(raco::user_types::Node::typeDescription.typeName, "node")};
	raco::utils::file::write((path / "lua_script.lua").string(), R"(
function interface(IN,OUT)
	OUT.translation = Type:Vec3f()
	OUT.rotation3 = Type:Vec3f()
	OUT.rotation4 = Type:Vec4f()
end
function run(IN,OUT)
end
	)");
	context.set({luaScript, {"uri"}}, (path / "lua_script.lua").string());
	auto link = context.addLink({luaScript, {"outputs", "translation"}}, {node, {"translation"}});
	return std::make_tuple(
		std::dynamic_pointer_cast<user_types::LuaScript>(luaScript), std::dynamic_pointer_cast<user_types::Node>(node), link);
}


inline auto createLinkedScene(raco::core::BaseContext& context, const raco::utils::u8path& path) {
	const auto luaScript{context.createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua_script", "lua_script_id")};
	const auto node{context.createObject(raco::user_types::Node::typeDescription.typeName, "node", "node_id")};
	raco::utils::file::write((path / "lua_script.lua").string(), R"(
function interface(IN,OUT)
	OUT.translation = Type:Vec3f()
	OUT.rotation3 = Type:Vec3f()
	OUT.rotation4 = Type:Vec4f()
end
function run(IN,OUT)
end
	)");
	context.set({luaScript, {"uri"}}, (path / "lua_script.lua").string());
	auto link = context.addLink({luaScript, {"outputs", "translation"}}, {node, {"translation"}});
	return std::make_tuple(
		std::dynamic_pointer_cast<user_types::LuaScript>(luaScript), std::dynamic_pointer_cast<user_types::Node>(node), link);
}

template <typename ContextOrCommandInterface>
inline auto createAnimatedScene(ContextOrCommandInterface& context, const raco::utils::u8path& path) {
	const auto anim{context.createObject(raco::user_types::Animation::typeDescription.typeName, "anim")};
	const auto animChannel{context.createObject(raco::user_types::AnimationChannel::typeDescription.typeName, "anim_ch")};
	const auto node{context.createObject(raco::user_types::Node::typeDescription.typeName, "node")};

	context.set({anim, {"animationChannels", "Channel 0"}}, animChannel);
	context.set({animChannel, {"uri"}}, (path / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string());

	auto link = context.addLink({anim, {"outputs", "Ch0.anim_ch"}}, {node, {"translation"}});
	return std::make_tuple(
		std::dynamic_pointer_cast<user_types::Animation>(anim),
		std::dynamic_pointer_cast<user_types::AnimationChannel>(animChannel),
		std::dynamic_pointer_cast<user_types::Node>(node),
		link);
}

inline bool awaitPreviewDirty(const raco::core::DataChangeRecorder& recorder, const raco::core::SEditorObject& obj, long long timeout = 5) {
	const std::chrono::steady_clock::time_point timeoutTS = std::chrono::steady_clock::now() + std::chrono::seconds{timeout};
	auto dirtyObjects{recorder.getPreviewDirtyObjects()};
	int argc = 0;
	QCoreApplication eventLoop_{argc, nullptr};
	while (std::find(dirtyObjects.begin(), dirtyObjects.end(), obj) == dirtyObjects.end()) {
		if (std::chrono::steady_clock::now() > timeoutTS) {
			assert(false && "Timeout");
			return false;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds{5});
		QCoreApplication::processEvents();
		dirtyObjects = recorder.getPreviewDirtyObjects();
	}
	return true;
}

inline void createGitLfsPlaceholderFile(const std::string& path) {
	std::fstream file;
	file.open(path, std::ios::out);
	file << "version https://git-lfs.github.com/spec/v1\n"
			"oid sha256:5a2f1df22ff0f80d375940c11cec7a2c1e9a4e7f63ba2ed325a119e0bf597a00\n"
			"size 26787";
	file.close();
}

inline std::pair<raco::core::MeshScenegraph, raco::core::FileChangeMonitor::UniqueListener> getMeshSceneGraphWithHandler(raco::core::MeshCache* meshCache, const raco::core::MeshDescriptor& descriptor) {
	auto dummyCacheEntry = meshCache->registerFileChangedHandler(descriptor.absPath, {nullptr, nullptr});
	raco::core::MeshScenegraph scenegraph{*meshCache->getMeshScenegraph(descriptor.absPath)};
	return {scenegraph, std::move(dummyCacheEntry)};
}


inline void checkVec2fValue(raco::core::ValueHandle handle, const std::array<float, 2>& value) {
	EXPECT_EQ(handle[0].asDouble(), value[0]);
	EXPECT_EQ(handle[1].asDouble(), value[1]);
}

inline void checkVec3fValue(raco::core::ValueHandle handle, const std::array<float, 3>& value) {
	EXPECT_EQ(handle[0].asDouble(), value[0]);
	EXPECT_EQ(handle[1].asDouble(), value[1]);
	EXPECT_EQ(handle[2].asDouble(), value[2]);
}

inline void checkVec4fValue(raco::core::ValueHandle handle, const std::array<float, 4>& value) {
	EXPECT_EQ(handle[0].asDouble(), value[0]);
	EXPECT_EQ(handle[1].asDouble(), value[1]);
	EXPECT_EQ(handle[2].asDouble(), value[2]);
	EXPECT_EQ(handle[3].asDouble(), value[3]);
}

inline void checkVec2iValue(raco::core::ValueHandle handle, const std::array<int32_t, 2>& value) {
	EXPECT_EQ(handle[0].asInt(), value[0]);
	EXPECT_EQ(handle[1].asInt(), value[1]);
}

inline void checkVec3iValue(raco::core::ValueHandle handle, const std::array<int32_t, 3>& value) {
	EXPECT_EQ(handle[0].asInt(), value[0]);
	EXPECT_EQ(handle[1].asInt(), value[1]);
	EXPECT_EQ(handle[2].asInt(), value[2]);
}

inline void checkVec4iValue(raco::core::ValueHandle handle, const std::array<int32_t, 4>& value) {
	EXPECT_EQ(handle[0].asInt(), value[0]);
	EXPECT_EQ(handle[1].asInt(), value[1]);
	EXPECT_EQ(handle[2].asInt(), value[2]);
	EXPECT_EQ(handle[3].asInt(), value[3]);
}

}  // namespace raco
