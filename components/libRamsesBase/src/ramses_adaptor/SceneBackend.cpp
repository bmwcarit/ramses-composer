/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_adaptor/SceneBackend.h"

#include <ramses-client-api/Node.h>
#include <ramses-client-api/SceneGraphIterator.h>
#include <ramses-client-api/SceneObjectIterator.h>
#include <ramses-logic/RamsesAppearanceBinding.h>
#include <ramses-logic/RamsesNodeBinding.h>
#include <ramses-logic/RamsesCameraBinding.h>

#include "ramses_base/RamsesFormatter.h"
#include "ramses_base/BaseEngineBackend.h"

namespace raco::ramses_adaptor {

SceneBackend::SceneBackend(ramses_base::BaseEngineBackend *engine, const SDataChangeDispatcher& dispatcher) 
	: client_{&engine->client()},
	logicEngine_{&engine->logicEngine()},
	dispatcher_{dispatcher} {}

void SceneBackend::setScene(Project* project, core::Errors *errors) {
	scene_.reset();
	scene_ = std::make_unique<SceneAdaptor>(client_, logicEngine_, toSceneId(*project->settings()->sceneId_), project, dispatcher_, errors);
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


bool SceneBackend::sceneValid() const {
	return currentScene()->validate() == ramses::StatusOK;
}

std::string SceneBackend::getValidationReport(core::ErrorLevel minLevel) const {
	return currentScene()->getValidationReport(minLevel == core::ErrorLevel::ERROR ? ramses::EValidationSeverity_Error : ramses::EValidationSeverity_Warning);
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

	std::map<ramses::RamsesObject*, int> parents{};
	for (const auto object : ramsesObjects) {
		ramses::RamsesObject* ramsesParent = getParent(object);
		int parentIndex = ramsesParent ? parents[ramsesParent] : -1;
		sceneItems.emplace_back(fmt::format("{}", object->getType()), object->getName(), parentIndex);
		parents[object] = static_cast<int>(sceneItems.size() - 1);
	}

	for (const auto object : ramsesObjects) {
		auto meshnode = dynamic_cast<ramses::MeshNode*>(object);
		if (meshnode) {
			int meshnodeIdx = parents[meshnode];

			auto appearance = meshnode->getAppearance();	
			int appIdx = parents[appearance];
			sceneItems[appIdx].parentIndex_ = meshnodeIdx;

			auto geometryBinding = meshnode->getGeometryBinding();
			auto geomIdx = parents[geometryBinding];
			sceneItems[geomIdx].parentIndex_ = meshnodeIdx;
		}
	}

	for (const auto* script : logicEngine_->scripts()) {
		sceneItems.emplace_back("LuaScript", script->getName().data(), -1);
	}

	for (const auto* binding : logicEngine_->ramsesAppearanceBindings()) {
		auto parentIdx = parents[&binding->getRamsesAppearance()];
		sceneItems.emplace_back("AppearanceBinding", binding->getName().data(), parentIdx);
	}

	for (const auto* binding : logicEngine_->ramsesNodeBindings()) {
		auto parentIdx = parents[&binding->getRamsesNode()];
		sceneItems.emplace_back("NodeBinding", binding->getName().data(), parentIdx);
	}

	for (const auto* binding : logicEngine_->ramsesCameraBindings()) {
		auto parentIdx = parents[&binding->getRamsesCamera()];
		sceneItems.emplace_back("CameraBinding", binding->getName().data(), parentIdx);
	}

	return sceneItems;
}


}  // namespace raco::ramses_adaptor
