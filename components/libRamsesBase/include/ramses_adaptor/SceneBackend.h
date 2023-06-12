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
#include "ramses_base/LogicEngine.h"
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

class SceneBackend : public raco::core::SceneBackendInterface {
public:
	static ramses::sceneId_t toSceneId(int i);

	using Project = raco::core::Project;
	using SEditorObject = raco::core::SEditorObject;
	using SDataChangeDispatcher = raco::components::SDataChangeDispatcher;

	explicit SceneBackend(ramses_base::BaseEngineBackend& engine, const SDataChangeDispatcher& dispatcher);
	void setScene(Project* project, core::Errors* errors, bool optimizeForExport);
	void reset();
	void flush();
	void readDataFromEngine(core::DataChangeRecorder &recorder);
	ramses::sceneId_t currentSceneId() const;
	const ramses::Scene* currentScene() const;

	SceneAdaptor* sceneAdaptor() const {
		return scene_.get();
	}
	
	core::ErrorLevel sceneValid() const override;
	std::string getValidationReport(core::ErrorLevel minLevel) const override;
	uint64_t currentSceneIdValue() const override;
	std::vector<SceneItemDesc> getSceneItemDescriptions() const override;
	std::string getExportedObjectNames(SEditorObject editorObject) const override;

	static bool discardLogicEngineMessage(std::string_view message);

private:
	/**
	 * @brief call LogicEngine validate() and filter out warnings that RamsesComposer is 
	 * deliberately ignoring.
	*/
	std::vector<rlogic::WarningData> logicEngineFilteredValidation() const;
	std::string ramsesFilteredValidationReport(core::ErrorLevel minLevel) const;

	ramses::RamsesClient* client() const;
	ramses_base::LogicEngine* logicEngine() const;

	SDataChangeDispatcher dispatcher_;
	ramses_base::BaseEngineBackend& engine_;
	std::unique_ptr<SceneAdaptor> scene_;
};

}  // namespace raco::ramses_adaptor
