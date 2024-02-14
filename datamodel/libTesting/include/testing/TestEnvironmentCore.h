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

#include "RacoBaseTest.h"
#include "core/ChangeRecorder.h"
#include "core/Context.h"
#include "core/EditorObject.h"
#include "core/Errors.h"
#include "core/Link.h"
#include "core/MeshCacheInterface.h"
#include "core/Project.h"
#include "core/PropertyDescriptor.h"
#include "core/Undo.h"
#include "core/ProjectSettings.h"
#include "core/Queries.h"
#include "core/UserObjectFactoryInterface.h"
#include "log_system/log.h"
#include "ramses_base/HeadlessEngineBackend.h"
#include "components/FileChangeMonitorImpl.h"
#include "components/MeshCacheImpl.h"
#include "user_types/UserObjectFactory.h"
#include "utils/FileUtils.h"
#include "user_types/Mesh.h"
#include "user_types/MeshNode.h"
#include "user_types/Material.h"
#include "user_types/LuaScript.h"
#include "user_types/LuaScriptModule.h"
#include "user_types/LuaInterface.h"
#include "user_types/RenderLayer.h"
#include "user_types/Skin.h"
#include "user_types/Texture.h"
#include "user_types/UserObjectFactory.h"
#include "user_types/PrefabInstance.h"
#include "user_types/Enumerations.h"

#include "testing/MockUserTypes.h"

#include <QCoreApplication>

#include <filesystem>

#include <chrono>
#include <memory>

inline void clearQEventLoop() {
	int argc = 0;
	QCoreApplication eventLoop_{argc, nullptr};
	QCoreApplication::processEvents();
}


class TestUndoStack : public core::UndoStack {
public:
	using Entry = core::UndoStack::Entry;

	std::vector<std::unique_ptr<Entry>>& stack() {
		return stack_;
	}
};

class TestObjectFactory : public user_types::UserObjectFactory {
public:
	TestObjectFactory() : UserObjectFactory() {}

	static TestObjectFactory& getInstance() {
		using namespace raco::data_storage;

		static TestObjectFactory* instance = nullptr;
		if (!instance) {
			instance = new TestObjectFactory();
			instance->addType<user_types::Foo>();
			instance->addType<user_types::MockTableObject>();
			instance->addType<user_types::ObjectWithTableProperty>();
			instance->addType<user_types::ObjectWithStructProperty>();
			instance->addType<user_types::ObjectWithArrays>();
			instance->addType<user_types::FutureType>();

			instance->addProperty<Value<Array<double>>>();
			instance->addProperty<Value<Array<Array<double>>>>();
			instance->addProperty<Value<Array<user_types::SNode>>>();
		}
		return *instance;
	}
};


template <class BaseClass = ::testing::Test>
struct TestEnvironmentCoreT : public RacoBaseTest<BaseClass> {
	using BaseContext = core::BaseContext;
	using Project = core::Project;
	using Errors = core::Errors;
	using UserObjectFactoryInterface = core::UserObjectFactoryInterface;
	using UserObjectFactory = user_types::UserObjectFactory;
	using DataChangeRecorderInterface = core::DataChangeRecorderInterface;

	UserObjectFactoryInterface* objectFactory() {
		return context.objectFactory();
	};

	template <class C>
	std::shared_ptr<C> create(std::string name, core::SEditorObject parent = nullptr, const std::vector<std::string>& tags = {}) {
		auto obj = std::dynamic_pointer_cast<C>(commandInterface.createObject(C::typeDescription.typeName, name));
		if (parent) {
			commandInterface.moveScenegraphChildren({obj}, parent);
		}
		if (!tags.empty()) {
			context.set({obj, {"tags"}}, tags);
		}
		return obj;
	}

	template <class C>
	std::shared_ptr<C> create(core::CommandInterface& cmd, std::string name, core::SEditorObject parent = nullptr) {
		auto obj = std::dynamic_pointer_cast<C>(cmd.createObject(C::typeDescription.typeName, name));
		if (parent) {
			cmd.moveScenegraphChildren({obj}, parent);
		}
		return obj;
	}

	user_types::SMesh create_mesh(const std::string& name, const std::string& relpath) {
		auto mesh = create<user_types::Mesh>(name);
		commandInterface.set({mesh, {"uri"}}, (RacoBaseTest<BaseClass>::test_path() / relpath).string());
		return mesh;
	}

	user_types::SMaterial create_material(const std::string& name, const std::string& relpathVertex, const std::string& relpathFragment, const std::string& relpathGeometry = std::string()) {
		auto material = create<user_types::Material>(name);
		if (relpathGeometry.length()) {
			commandInterface.set({material, {"uriGeometry"}}, (RacoBaseTest<BaseClass>::test_path() / relpathGeometry).string());
		}
		commandInterface.set({material, {"uriVertex"}}, (RacoBaseTest<BaseClass>::test_path() / relpathVertex).string());
		commandInterface.set({material, {"uriFragment"}}, (RacoBaseTest<BaseClass>::test_path() / relpathFragment).string());
		return material;
	}

	user_types::SMeshNode create_meshnode(const std::string& name, user_types::SMesh mesh, user_types::SMaterial material, user_types::SEditorObject parent = nullptr) {
		auto meshnode = create<user_types::MeshNode>(name, parent);
		commandInterface.set({meshnode, {"mesh"}}, mesh);
		commandInterface.set({meshnode, {"materials", "material", "material"}}, material);
		return meshnode;
	}

	user_types::SLuaScript create_lua(core::CommandInterface& cmd, const std::string& name, const std::string& relpath, core::SEditorObject parent = nullptr) {
		auto lua = create<user_types::LuaScript>(cmd, name, parent);
		cmd.set({lua, {"uri"}}, (RacoBaseTest<BaseClass>::test_path() / relpath).string());
		return lua;
	}

	user_types::SLuaScript create_lua(const std::string& name, const std::string& relpath, core::SEditorObject parent = nullptr) {
		return create_lua(commandInterface, name, relpath, parent);
	}

	user_types::SLuaScript create_lua(core::CommandInterface& cmd, const std::string& name, const typename RacoBaseTest<BaseClass>::TextFile& file) {
		auto lua = create<user_types::LuaScript>(cmd, name);
		cmd.set({lua, {"uri"}}, file);
		return lua;
	}

	user_types::SLuaScript create_lua(const std::string& name, const typename 
		RacoBaseTest<BaseClass>::TextFile& file) {
		return create_lua(commandInterface, name, file);
	}

	user_types::SLuaScriptModule create_lua_module(const std::string& name, const std::string& relpath, core::SEditorObject parent = nullptr) {
		auto module = create<user_types::LuaScriptModule>(commandInterface, name, parent);
		commandInterface.set({module, {"uri"}}, (RacoBaseTest<BaseClass>::test_path() / relpath).string());
		return module;
	}

	user_types::SLuaInterface create_lua_interface(const std::string& name, const std::string& relpath, core::SEditorObject parent = nullptr) {
		auto interface = create<user_types::LuaInterface>(commandInterface, name, parent);
		commandInterface.set({interface, {"uri"}}, (RacoBaseTest<BaseClass>::test_path() / relpath).string());
		return interface;
	}

	user_types::SLuaInterface create_lua_interface(const std::string& name, const typename RacoBaseTest<BaseClass>::TextFile& file, core::SEditorObject parent = nullptr) {
		auto interface = create<user_types::LuaInterface>(commandInterface, name, parent);
		commandInterface.set({interface, {"uri"}}, file);
		return interface;
	}

	user_types::SRenderLayer create_layer(const std::string& name,
		const std::vector<std::string>& tags = {},
		const std::vector<std::pair<std::string, int>>& renderables = {},
		const std::vector<std::string>& matFilterTags = {},
		bool filterExclusive = true) {
		auto layer = create<user_types::RenderLayer>(name, nullptr, tags);
		for (int index = 0; index < renderables.size(); index++) {
			context.addProperty({layer, {"renderableTags"}}, renderables[index].first, std::make_unique<data_storage::Value<int>>(renderables[index].second));
		}
		if (!matFilterTags.empty()) {
			context.set({layer, &user_types::RenderLayer::materialFilterTags_}, matFilterTags);
		}
		context.set({layer, &user_types::RenderLayer::materialFilterMode_},
			static_cast<int>(filterExclusive ? user_types::ERenderLayerMaterialFilterMode::Exclusive : user_types::ERenderLayerMaterialFilterMode::Inclusive));

		return layer;
	}

	user_types::SSkin create_skin(const std::string& name,
		const std::string& relpath,
		int skinIndex,
		user_types::SMeshNode meshnode,
		const std::vector<user_types::SNode>& nodes) {
		auto skin = create<user_types::Skin>(name);
		commandInterface.set(core::ValueHandle(skin, &user_types::Skin::targets_)[0], meshnode);
		commandInterface.set({skin, &user_types::Skin::uri_}, (RacoBaseTest<BaseClass>::test_path() / relpath).string());
		commandInterface.set({skin, &user_types::Skin::skinIndex_}, skinIndex);
		for (int index = 0; index < nodes.size(); index++) {
			commandInterface.set(core::ValueHandle(skin, &user_types::Skin::joints_)[index], nodes[index]);	
		}
		return skin;
	}

	user_types::STexture create_texture(const std::string& name, const std::string& relpath) {
		auto texture = create<user_types::Texture>(name);
		commandInterface.set({texture, {"uri"}}, (RacoBaseTest<BaseClass>::test_path() / relpath).string());
		return texture;
	}

	user_types::SPrefabInstance create_prefabInstance(core::CommandInterface& cmd, const std::string& name, user_types::SPrefab prefab, user_types::SEditorObject parent = nullptr) {
		auto inst = create<user_types::PrefabInstance>(cmd, name, parent);
		cmd.set({inst, {"template"}}, prefab);
		return inst;
	}

	user_types::SPrefabInstance create_prefabInstance(const std::string& name, user_types::SPrefab prefab, user_types::SEditorObject parent = nullptr) {
		return create_prefabInstance(commandInterface, name, prefab, parent);
	}

	void checkUndoRedo(std::function<void()> operation, std::function<void()> preCheck, std::function<void()> postCheck) {
		RacoBaseTest<BaseClass>::checkUndoRedo(commandInterface, operation, preCheck, postCheck);
	}

	template <std::size_t N>
	void checkUndoRedoMultiStep(std::array<std::function<void()>, N> operations, std::array<std::function<void()>, N + 1> checks) {
		RacoBaseTest<BaseClass>::checkUndoRedoMultiStep(commandInterface, operations, checks);
	}

	std::pair<core::PropertyDescriptor, core::PropertyDescriptor> link(user_types::SEditorObject startObj, std::vector<std::string> startProp, user_types::SEditorObject endObj, std::vector<std::string> endProp, bool isWeak = false) {
		core::PropertyDescriptor start{startObj, startProp};
		core::PropertyDescriptor end{endObj, endProp};
		commandInterface.addLink(core::ValueHandle(start), core::ValueHandle(end), isWeak);
		return {start, end};
	}

	static void checkLinks(core::Project& project, const std::vector<core::Link>& refLinks) {
		RacoBaseTest<BaseClass>::checkLinks(project, refLinks);
	}

	void checkLinks(const std::vector<core::Link>& refLinks) {
		checkLinks(project, refLinks);
	}

	void assertOperationTimeIsBelow(testing::TimeInMillis maxMs, const std::function<void()>& operation) {
		auto startTime = std::chrono::steady_clock::now();
		operation();
		auto endTime = std::chrono::steady_clock::now();
		auto opTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
		ASSERT_LE(opTimeMs, maxMs) << "Operation took longer than allowed boundary of " << maxMs << " ms\nActual operation duration: " << opTimeMs << " ms";
	}

	TestEnvironmentCoreT(UserObjectFactoryInterface* objectFactory = &UserObjectFactory::getInstance(), ramses::EFeatureLevel featureLevel = ramses_base::BaseEngineBackend::maxFeatureLevel)
		: backend{},
		  meshCache{},
		  project{},
		  recorder{},
		  errors{&recorder},
		  context{&project, backend.coreInterface(), objectFactory, &recorder, &errors},
		  undoStack{&context},
		  commandInterface{&context, &undoStack} {
		spdlog::drop_all();
		log_system::init();
		clearQEventLoop();
		context.setMeshCache(&meshCache);
		auto settings = context.createObject(core::ProjectSettings::typeDescription.typeName, "ProjectSettings");
		context.set({settings, &core::ProjectSettings::featureLevel_}, static_cast<int>(featureLevel));
		undoStack.reset();
		context.changeMultiplexer().reset();
		backend.setFeatureLevel(featureLevel);
	}

	virtual ~TestEnvironmentCoreT() {
		// Cleanup everything
		for (const auto& instance : context.project()->instances()) {
			instance->onBeforeDeleteObject(context);
		}
		clearQEventLoop();
	}


	ramses_base::HeadlessEngineBackend backend;
	components::MeshCacheImpl meshCache;
	Project project;
	core::DataChangeRecorder recorder;
	Errors errors;
	BaseContext context;
	TestUndoStack undoStack;
	core::CommandInterface commandInterface;
};

using TestEnvironmentCore = TestEnvironmentCoreT<>;
