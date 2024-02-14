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
#include "gtest/gtest.h"

#include "application/RaCoApplication.h"
#include "ramses_base/HeadlessEngineBackend.h"
#include "testing/RacoBaseTest.h"

#pragma once

class RaCoApplicationTest : public RacoBaseTest<> {
public:
	ramses_base::HeadlessEngineBackend backend{};
	raco::application::RaCoApplication application{backend, {{}, false, false, -1, -1, false}};

	core::ExternalProjectsStoreInterface* externalProjectStore() {
		return application.externalProjects();
	};

	core::CommandInterface& commandInterface() {
		return *application.activeRaCoProject().commandInterface();
	};

	core::DataChangeRecorder& recorder() {
		return *application.activeRaCoProject().recorder();
	};

	core::Project& project() {
		return *application.activeRaCoProject().project();
	};

	template <class C>
	std::shared_ptr<C> create(std::string name, core::SEditorObject parent = nullptr) {
		auto obj = std::dynamic_pointer_cast<C>(commandInterface().createObject(C::typeDescription.typeName, name));
		if (parent) {
			commandInterface().moveScenegraphChildren({obj}, parent);
		}
		return obj;
	}
};
