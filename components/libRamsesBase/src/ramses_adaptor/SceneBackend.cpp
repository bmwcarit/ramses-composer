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

#include <ramses/client/Node.h>
#include <ramses/client/RenderPass.h>
#include <ramses/client/SceneGraphIterator.h>
#include <ramses/client/SceneObjectIterator.h>
#include <ramses/client/logic/LuaScript.h>
#include <ramses/client/logic/AppearanceBinding.h>
#include <ramses/client/logic/NodeBinding.h>
#include <ramses/client/logic/CameraBinding.h>
#include <ramses/client/logic/RenderPassBinding.h>
#include <ramses/client/logic/SkinBinding.h>

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

ramses::LogicEngine* SceneBackend::logicEngine() const {
	return &scene_->logicEngine();
}


void SceneBackend::setScene(Project* project, core::Errors* errors, bool optimizeForExport, ramses::sceneId_t sceneId) {
	scene_.reset();
	scene_ = std::make_unique<SceneAdaptor>(client(), sceneId, project, dispatcher_, errors, optimizeForExport);
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

ramses::Scene* SceneBackend::currentScene() const {
	return scene_->scene();
}

std::optional<ramses::Issue> SceneBackend::getLastError() {
	return scene_->scene()->getRamsesClient().getRamsesFramework().getLastError();
}

void SceneBackend::flush() {
	if (scene_) {
		if (!scene_->scene()->flush()) {
			LOG_ERROR(log_system::RAMSES_BACKEND, "Scene flush failed with '{}'", getLastError().value().message);
		}
	}
}

void SceneBackend::readDataFromEngine(core::DataChangeRecorder& recorder) {
	if (scene_) {
		scene_->readDataFromEngine(recorder);
	}
}

bool SceneBackend::discardRamsesMessage(std::string_view message) {
	// The ramses Renderer will produce this error message if a RenderBuffer changes its size after it has been allocated.
	// If the size changes via a normal property change of the RenderBuffer the engine backend would recreate the objects and all is fine.
	// If the change happens due to a link update only this error is generated.  But in this case SceneAdaptor::readDataFromEngine 
	// will tag the adaptor as dirty and the buffer will be recreated in Ramses in the next frame so we are OK too.
	// So we can safely ignore this error.
	if (message.find("RendererResourceManager::updateRenderTargetBufferProperties changing properties of RenderBuffer which is already uploaded is not supported!") != std::string_view::npos) {
		return true;
	}
	if (message.find("has unlinked output") != std::string::npos) {
		return true;
	}
	if (message.find("has no incoming links! Node should be deleted or properly linked!") != std::string::npos) {
		return true;
	}
	if (message.find("has no outgoing links! Node should be deleted or properly linked!") != std::string::npos) {
		return true;
	}
	if (message.find("Unused [uniform]") != std::string::npos ||
		message.find("Unused [in]") != std::string::npos ||
		message.find("Unused [out]") != std::string::npos) {
		return true;
	}
	return false;
}

std::string SceneBackend::ramsesFilteredValidationReport(core::ErrorLevel minLevel) const {
	ramses::ValidationReport report;
	currentScene()->validate(report);

	std::string filtered;

	for (const auto& issue : report.getIssues()) {
		if (issue.type == ramses::EIssueType::Warning && minLevel == core::ErrorLevel::ERROR) {
			continue;
		}

		if (discardRamsesMessage(issue.message)) {
			continue;
		}

		filtered.append(fmt::format("{}: {} '{}': {}\n",
			issue.type == ramses::EIssueType::Warning ? "WARNING" : "ERROR",
			issue.object->getType(), issue.object->getName(), issue.message));
	}
	return filtered;
}

core::ErrorLevel SceneBackend::sceneValid() const {
	ramses::ValidationReport report;
	currentScene()->validate(report);
	if (report.hasError() && !ramsesFilteredValidationReport(core::ErrorLevel::ERROR).empty()) {
		return core::ErrorLevel::ERROR;
	} else if (report.hasIssue() && !ramsesFilteredValidationReport(core::ErrorLevel::WARNING).empty()) {
		return core::ErrorLevel::WARNING;
	}
	return core::ErrorLevel::NONE;
}

std::string SceneBackend::getValidationReport(core::ErrorLevel minLevel) const {
	return ramsesFilteredValidationReport(minLevel);
}

uint64_t SceneBackend::currentSceneIdValue() const {
	return currentSceneId().getValue();
}

namespace {
	const ramses::Node* getParent(ramses::RamsesObject* obj) {
		if (auto node{ dynamic_cast<ramses::Node*>(obj) }) {
			return node->getParent();
		}
		return nullptr;
	}
}  // namespace

std::vector<SceneBackend::SceneItemDesc> SceneBackend::getSceneItemDescriptions() const {
	ramses::SceneObjectIterator it{ *scene_->scene() };
	std::vector<ramses::RamsesObject*> ramsesObjects{};
	while (auto object = it.getNext()) {
		if (object->isOfType(ramses::ERamsesObjectType::Node)) {
			if (!getParent(object)) {
				// If we hit a root parent we separatly iterate over it
				ramses::SceneGraphIterator graphIt{ *object->as<ramses::Node>(), ramses::ETreeTraversalStyle::BreadthFirst };
				while (auto node = graphIt.getNext()) {
					ramsesObjects.push_back(node);
				}
			}
		} else if (!object->isOfType(ramses::ERamsesObjectType::LogicEngine)) {
			ramsesObjects.push_back(object);
		}
	}

	std::vector<SceneItemDesc> sceneItems;

	std::map<const ramses::RamsesObject*, int> parents{};
	for (const auto object : ramsesObjects) {
		const ramses::RamsesObject* ramsesParent = getParent(object);
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

			auto geometry = meshnode->getGeometry();
			auto geomIdx = parents[geometry];
			sceneItems[geomIdx].parentIndex_ = meshnodeIdx;
		}
	}

	for (const auto* module : logicEngine()->getCollection<ramses::LuaModule>()) {
		sceneItems.emplace_back("LuaScriptModule", module->getName(), -1);
	}

	for (const auto* script : logicEngine()->getCollection<ramses::LuaScript>()) {
		sceneItems.emplace_back("LuaScript", script->getName(), -1);
	}

	for (const auto* interface: logicEngine()->getCollection<ramses::LuaInterface>()) {
		sceneItems.emplace_back("LuaInterface", interface->getName(), -1);
	}

	for (const auto* dataArray : logicEngine()->getCollection<ramses::DataArray>()) {
		sceneItems.emplace_back("DataArray", dataArray->getName(), -1);
	}

	for (const auto* animation : logicEngine()->getCollection<ramses::AnimationNode>()) {
		sceneItems.emplace_back("Animation", animation->getName(), -1);
	}

	for (const auto* binding : logicEngine()->getCollection<ramses::AppearanceBinding>()) {
		auto parentIdx = parents[&binding->getRamsesAppearance()];
		sceneItems.emplace_back("AppearanceBinding", binding->getName(), parentIdx);
	}

	for (const auto* binding : logicEngine()->getCollection<ramses::NodeBinding>()) {
		auto parentIdx = parents[&binding->getRamsesNode()];
		sceneItems.emplace_back("NodeBinding", binding->getName(), parentIdx);
	}

	for (const auto* binding : logicEngine()->getCollection<ramses::MeshNodeBinding>()) {
		auto parentIdx = parents[&binding->getRamsesMeshNode()];
		sceneItems.emplace_back("MeshNodeBinding", binding->getName(), parentIdx);
	}

	for (const auto* binding : logicEngine()->getCollection<ramses::CameraBinding>()) {
		auto parentIdx = parents[&binding->getRamsesCamera()];
		sceneItems.emplace_back("CameraBinding", binding->getName(), parentIdx);
	}

	for (const auto* binding : logicEngine()->getCollection<ramses::RenderPassBinding>()) {
		auto parentIdx = parents[&binding->getRamsesRenderPass()];
		sceneItems.emplace_back("RenderPassBinding", binding->getName(), parentIdx);
	}

	for (const auto* timer : logicEngine()->getCollection<ramses::TimerNode>()) {
		sceneItems.emplace_back("Timer", timer->getName(), -1);
	}

	for (const auto* anchorPoint : logicEngine()->getCollection<ramses::AnchorPoint>()) {
		sceneItems.emplace_back("AnchorPoint", anchorPoint->getName(), -1);
	}

	for (const auto* binding : logicEngine()->getCollection<ramses::RenderGroupBinding>()) {
		auto parentIdx = parents[&binding->getRamsesRenderGroup()];
		sceneItems.emplace_back("RenderGroupBinding", binding->getName(), parentIdx);
	}

	for (const auto* skin : logicEngine()->getCollection<ramses::SkinBinding>()) {
		sceneItems.emplace_back("SkinBinding", skin->getName(), -1);
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

std::map<std::string, std::chrono::microseconds> SceneBackend::getPerformanceReport() {
	auto report{logicEngine()->getLastUpdateReport()};

	std::map<std::string, std::chrono::microseconds> timings;
	for (auto& [logicNode, time_us] : report.getNodesExecuted()) {
		auto objectID{core::EditorObject::ramsesLogicIDAsObjectID(logicNode->getUserId())};
		timings[objectID] += time_us;
	}
	return timings;
}

}  // namespace raco::ramses_adaptor
