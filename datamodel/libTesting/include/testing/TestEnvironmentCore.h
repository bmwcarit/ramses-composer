/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
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
#include "core/Undo.h"
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

#include <QCoreApplication>

#include "utils/stdfilesystem.h"

#include <memory>

inline void clearQEventLoop() {
	int argc = 0;
	QCoreApplication eventLoop_{argc, nullptr};
	QCoreApplication::processEvents();
}

template <class BaseClass = ::testing::Test>
struct TestEnvironmentCoreT : public RacoBaseTest<BaseClass> {
	using BaseContext = raco::core::BaseContext;
	using Project = raco::core::Project;
	using Errors = raco::core::Errors;
	using FileChangeMonitorImpl = raco::components::FileChangeMonitorImpl;
	using UserObjectFactoryInterface = raco::core::UserObjectFactoryInterface;
	using UserObjectFactory = raco::user_types::UserObjectFactory;
	using DataChangeRecorderInterface = raco::core::DataChangeRecorderInterface;

	UserObjectFactoryInterface* objectFactory() {
		return &UserObjectFactory::getInstance();
	};

	template <class C>
	std::shared_ptr<C> create(std::string name, raco::core::SEditorObject parent = nullptr) {
		auto obj = std::dynamic_pointer_cast<C>(commandInterface.createObject(C::typeDescription.typeName, name));
		if (parent) {
			commandInterface.moveScenegraphChild(obj, parent);
		}
		return obj;
	}

	template <class C>
	std::shared_ptr<C> create(raco::core::CommandInterface& cmd, std::string name, raco::core::SEditorObject parent = nullptr) {
		auto obj = std::dynamic_pointer_cast<C>(cmd.createObject(C::typeDescription.typeName, name));
		if (parent) {
			cmd.moveScenegraphChild(obj, parent);
		}
		return obj;
	}

	raco::user_types::SMesh create_mesh(const std::string &name, const std::string &relpath) {
		auto mesh = create<raco::user_types::Mesh>(name);
		commandInterface.set({mesh, {"uri"}}, (RacoBaseTest<BaseClass>::cwd_path() / relpath).string());
		return mesh;
	}

	raco::user_types::SMaterial create_material(const std::string &name, const std::string &relpathVertex, const std::string &relpathFragment) {
		auto material = create<raco::user_types::Material>(name);
		commandInterface.set({material, {"uriVertex"}}, (RacoBaseTest<BaseClass>::cwd_path() / relpathVertex).string());
		commandInterface.set({material, {"uriFragment"}}, (RacoBaseTest<BaseClass>::cwd_path() / relpathFragment).string());
		return material;
	}

	raco::user_types::SMeshNode create_meshnode(const std::string &name, raco::user_types::SMesh mesh, raco::user_types::SMaterial material, raco::user_types::SEditorObject parent = nullptr) {
		auto meshnode = create<raco::user_types::MeshNode>(name, parent);
		commandInterface.set({meshnode, {"mesh"}}, mesh);
		commandInterface.set({meshnode, {"materials", "material", "material"}}, material);
		return meshnode;
	}

	raco::user_types::SLuaScript create_lua(const std::string& name, const std::string& relpath) {
		auto lua = create<raco::user_types::LuaScript>(name);
		commandInterface.set({lua, {"uri"}}, (RacoBaseTest<BaseClass>::cwd_path() / relpath).string());
		return lua;
	}

	raco::user_types::SLuaScript create_lua(const std::string& name, const typename RacoBaseTest<BaseClass>::TextFile& file) {
		auto lua = create<raco::user_types::LuaScript>(name);
		commandInterface.set({lua, {"uri"}}, file);
		return lua;
	}

	void checkUndoRedo(std::function<void()> operation, std::function<void()> preCheck, std::function<void()> postCheck) {
		RacoBaseTest<BaseClass>::checkUndoRedo(commandInterface, operation, preCheck, postCheck);
	}

	template <std::size_t N>
	void checkUndoRedoMultiStep(std::array<std::function<void()>, N> operations, std::array<std::function<void()>, N + 1> checks) {
		RacoBaseTest<BaseClass>::checkUndoRedoMultiStep(commandInterface, operations, checks);
	}

	void checkLinks(Project& project, const std::vector<raco::core::Link>& refLinks) {
		EXPECT_EQ(refLinks.size(), project.links().size());
		for (const auto &refLink : refLinks) {
			auto projectLink = raco::core::Queries::getLink(project, refLink.endProp());
			EXPECT_TRUE(projectLink && projectLink->startProp() == refLink.startProp() && projectLink->isValid() == refLink.isValid());
		}
	}

	void checkLinks(const std::vector<raco::core::Link>& refLinks) {
		checkLinks(project, refLinks);
	}


	TestEnvironmentCoreT() : fileChangeMonitor(std::make_unique<FileChangeMonitorImpl>()), meshCache() {
		spdlog::drop_all();
		raco::log_system::init();
		clearQEventLoop();
		context.setFileChangeMonitor(fileChangeMonitor.get());
		context.setMeshCache(&meshCache);
	}
	virtual ~TestEnvironmentCoreT() {
		// Cleanup everything
		for (const auto& instance : context.project()->instances()) {
			instance->onBeforeDeleteObject(errors);
		}
		clearQEventLoop();
	}

	raco::ramses_base::HeadlessEngineBackend backend{};
	// FileChangeMonitor needs to be destroyed after the project (and with them all FileListeners have been cleaned up)
	std::unique_ptr<FileChangeMonitorImpl> fileChangeMonitor;
	raco::components::MeshCacheImpl meshCache;
	Project project{};
	raco::core::DataChangeRecorder recorder{};
	Errors errors{&recorder};
	BaseContext context{&project, backend.coreInterface(), objectFactory(), &recorder, &errors};
	raco::core::UndoStack undoStack{&context};
	raco::core::CommandInterface commandInterface{&context, &undoStack};
};

using TestEnvironmentCore = TestEnvironmentCoreT<>;
