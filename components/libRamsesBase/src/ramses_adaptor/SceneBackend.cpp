/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_adaptor/SceneBackend.h"

#include <ramses-client-api/Node.h>
#include <ramses-client-api/RenderPass.h>
#include <ramses-client-api/SceneGraphIterator.h>
#include <ramses-client-api/SceneObjectIterator.h>
#include <ramses-logic/LuaScript.h>
#include <ramses-logic/RamsesAppearanceBinding.h>
#include <ramses-logic/RamsesNodeBinding.h>
#include <ramses-logic/RamsesCameraBinding.h>
#include <ramses-logic/RamsesRenderPassBinding.h>
#include <ramses-logic/SkinBinding.h>

#include "ramses_base/RamsesFormatter.h"
#include "ramses_base/BaseEngineBackend.h"

#include "ramses_adaptor/LuaInterfaceAdaptor.h"
#include "ramses_adaptor/LuaScriptAdaptor.h"
#include "ramses_adaptor/MeshNodeAdaptor.h"

namespace raco::ramses_adaptor {

SceneBackend::SceneBackend(ramses_base::BaseEngineBackend& engine, const SDataChangeDispatcher& dispatcher) 
	: engine_(engine),
	dispatcher_{dispatcher} {}

ramses::RamsesClient* SceneBackend::client() const {
	return &engine_.client();
}

ramses_base::LogicEngine* SceneBackend::logicEngine() const {
	return &engine_.logicEngine();
}


void SceneBackend::setScene(Project* project, core::Errors *errors, bool optimizeForExport) {
	scene_.reset();
	scene_ = std::make_unique<SceneAdaptor>(client(), logicEngine(), toSceneId(*project->settings()->sceneId_), project, dispatcher_, errors, optimizeForExport);
}

ramses::sceneId_t SceneBackend::toSceneId(int i) {
	auto sceneId{ramses::sceneId_t{static_cast<uint32_t>(i)}};
	if (sceneId == ramses::sceneId_t::Invalid()) {
		sceneId = ramses::sceneId_t{1u};
	}
	return sceneId;
}

void SceneBackend::reset() {
	scene_.reset();
}

ramses::sceneId_t SceneBackend::currentSceneId() const {
	return scene_ ? scene_->sceneId() : ramses::sceneId_t::Invalid();
}

const ramses::Scene* SceneBackend::currentScene() const {
	return scene_->scene();
}

void SceneBackend::flush() {
	if (scene_) {
		scene_->scene()->flush();
	}
}

void SceneBackend::readDataFromEngine(core::DataChangeRecorder& recorder) {
	if (scene_) {
		scene_->readDataFromEngine(recorder);
	}
}

bool SceneBackend::discardLogicEngineMessage(std::string_view message) {
	if (message.find("has unlinked output") != std::string::npos) {
		return true;
	}
	if (message.find("has no ingoing links! Node should be deleted or properly linked!") != std::string::npos) {
		return true;
	}
	if (message.find("has no outgoing links! Node should be deleted or properly linked!") != std::string::npos) {
		return true;
	}
	return false;
}


std::vector<rlogic::WarningData> SceneBackend::logicEngineFilteredValidation() const {
	std::vector<rlogic::WarningData> filteredWarnings;
	std::vector<rlogic::WarningData> warnings = logicEngine()->validate();

	for (auto& warning : warnings) {
		if (!SceneBackend::discardLogicEngineMessage(warning.message)) {
			filteredWarnings.emplace_back(warning);
		}
	}
	return filteredWarnings;
}


std::string SceneBackend::ramsesFilteredValidationReport(core::ErrorLevel minLevel) const {
	auto report = currentScene()->getValidationReport(minLevel == core::ErrorLevel::ERROR ? ramses::EValidationSeverity_Error : ramses::EValidationSeverity_Warning);

	std::istringstream stream(report);
	std::string line;
	std::string filtered;
	while (std::getline(stream, line)) {
		if (line.find("rendergroup does not contain any meshes") != std::string::npos) {
			continue;
		}
		// The next two are a workaround for a ramses bug: remove eventually
		if (line.find("There is no valid content source set in TextureSampler") != std::string::npos) {
			continue;
		}
		if (line.find("RenderBuffer") != std::string::npos) {
			continue;
		}
		filtered.append(line).append("\n");
	}
	return filtered;
}

core::ErrorLevel SceneBackend::sceneValid() const {
	auto status = currentScene()->validate();
	if (status != ramses::StatusOK) {
		std::string msg = currentScene()->getStatusMessage(status);
		if (msg == "Validation error" && !ramsesFilteredValidationReport(core::ErrorLevel::ERROR).empty()) {
			return core::ErrorLevel::ERROR;
		} else if (msg == "Validation warning" && !ramsesFilteredValidationReport(core::ErrorLevel::WARNING).empty()) {
			return core::ErrorLevel::WARNING;
		}
	}
	if (!logicEngineFilteredValidation().empty()) {
		return core::ErrorLevel::ERROR;
	}
	return core::ErrorLevel::NONE;
}

std::string SceneBackend::getValidationReport(core::ErrorLevel minLevel) const {
	auto logicErrors = logicEngineFilteredValidation();
	std::string logicErrorMessages;
	for (const auto& logicError : logicErrors) {
		logicErrorMessages.append(logicError.message).append("\n");
	}

	return ramsesFilteredValidationReport(minLevel) + logicErrorMessages;
}

uint64_t SceneBackend::currentSceneIdValue() const {
	return currentSceneId().getValue();
}

namespace {
ramses::Node* getParent(ramses::RamsesObject* obj) {
	if (auto node{dynamic_cast<ramses::Node*>(obj)}) {
		return node->getParent();
	}
	return nullptr;
}
}  // namespace

std::vector<SceneBackend::SceneItemDesc> SceneBackend::getSceneItemDescriptions() const {
	ramses::SceneObjectIterator it{*scene_->scene()};
	std::vector<ramses::RamsesObject*> ramsesObjects{};
	while (auto object = it.getNext()) {
		if (object->isOfType(ramses::ERamsesObjectType_Node)) {
			if (!getParent(object)) {
				// If we hit a root parent we separatly iterate over it
				ramses::SceneGraphIterator graphIt{*ramses::RamsesUtils::TryConvert<ramses::Node>(*object), ramses::ETreeTraversalStyle_BreadthFirst};
				while (auto node = graphIt.getNext()) {
					ramsesObjects.push_back(node);
				}
			}
		} else {
			ramsesObjects.push_back(object);
		}
	}

	std::vector<SceneItemDesc> sceneItems;

	std::map<const ramses::RamsesObject*, int> parents{};
	for (const auto object : ramsesObjects) {
		ramses::RamsesObject* ramsesParent = getParent(object);
		int parentIndex = ramsesParent ? parents[ramsesParent] : -1;
		sceneItems.emplace_back(fmt::format("{}", object->getType()), object->getName(), parentIndex);
		parents[object] = static_cast<int>(sceneItems.size() - 1);
	}

	std::set<ramses::RamsesObject*> privateAppearances;
	scene_->iterateAdaptors([&privateAppearances](ObjectAdaptor* adaptor) {
		if (auto meshnode = dynamic_cast<MeshNodeAdaptor*>(adaptor)) {
			if (meshnode->editorObject()->materialPrivate(0)) {
				if (auto appearance = meshnode->privateAppearance().get()) {
					if (auto ramsesAppearance = appearance->get()) {
						privateAppearances.insert(ramsesAppearance);
					}
				}
			}
		}
	});

	for (const auto object : ramsesObjects) {
		auto meshnode = dynamic_cast<ramses::MeshNode*>(object);
		if (meshnode) {
			int meshnodeIdx = parents[meshnode];

			auto appearance = meshnode->getAppearance();
			if (privateAppearances.find(appearance) != privateAppearances.end()) {
				int appIdx = parents[appearance];
				sceneItems[appIdx].parentIndex_ = meshnodeIdx;
			}

			auto geometryBinding = meshnode->getGeometryBinding();
			auto geomIdx = parents[geometryBinding];
			sceneItems[geomIdx].parentIndex_ = meshnodeIdx;
		}
	}

	for (const auto* module : logicEngine()->getCollection<rlogic::LuaModule>()) {
		sceneItems.emplace_back("LuaScriptModule", module->getName().data(), -1);
	}

	for (const auto* script : logicEngine()->getCollection<rlogic::LuaScript>()) {
		sceneItems.emplace_back("LuaScript", script->getName().data(), -1);
	}

	for (const auto* interface: logicEngine()->getCollection<rlogic::LuaInterface>()) {
		sceneItems.emplace_back("LuaInterface", interface->getName().data(), -1);
	}

	for (const auto* dataArray : logicEngine()->getCollection<rlogic::DataArray>()) {
		sceneItems.emplace_back("DataArray", dataArray->getName().data(), -1);
	}

	for (const auto* animation : logicEngine()->getCollection<rlogic::AnimationNode>()) {
		sceneItems.emplace_back("Animation", animation->getName().data(), -1);
	}

	for (const auto* binding : logicEngine()->getCollection<rlogic::RamsesAppearanceBinding>()) {
		auto parentIdx = parents[&binding->getRamsesAppearance()];
		sceneItems.emplace_back("AppearanceBinding", binding->getName().data(), parentIdx);
	}

	for (const auto* binding : logicEngine()->getCollection<rlogic::RamsesNodeBinding>()) {
		auto parentIdx = parents[&binding->getRamsesNode()];
		sceneItems.emplace_back("NodeBinding", binding->getName().data(), parentIdx);
	}

	for (const auto* binding : logicEngine()->getCollection<rlogic::RamsesMeshNodeBinding>()) {
		auto parentIdx = parents[&binding->getRamsesMeshNode()];
		sceneItems.emplace_back("MeshNodeBinding", binding->getName().data(), parentIdx);
	}

	for (const auto* binding : logicEngine()->getCollection<rlogic::RamsesCameraBinding>()) {
		auto parentIdx = parents[&binding->getRamsesCamera()];
		sceneItems.emplace_back("CameraBinding", binding->getName().data(), parentIdx);
	}

	for (const auto* binding : logicEngine()->getCollection<rlogic::RamsesRenderPassBinding>()) {
		auto parentIdx = parents[&binding->getRamsesRenderPass()];
		sceneItems.emplace_back("RenderPassBinding", binding->getName().data(), parentIdx);
	}

	for (const auto* timer : logicEngine()->getCollection<rlogic::TimerNode>()) {
		sceneItems.emplace_back("Timer", timer->getName().data(), -1);
	}

	for (const auto* anchorPoint : logicEngine()->getCollection<rlogic::AnchorPoint>()) {
		sceneItems.emplace_back("AnchorPoint", anchorPoint->getName().data(), -1);
	}

	for (const auto* binding : logicEngine()->getCollection<rlogic::RamsesRenderGroupBinding>()) {
		auto parentIdx = parents[&binding->getRamsesRenderGroup()];
		sceneItems.emplace_back("RenderGroupBinding", binding->getName().data(), parentIdx);
	}

	for (const auto* skin : logicEngine()->getCollection<rlogic::SkinBinding>()) {
		sceneItems.emplace_back("SkinBinding", skin->getName().data(), -1);
	}

	return sceneItems;
}

std::string SceneBackend::getExportedObjectNames(SEditorObject editorObject) const {
	auto* adaptor = sceneAdaptor()->lookupAdaptor(editorObject);
	if (adaptor == nullptr) {
		return "";
	}

	auto exportInfo = adaptor->getExportInformation();
	if (exportInfo.empty()){
		return "Will not be exported.";
	}

	std::string result = "Will be exported as:";
	for (const auto& item : exportInfo) {
		result += fmt::format("\n{} [{}]", item.name, item.type);
	}

	return result;
}

}  // namespace raco::ramses_adaptor
