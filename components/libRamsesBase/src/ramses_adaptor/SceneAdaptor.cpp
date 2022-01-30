/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
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
#include "log_system/log.h"
#include "ramses_adaptor/AnimationAdaptor.h"
#include "ramses_adaptor/Factories.h"
#include "ramses_adaptor/LuaScriptAdaptor.h"
#include "ramses_adaptor/ObjectAdaptor.h"
#include "ramses_adaptor/OrthographicCameraAdaptor.h"
#include "ramses_adaptor/PerspectiveCameraAdaptor.h"
#include "ramses_base/RamsesHandles.h"
#include "user_types/Animation.h"
#include "user_types/MeshNode.h"
#include "user_types/Prefab.h"
#include "user_types/RenderPass.h"

#include <spdlog/fmt/fmt.h>

#include <algorithm>
#include <map>
#include <memory>
#include <unordered_set>

namespace raco::ramses_adaptor {

using namespace raco::ramses_base;

inline RamsesEffect createDefaultEffect(ramses::Scene* scene, bool withNormals) {
	ramses::EffectDescription effectDescription{};
	effectDescription.setVertexShader(withNormals ? defaultVertexShaderWithNormals : defaultVertexShader);
	effectDescription.setFragmentShader(withNormals ? defaultFragmentShaderWithNormals : defaultFragmentShader);
	effectDescription.setUniformSemantic("mvpMatrix", ramses::EEffectUniformSemantic::ModelViewProjectionMatrix);
	return ramsesEffect(scene, effectDescription, (withNormals) ? defaultEffectWithNormalsName : defaultEffectName);
}

inline RamsesArrayResource createDefaultIndexDataBuffer(ramses::Scene* scene) {
	static std::vector<unsigned int> indices{
		// front
		0, 1, 2,
		2, 3, 0,
		// right
		1, 5, 6,
		6, 2, 1,
		// back
		7, 6, 5,
		5, 4, 7,
		// left
		4, 0, 3,
		3, 7, 4,
		// bottom
		4, 5, 1,
		1, 0, 4,
		// top
		3, 2, 6,
		6, 7, 3};
	return ramsesArrayResource(scene, ramses::EDataType::UInt32, static_cast<uint32_t>(indices.size()), indices.data(), defaultIndexDataBufferName);
}

inline RamsesArrayResource createDefaultVertexDataBuffer(ramses::Scene* scene) {
	static std::vector<float> vertices{
		// front
		-1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		// back
		-1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, 1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f};
	return ramsesArrayResource(scene, ramses::EDataType::Vector3F, static_cast<uint32_t>(vertices.size()), vertices.data(), defaultVertexDataBufferName);
}

inline raco::ramses_base::RamsesAnimationChannelHandle createDefaultAnimationChannel(ramses_base::LogicEngine& logicEngine) {
	auto animHandle = raco::ramses_base::RamsesAnimationChannelHandle(new ramses_base::RamsesAnimationChannelData);
	animHandle->keyframeTimes = ramsesDataArray(std::vector<float>{0.0}, &logicEngine, defaultAnimationChannelTimestampsName);
	animHandle->animOutput = ramsesDataArray(std::vector<float>{0.0}, &logicEngine, defaultAnimationChannelKeyframesName);
	animHandle->name = defaultAnimationChannelName;
	animHandle->interpolationType = rlogic::EInterpolationType::Linear;

	return animHandle;
}

inline ramses_base::RamsesAnimationNode createDefaultAnimation(const raco::ramses_base::RamsesAnimationChannelHandle& animChannel, ramses_base::LogicEngine& logicEngine) {
	rlogic::AnimationChannels channels;
	channels.push_back({animChannel->name,
		animChannel->keyframeTimes.get(),
		animChannel->animOutput.get(),
		animChannel->interpolationType,
		animChannel->tangentIn.get(),
		animChannel->tangentOut.get()});

	return ramsesAnimationNode(channels, &logicEngine, defaultAnimationName);
};


SceneAdaptor::SceneAdaptor(ramses::RamsesClient* client, ramses_base::LogicEngine* logicEngine, ramses::sceneId_t id, Project* project, components::SDataChangeDispatcher dispatcher, core::Errors* errors)
	: client_{client},
	  logicEngine_{logicEngine},
	  project_(project),
	  scene_{ramsesScene(id, client_)},
	  subscription_{dispatcher->registerOnObjectsLifeCycle([this](SEditorObject obj) { createAdaptor(obj); }, [this](SEditorObject obj) { removeAdaptor(obj); })},
	  childrenSubscription_(dispatcher->registerOnPropertyChange("children", [this](core::ValueHandle handle) {
	adaptorStatusDirty_ = true; 
		  })),
	  linksLifecycle_{dispatcher->registerOnLinksLifeCycle(
		  nullptr,
		  [this](const core::LinkDescriptor& link) { createLink(link); },
		  [this](const core::LinkDescriptor& link) { removeLink(link); })},
	  linkValidityChangeSub_{dispatcher->registerOnLinkValidityChange(
		  [this](const core::LinkDescriptor& link) { changeLinkValidity(link, link.isValid); })},
	  dispatcher_{dispatcher},
	  errors_{errors} {

	for (const SEditorObject& obj : project_->instances()) {
		createAdaptor(obj);
	}

	dispatcher_->registerBulkChangeCallback([this](const core::SEditorObjectSet& changedObjects) {
		performBulkEngineUpdate(changedObjects);
	});

	for (const SLink& link : project_->links()) {
		createLink(link->descriptor());
	}

	const auto& instances{project_->instances()};
	core::SEditorObjectSet initialBulkUpdate(instances.begin(), instances.end());
	performBulkEngineUpdate(initialBulkUpdate);

	scene_->flush();
	scene_->publish();
}

SceneAdaptor::~SceneAdaptor() {
	dispatcher_->resetBulkChangeCallback();
}

ramses::Scene* SceneAdaptor::scene() {
	return scene_.get();
}

ramses::sceneId_t SceneAdaptor::sceneId() {
	return scene_->getSceneId();
}

void SceneAdaptor::createAdaptor(SEditorObject obj) {
	if (!core::PrefabOperations::findContainingPrefab(obj)) {
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
	if (adaptorWasLogicProvider) {
		updateRuntimeErrorList();
	}
	dependencyGraph_.clear();
}

void SceneAdaptor::iterateAdaptors(std::function<void(ObjectAdaptor*)> func) {
	for (const auto& [obj, adaptor] : adaptors_) {
		func(adaptor.get());
	}
}

void SceneAdaptor::updateRuntimeErrorList() {
	auto logicEngineErrors = logicEngine().getErrors();
	if (logicEngineErrors.empty()) {
		errors_->removeIf([](const core::ErrorItem& errorItem) {
			return errorItem.category() == core::ErrorCategory::RAMSES_LOGIC_RUNTIME_ERROR;
		});

		return;
	}

	// Avoid having to search the entire list of errors for each instance and O(N*M) complexity
	std::sort(logicEngineErrors.begin(), logicEngineErrors.end(), [](rlogic::ErrorData const& e1, rlogic::ErrorData const& e2) {
		return e1.object < e2.object;
	});
	std::unordered_set<ILogicPropertyProvider*> logicProvidersWithoutRuntimeError;
	std::string runtimeErrorObjectNames;
	for (const auto& instance : project().instances()) {
		if (auto logicProvider = dynamic_cast<ILogicPropertyProvider*>(lookupAdaptor(instance))) {
			std::vector<rlogic::LogicNode*> logicNodes;
			logicProvider->getLogicNodes(logicNodes);
			rlogic::ErrorData const* runtimeError = nullptr;
			for (auto logicNode : logicNodes) {
				auto const itRuntimeErrorForScript = std::lower_bound(logicEngineErrors.begin(), logicEngineErrors.end(), logicNode, [](rlogic::ErrorData const& e, rlogic::LogicNode* s) {
					return e.object < s;
				});
				if (itRuntimeErrorForScript != logicEngineErrors.end() && itRuntimeErrorForScript->object == logicNode) {
					runtimeError = &*itRuntimeErrorForScript;
					break;
				}
			}
			if (runtimeError != nullptr) {
				runtimeErrorObjectNames.append("\n'" + instance->objectName() + "'");

				// keep the old runtime error message if it is identical to the new message to prevent unnecessary error regeneration in the UI
				if (errors_->hasError(instance)) {
					auto instError = errors_->getError(instance);
					if (instError.category() == core::ErrorCategory::RAMSES_LOGIC_RUNTIME_ERROR && instError.message() != runtimeError->message) {
						errors_->removeError(instance);
					}
				}
				logicProvider->onRuntimeError(*errors_, runtimeError->message, core::ErrorLevel::ERROR);
			} else {
				logicProvidersWithoutRuntimeError.emplace(logicProvider);
			}
		}
	}

	// keep the old runtime error info message if it is identical to the new message to prevent unnecessary error regeneration in the UI
	auto ramsesLogicErrorFoundMsg = fmt::format("Ramses logic engine detected a runtime error in{}\nBe aware that some Lua script outputs and/or linked properties might not have been updated.", runtimeErrorObjectNames);
	errors_->removeIf([this, &ramsesLogicErrorFoundMsg, &logicProvidersWithoutRuntimeError](const core::ErrorItem& errorItem) {
		if (auto logicProvider = dynamic_cast<ILogicPropertyProvider*>(lookupAdaptor(errorItem.valueHandle().rootObject()))) {
			return logicProvidersWithoutRuntimeError.count(logicProvider) == 1 && errorItem.category() == core::ErrorCategory::RAMSES_LOGIC_RUNTIME_ERROR && errorItem.message() != ramsesLogicErrorFoundMsg;
		}
		return false;
	});

	for (const auto& logicProvider : logicProvidersWithoutRuntimeError) {
		logicProvider->onRuntimeError(*errors_, ramsesLogicErrorFoundMsg, core::ErrorLevel::INFORMATION);
	}
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
	if (defaultEffect_.use_count() == 1) {
		defaultEffect_.reset();
	}
	if (defaultEffectWithNormals_.use_count() == 1) {
		defaultEffectWithNormals_.reset();
	}
	if (defaultIndices_.use_count() == 1) {
		defaultIndices_.reset();
	}
	if (defaultVertices_.use_count() == 1) {
		defaultVertices_.reset();
	}
	if (defaultAnimation_.use_count() == 1) {
		defaultAnimation_.reset();
		defaultAnimChannel_.reset();
	}
}

void SceneAdaptor::readDataFromEngine(core::DataChangeRecorder& recorder) {
	updateRuntimeErrorList();

	for (const auto& [endObjecttID, linkMap] : links_.linksByEnd_) {
		for (const auto& [link, adaptor] : linkMap) {
			adaptor->readDataFromEngine(recorder);
		}
	}
	for (const auto& [editorObject, adaptor] : adaptors_) {
		if (&editorObject->getTypeDescription() == &user_types::LuaScript::typeDescription) {
			static_cast<LuaScriptAdaptor*>(adaptor.get())->readDataFromEngine(recorder);
		} else if (&editorObject->getTypeDescription() == &user_types::Animation::typeDescription) {
			static_cast<AnimationAdaptor*>(adaptor.get())->readDataFromEngine(recorder);
		}
	}
}

void SceneAdaptor::createLink(const core::LinkDescriptor& link) {	
	newLinks_.emplace_back(link);	
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

ramses_base::LogicEngine& SceneAdaptor::logicEngine() {
	return *logicEngine_;
}

const ramses::RamsesClient* SceneAdaptor::client() const {
	return client_;
}

const SRamsesAdaptorDispatcher SceneAdaptor::dispatcher() const {
	return dispatcher_;
}

const RamsesAppearance SceneAdaptor::defaultAppearance(bool withMeshNormals) {
	if (withMeshNormals) {
		if (!defaultEffectWithNormals_) {
			defaultEffectWithNormals_ = createDefaultEffect(scene_.get(), true);
			defaultAppearanceWithNormals_ = raco::ramses_base::ramsesAppearance(scene(), defaultEffectWithNormals_);
			(*defaultAppearanceWithNormals_)->setName(defaultAppearanceWithNormalsName);
		}
		return defaultAppearanceWithNormals_;
	}

	if (!defaultEffect_) {
		defaultEffect_ = createDefaultEffect(scene_.get(), false);
		defaultAppearance_ = raco::ramses_base::ramsesAppearance(scene(), defaultEffect_);
		(*defaultAppearance_)->setName(defaultAppearanceName);
	}
	return defaultAppearance_;
}

const RamsesArrayResource SceneAdaptor::defaultVertices() {
	if (!defaultVertices_) {
		defaultVertices_ = createDefaultVertexDataBuffer(scene_.get());
	}
	return defaultVertices_;
}

const RamsesArrayResource SceneAdaptor::defaultIndices() {
	if (!defaultIndices_) {
		defaultIndices_ = createDefaultIndexDataBuffer(scene_.get());
	}
	return defaultIndices_;
}

const ramses_base::RamsesAnimationNode SceneAdaptor::defaultAnimation() {
	if (!defaultAnimation_) {
		defaultAnimChannel_ = createDefaultAnimationChannel(*logicEngine_);
		defaultAnimation_ = createDefaultAnimation(defaultAnimChannel_, *logicEngine_);
	}

	return defaultAnimation_;
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

raco::core::Project& SceneAdaptor::project() const {
	return *project_;
}

void SceneAdaptor::depthFirstSearch(data_storage::ReflectionInterface* object, DependencyNode& item, SEditorObjectSet const& instances, SEditorObjectSet& sortedObjs, std::vector<DependencyNode>& outSorted) {
	for (size_t index = 0; index < object->size(); index++) {
		auto v = (*object)[index];
		switch (v->type()) {
			case data_storage::PrimitiveType::Ref: {
				auto refValue = v->asRef();
				if (refValue && instances.find(refValue) != instances.end()) {
					depthFirstSearch(refValue, instances, sortedObjs, outSorted);
					item.referencedObjects.insert(refValue);
				}
				break;
			}
			case data_storage::PrimitiveType::Table:
				depthFirstSearch(&v->asTable(), item, instances, sortedObjs, outSorted);
				break;
		}
	}
}

void SceneAdaptor::depthFirstSearch(SEditorObject object, SEditorObjectSet const& instances, SEditorObjectSet& sortedObjs, std::vector<DependencyNode>& outSorted) {
	using namespace raco::core;

	if (sortedObjs.find(object) != sortedObjs.end()) {
		return;
	}

	DependencyNode item;
	item.object = object;
	depthFirstSearch(object.get(), item, instances, sortedObjs, outSorted);

	outSorted.emplace_back(item);
	sortedObjs.insert(item.object);
}

void SceneAdaptor::rebuildSortedDependencyGraph(SEditorObjectSet const& objects) {
	dependencyGraph_.clear();
	dependencyGraph_.reserve(objects.size());
	SEditorObjectSet sortedObjs;
	for (auto obj : objects) {
		depthFirstSearch(obj, objects, sortedObjs, dependencyGraph_);
	}
}

void SceneAdaptor::performBulkEngineUpdate(const core::SEditorObjectSet& changedObjects) {
	if (adaptorStatusDirty_) {
		for (const auto& item : dependencyGraph_) {
			auto object = item.object;
			auto adaptor = lookupAdaptor(object);

			bool haveAdaptor = adaptor != nullptr;
			bool needAdaptor = !core::PrefabOperations::findContainingPrefab(object);
			if (haveAdaptor != needAdaptor) {
				if (haveAdaptor) {
					removeAdaptor(object);
				} else {
					createAdaptor(object);
				}
			}
		}
		adaptorStatusDirty_ = false;
	}

	if (dependencyGraph_.empty() || !changedObjects.empty()) {
		rebuildSortedDependencyGraph(SEditorObjectSet(project_->instances().begin(), project_->instances().end()));
		// Check if all render passes have a unique order index, otherwise Ramses renders them in arbitrary order.
		errors_->removeIf([](core::ErrorItem const& error) {
			return error.valueHandle().isRefToProp(&user_types::RenderPass::order_);
		});
		auto renderPasses = core::Queries::filterByTypeName(project_->instances(), {user_types::RenderPass::typeDescription.typeName});
		std::map<int, std::vector<user_types::SRenderPass>> orderIndices;
		for (auto const& rpObj : renderPasses) {
			auto rp = rpObj->as<user_types::RenderPass>();
			orderIndices[rp->order_.asInt()].emplace_back(rp);
		}
		for (auto const& oi : orderIndices) {
			if (oi.second.size() > 1) {				
				auto errorMsg = fmt::format("The render passes {} have the same order index and will be rendered in arbitrary order.", oi.second);
				for (auto const& rp : oi.second) {
					errors_->addError(core::ErrorCategory::GENERAL, core::ErrorLevel::WARNING, ValueHandle{rp, &user_types::RenderPass::order_}, errorMsg);
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
