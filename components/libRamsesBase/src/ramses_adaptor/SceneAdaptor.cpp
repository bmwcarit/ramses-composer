/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_adaptor/SceneAdaptor.h"

#include "components/DataChangeDispatcher.h"
#include "components/EditorObjectFormatter.h"
#include "core/Iterators.h"
#include "core/PrefabOperations.h"
#include "core/Project.h"
#include "core/Queries.h"
#include "ramses_adaptor/AnchorPointAdaptor.h"
#include "ramses_adaptor/AnimationAdaptor.h"
#include "ramses_adaptor/DefaultRamsesObjects.h"
#include "ramses_adaptor/Factories.h"
#include "ramses_adaptor/LuaScriptAdaptor.h"
#include "ramses_adaptor/ObjectAdaptor.h"
#include "ramses_adaptor/OrthographicCameraAdaptor.h"
#include "ramses_adaptor/PerspectiveCameraAdaptor.h"
#include "ramses_adaptor/TimerAdaptor.h"
#include "ramses_base/RamsesHandles.h"
#include "user_types/Animation.h"
#include "user_types/BlitPass.h"
#include "user_types/MeshNode.h"
#include "user_types/Prefab.h"
#include "core/ProjectSettings.h"
#include "user_types/RenderPass.h"

#include <spdlog/fmt/fmt.h>

#include <algorithm>
#include <map>
#include <memory>
#include <unordered_set>

namespace raco::ramses_adaptor {

using namespace raco::ramses_base;

SceneAdaptor::SceneAdaptor(ramses::RamsesClient* client, ramses::sceneId_t id, Project* project, components::SDataChangeDispatcher dispatcher, core::Errors* errors, bool optimizeForExport)
	: client_{client},
	  project_(project),
	  scene_{ramsesScene(id, client_)},
	  logicEngine_{ramses_base::BaseEngineBackend::UniqueLogicEngine(scene_->createLogicEngine(), [this](ramses::LogicEngine* logicEngine) { scene_->destroy(*logicEngine); })},
	  subscription_{dispatcher->registerOnObjectsLifeCycle([this](SEditorObject obj) { createAdaptor(obj); }, [this](SEditorObject obj) { removeAdaptor(obj); })},
	  childrenSubscription_(dispatcher->registerOnPropertyChange("children", [this](core::ValueHandle handle) {
		  adaptorStatusDirty_ = true;
	  })),
	  linksLifecycle_{dispatcher->registerOnLinksLifeCycle(
		  [this](const core::LinkDescriptor& link) { createLink(link); }, 
		  [this](const core::LinkDescriptor& link) { removeLink(link); })},
	  linkValidityChangeSub_{dispatcher->registerOnLinkValidityChange(
		  [this](const core::LinkDescriptor& link) { changeLinkValidity(link, link.isValid); })},
	  dispatcher_{dispatcher},
	  errors_{errors},
	  optimizeForExport_(optimizeForExport) {

	for (const SEditorObject& obj : project_->instances()) {
		createAdaptor(obj);
	}

	dispatcher_->addBulkChangeCallback(id.getValue(), [this](const core::SEditorObjectSet& changedObjects) {
		performBulkEngineUpdate(changedObjects);
	});

	for (const SLink& link : project_->links()) {
		createLink(link->descriptor());
	}

	const auto& instances{project_->instances()};
	core::SEditorObjectSet initialBulkUpdate(instances.begin(), instances.end());
	performBulkEngineUpdate(initialBulkUpdate);

	scene_->flush();
	scene_->publish(ramses::EScenePublicationMode::LocalAndRemote);
}

SceneAdaptor::~SceneAdaptor() {
	dispatcher_->removeBulkChangeCallback(sceneId().getValue());
}

ramses::Scene* SceneAdaptor::scene() {
	return scene_.get();
}

ramses::sceneId_t SceneAdaptor::sceneId() {
	return scene_->getSceneId();
}

bool SceneAdaptor::needAdaptor(SEditorObject object) {
	return !core::PrefabOperations::findContainingPrefab(object) && !object->isType<core::ProjectSettings>();
}


void SceneAdaptor::createAdaptor(SEditorObject obj) {
	if (needAdaptor(obj)) {
		auto adaptor = Factories::createAdaptor(this, obj);
		if (adaptor) {
			adaptor->tagDirty();
			adaptors_[obj] = std::move(adaptor);
		}
	}
}

void SceneAdaptor::removeAdaptor(SEditorObject obj) {
	auto adaptorWasLogicProvider = dynamic_cast<ILogicPropertyProvider*>(lookupAdaptor(obj)) != nullptr;
	adaptors_.erase(obj);
	deleteUnusedDefaultResources();
	if (adaptorWasLogicProvider && lastErrorObject_ == obj) {
		clearRuntimeError();
	}
	dependencyGraph_.clear();
}

void SceneAdaptor::iterateAdaptors(std::function<void(ObjectAdaptor*)> func) {
	for (const auto& [obj, adaptor] : adaptors_) {
		func(adaptor.get());
	}
}

bool SceneAdaptor::optimizeForExport() const {
	return optimizeForExport_;
}

ramses::EFeatureLevel SceneAdaptor::featureLevel() const {
	return client_->getRamsesFramework().getFeatureLevel();
}

void SceneAdaptor::updateRuntimeError(const ramses::Issue& issue) {
	std::unordered_set<ILogicPropertyProvider*> logicProvidersWithoutRuntimeError;
	std::string runtimeErrorObjectName;
	lastErrorObject_ = nullptr;

	for (const auto& instance : project().instances()) {
		if (auto logicProvider = dynamic_cast<ILogicPropertyProvider*>(lookupAdaptor(instance))) {
			std::vector<ramses::LogicNode*> logicNodes;
			logicProvider->getLogicNodes(logicNodes);

			if (std::find(logicNodes.begin(), logicNodes.end(), issue.object) != logicNodes.end()) {
				runtimeErrorObjectName = instance->objectName();

				// keep the old runtime error message if it is identical to the new message to prevent unnecessary error regeneration in the UI
				if (errors_->hasError(instance)) {
					auto instError = errors_->getError(instance);
					if (instError.category() == core::ErrorCategory::RAMSES_LOGIC_RUNTIME && instError.message() != issue.message) {
						errors_->removeError(instance);
					}
				}
				lastErrorObject_ = instance;
				logicProvider->onRuntimeError(*errors_, issue.message, core::ErrorLevel::ERROR);
			} else {
				logicProvidersWithoutRuntimeError.emplace(logicProvider);
			}
		}
	}

	// keep the old runtime error info message if it is identical to the new message to prevent unnecessary error regeneration in the UI
	auto ramsesLogicErrorFoundMsg = fmt::format("Ramses logic engine detected a runtime error in '{}'.\nBe aware that some Lua script outputs and/or linked properties might not have been updated.", runtimeErrorObjectName);
	errors_->removeIf([this, &ramsesLogicErrorFoundMsg, &logicProvidersWithoutRuntimeError](const core::ErrorItem& errorItem) {
		if (auto logicProvider = dynamic_cast<ILogicPropertyProvider*>(lookupAdaptor(errorItem.valueHandle().rootObject()))) {
			return logicProvidersWithoutRuntimeError.count(logicProvider) == 1 && errorItem.category() == core::ErrorCategory::RAMSES_LOGIC_RUNTIME && errorItem.message() != ramsesLogicErrorFoundMsg;
		}
		return false;
	});

	for (const auto& logicProvider : logicProvidersWithoutRuntimeError) {
		logicProvider->onRuntimeError(*errors_, ramsesLogicErrorFoundMsg, core::ErrorLevel::INFORMATION);
	}
}

void SceneAdaptor::clearRuntimeError() {
	lastErrorObject_ = nullptr;
	errors_->removeIf([](const core::ErrorItem& errorItem) {
		return errorItem.category() == core::ErrorCategory::RAMSES_LOGIC_RUNTIME;
	});
}

void SceneAdaptor::deleteUnusedDefaultResources() {
	// Resource use count is 1 => it is stored in the scene but not used by any MeshNodeAdaptor
	// - delete the resource as to not get unnecessarily exported.
	if (defaultAppearance_.use_count() == 1) {
		defaultAppearance_.reset();
	}
	if (defaultAppearanceWithNormals_.use_count() == 1) {
		defaultAppearanceWithNormals_.reset();
	}
	if (defaultIndices_[0].use_count() == 1) {
		defaultIndices_[0].reset();
	}
	if (defaultIndices_[1].use_count() == 1) {
		defaultIndices_[1].reset();
	}
	if (defaultVertices_[0].use_count() == 1) {
		defaultVertices_[0].reset();
	}
	if (defaultVertices_[1].use_count() == 1) {
		defaultVertices_[1].reset();
	}
	if (defaultNormals_[0].use_count() == 1) {
		defaultNormals_[0].reset();
	}
	if (defaultNormals_[1].use_count() == 1) {
		defaultNormals_[1].reset();
	}
	if (defaultTextureSampler_.use_count() == 1) {
		defaultTextureSampler_.reset();
	}
	if (defaultTextureCubeSampler_.use_count() == 1) {
		defaultTextureCubeSampler_.reset();
	}
}

void SceneAdaptor::readDataFromEngine(core::DataChangeRecorder& recorder) {
	for (const auto& [endObjecttID, linkMap] : links_.linksByEnd_) {
		for (const auto& [link, adaptor] : linkMap) {
			bool changed = adaptor->readDataFromEngine(recorder);
			if (changed && (link.end.object()->isType<user_types::RenderBuffer>() || link.end.object()->isType<user_types::RenderBufferMS>())) {
				if (auto endObjAdaptor = lookupAdaptor(link.end.object())) {
					// This is needed because Ramses can't change the size of a RenderBuffer after it is created.
					// To work around this we detect a buffer size change via a link and set the adaptor to dirty to
					// force a recreation of the buffer in ramses in the next frame:
					endObjAdaptor->tagDirty(true);
				}
			}
		}
	}
	for (const auto& [editorObject, adaptor] : adaptors_) {
		if (editorObject->isType<user_types::LuaScript>()) {
			static_cast<LuaScriptAdaptor*>(adaptor.get())->readDataFromEngine(recorder);
		} else if (editorObject->isType<user_types::Animation>()) {
			static_cast<AnimationAdaptor*>(adaptor.get())->readDataFromEngine(recorder);
		} else if (editorObject->isType<user_types::Timer>()) {
			static_cast<TimerAdaptor*>(adaptor.get())->readDataFromEngine(recorder);
		} else if (editorObject->isType<user_types::AnchorPoint>()) {
			static_cast<AnchorPointAdaptor*>(adaptor.get())->readDataFromEngine(recorder);
		}
	}
}

void SceneAdaptor::createLink(const core::LinkDescriptor& link) {
	newLinks_.insert(link);	
}

void SceneAdaptor::changeLinkValidity(const core::LinkDescriptor& link, bool isValid) {
	auto& map = links_.linksByEnd_.at(link.end.object()->objectID());
	map[link]->editorLink().isValid = isValid;
}

void SceneAdaptor::removeLink(const core::LinkDescriptor& link) {
	auto startObjId = link.start.object()->objectID();
	links_.linksByStart_[startObjId].erase(link);
	if (links_.linksByStart_[startObjId].empty()) {
		links_.linksByStart_.erase(startObjId);
	}

	auto endObjId = link.end.object()->objectID();
	links_.linksByEnd_[endObjId].erase(link);
	if (links_.linksByEnd_[endObjId].empty()) {
		links_.linksByEnd_.erase(endObjId);
	}
}

ramses::RamsesClient* SceneAdaptor::client() {
	return client_;
}

ramses::LogicEngine& SceneAdaptor::logicEngine() {
	return *logicEngine_;
}

const ramses::RamsesClient* SceneAdaptor::client() const {
	return client_;
}

const SRamsesAdaptorDispatcher SceneAdaptor::dispatcher() const {
	return dispatcher_;
}

const RamsesAppearance SceneAdaptor::defaultAppearance(bool withMeshNormals) {
	ramses_base::RamsesAppearance& appearance = withMeshNormals ? defaultAppearanceWithNormals_ : defaultAppearance_;
	if (!appearance) {
		appearance = createDefaultAppearance(scene_.get(), withMeshNormals, false, false);
	}
	return appearance;
}

const RamsesArrayResource SceneAdaptor::defaultVertices(int index) {
	if (!defaultVertices_[index]) {
		defaultVertices_[index] = index == 1 ? createCatVertexDataBuffer(scene_.get()) : createCubeVertexDataBuffer(scene_.get());
	}
	return defaultVertices_[index];
}

const RamsesArrayResource SceneAdaptor::defaultNormals(int index) {
	if (!defaultNormals_[index]) {
		defaultNormals_[index] = index == 1 ? createCatNormalDataBuffer(scene_.get()) : createCubeNormalDataBuffer(scene_.get());
	}
	return defaultNormals_[index];
}

const RamsesArrayResource SceneAdaptor::defaultIndices(int index) {
	if (!defaultIndices_[index]) {
		defaultIndices_[index] = index == 1 ? createCatIndexDataBuffer(scene_.get()) : createCubeIndexDataBuffer(scene_.get());
	}
	return defaultIndices_[index];
}

const ramses_base::RamsesTextureSampler SceneAdaptor::defaultTextureSampler() {
	if (!defaultTextureSampler_) {
		defaultTextureSampler_ = createDefaultTextureSampler(scene_.get());
	}
	return defaultTextureSampler_;
}

const ramses_base::RamsesTextureSampler SceneAdaptor::defaultTextureCubeSampler() {
	if (!defaultTextureCubeSampler_) {
		defaultTextureCubeSampler_ = createDefaultTextureCubeSampler(scene_.get());
	}
	return defaultTextureCubeSampler_;
}

const std::vector<DependencyNode>& SceneAdaptor::dependencyGraph() {
	return dependencyGraph_;
}

ObjectAdaptor* SceneAdaptor::lookupAdaptor(const core::SEditorObject& editorObject) const {
	if (!editorObject) {
		return nullptr;
	}
	auto adaptorIt = adaptors_.find(editorObject);
	if (adaptorIt != adaptors_.end()) {
		return adaptorIt->second.get();
	}
	return nullptr;
}

core::Project& SceneAdaptor::project() const {
	return *project_;
}

void SceneAdaptor::rebuildSortedDependencyGraph(SEditorObjectSet const& objects) {
	dependencyGraph_ = buildSortedDependencyGraph(objects);
}

void SceneAdaptor::performBulkEngineUpdate(const core::SEditorObjectSet& changedObjects) {
	if (adaptorStatusDirty_) {
		for (const auto& object : project().instances()) {
			auto adaptor = lookupAdaptor(object);

			bool haveAdaptor = adaptor != nullptr;
			if (haveAdaptor != needAdaptor(object)) {
				if (haveAdaptor) {
					for (auto link : core::Queries::getLinksConnectedToObject(*project_, object, true, true)) {
						removeLink(link->descriptor());
					}
					removeAdaptor(object);
				} else {
					createAdaptor(object);
					for (auto link : core::Queries::getLinksConnectedToObject(*project_, object, true, true)) {
						createLink(link->descriptor());
					}
				}
			}
		}
		adaptorStatusDirty_ = false;
	}

	if (dependencyGraph_.empty() || !changedObjects.empty()) {
		rebuildSortedDependencyGraph(SEditorObjectSet(project_->instances().begin(), project_->instances().end()));
		// Check if all render passes have a unique order index, otherwise Ramses renders them in arbitrary order.
		errors_->removeIf([](core::ErrorItem const& error) {
			return error.valueHandle().isRefToProp(&user_types::RenderPass::renderOrder_) || error.valueHandle().isRefToProp(&user_types::BlitPass::renderOrder_);
		});

		std::map<int, std::vector<core::SEditorObject>> orderIndices;
		for (auto const& obj : project_->instances()) {
			if (obj->isType<user_types::RenderPass>() || obj->isType<user_types::BlitPass>()) {
				int order = obj->get("renderOrder")->asInt();
				orderIndices[order].emplace_back(obj);
			}
		}
		for (auto const& oi : orderIndices) {
			if (oi.second.size() > 1) {				
				auto errorMsg = fmt::format("The render/blit passes {} have the same order index and will be rendered in arbitrary order.", oi.second);
				for (auto const& obj : oi.second) {
					errors_->addError(core::ErrorCategory::GENERAL, core::ErrorLevel::WARNING, ValueHandle{obj, {"renderOrder"}}, errorMsg);
				}
			}
		}
	}
	std::set<LinkAdaptor*> liftedLinks;

	SEditorObjectSet updated;
	for (const auto& item : dependencyGraph_) {
		auto object = item.object;
		if (auto adaptor = lookupAdaptor(object)) {
			bool needsUpdate = adaptor->isDirty();
			if (!needsUpdate) {
				needsUpdate = std::any_of(item.referencedObjects.begin(), item.referencedObjects.end(),
					[&updated](SEditorObject const& object) {
						return updated.find(object) != updated.end();
					});
			}

			if (needsUpdate) {
				auto startIt = links_.linksByStart_.find(object->objectID());
				if (startIt != links_.linksByStart_.end()) {
					for (const auto& [id, link] : startIt->second) {
						liftedLinks.insert(link.get());
						link->lift();
					}
				}

				auto endIt = links_.linksByEnd_.find(object->objectID());
				if (endIt != links_.linksByEnd_.end()) {
					for (const auto& [id, link] : endIt->second) {
						liftedLinks.insert(link.get());
						link->lift();
					}
				}
			}

			if (needsUpdate) {
				auto hasChanged = adaptor->sync(errors_);
				if (hasChanged) {
					updated.insert(object);
				}
			}
		}
	}

	for (const auto& link : liftedLinks) {
		link->connect();
	}

	for (const auto& newLink : newLinks_) {
		auto adaptor = std::make_shared<LinkAdaptor>(newLink, this);
		links_.linksByStart_[newLink.start.object()->objectID()][newLink] = adaptor;
		links_.linksByEnd_[newLink.end.object()->objectID()][newLink] = adaptor;
	}

	newLinks_.clear();

	if (!updated.empty()) {
		deleteUnusedDefaultResources();
	}
}

}  // namespace raco::ramses_adaptor
