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
#include "core/MeshCacheInterface.h"
#include "core/Project.h"
#include "core/Undo.h"
#include "core/UserObjectFactoryInterface.h"
#include "log_system/log.h"
#include "ramses_base/HeadlessEngineBackend.h"
#include "components/FileChangeMonitorImpl.h"
#include "components/MeshCacheImpl.h"
#include "user_types/UserObjectFactory.h"
#include "utils/FileUtils.h"

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

	void checkUndoRedo(std::function<void()> operation, std::function<void()> preCheck, std::function<void()> postCheck) {
		RacoBaseTest<BaseClass>::checkUndoRedo(commandInterface, operation, preCheck, postCheck);
	}

	template <std::size_t N>
	void checkUndoRedoMultiStep(std::array<std::function<void()>, N> operations, std::array<std::function<void()>, N + 1> checks) {
		RacoBaseTest<BaseClass>::checkUndoRedoMultiStep(commandInterface, operations, checks);
	}

	TestEnvironmentCoreT() {
		spdlog::drop_all();
		raco::log_system::init();
		clearQEventLoop();
		fileChangeMonitor = std::make_unique<FileChangeMonitorImpl>(context);
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
	raco::components::MeshCacheImpl meshCache{context};
	Project project{};
	raco::core::DataChangeRecorder recorder{};
	Errors errors{&recorder};
	BaseContext context{&project, backend.coreInterface(), objectFactory(), &recorder, &errors};
	raco::core::UndoStack undoStack{&context};
	raco::core::CommandInterface commandInterface{&context, &undoStack};
};

using TestEnvironmentCore = TestEnvironmentCoreT<>;
