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

#include "ramses_adaptor/Factories.h"
#include "ramses_adaptor/SceneAdaptor.h"
#include "ramses_base/RamsesHandles.h"
#include "components/DataChangeDispatcher.h"
#include <core/Context.h>
#include <memory>
#include "core/SceneBackendInterface.h"

namespace ramses {
class RamsesClient;
}

namespace raco::ramses_base {
class BaseEngineBackend;
}

namespace raco::ramses_adaptor {

class SceneBackend : public core::SceneBackendInterface {
public:
	static ramses::sceneId_t toSceneId(int i);

	using Project = core::Project;
	using SEditorObject = core::SEditorObject;
	using SDataChangeDispatcher = components::SDataChangeDispatcher;

	explicit SceneBackend(ramses_base::BaseEngineBackend& engine, const SDataChangeDispatcher& dispatcher);
	void setScene(Project* project, core::Errors* errors, bool optimizeForExport, ramses::sceneId_t sceneId);
	void reset();
	void flush();
	void readDataFromEngine(core::DataChangeRecorder &recorder);
	ramses::sceneId_t currentSceneId() const;
	ramses::Scene* currentScene() const;

	std::optional<ramses::Issue> getLastError();

	SceneAdaptor* sceneAdaptor() const {
		return scene_.get();
	}
	
	core::ErrorLevel sceneValid() const override;
	std::string getValidationReport(core::ErrorLevel minLevel) const override;
	uint64_t currentSceneIdValue() const override;
	std::vector<SceneItemDesc> getSceneItemDescriptions() const override;
	std::string getExportedObjectNames(SEditorObject editorObject) const override;
	
	ramses::LogicEngine* logicEngine() const;

	static bool discardRamsesMessage(std::string_view message);

private:
	std::string ramsesFilteredValidationReport(core::ErrorLevel minLevel) const;

	ramses::RamsesClient* client() const;

	SDataChangeDispatcher dispatcher_;
	ramses_base::BaseEngineBackend& engine_;
	std::unique_ptr<SceneAdaptor> scene_;
};

}  // namespace raco::ramses_adaptor
