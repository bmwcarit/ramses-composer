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

#include "core/Iterators.h"
#include "core/Project.h"
#include "core/Queries.h"
#include "log_system/log.h"
#include "ramses_adaptor/Factories.h"
#include "ramses_adaptor/LuaScriptAdaptor.h"
#include "ramses_adaptor/ObjectAdaptor.h"
#include "ramses_base/RamsesHandles.h"
#include "components/DataChangeDispatcher.h"
#include "components/EditorObjectFormatter.h"
#include "user_types/MeshNode.h"
#include "user_types/Prefab.h"

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

SceneAdaptor::SceneAdaptor(ramses::RamsesClient* client, ramses_base::LogicEngine* logicEngine, ramses::sceneId_t id, Project* project, components::SDataChangeDispatcher dispatcher, core::Errors* errors)
	: client_{client},
	  logicEngine_{logicEngine},
	  project_(project),
	  scene_{ramsesScene(id, client_)},
	  defaultRenderGroup_{ramsesRenderGroup(scene_.get())},
	  subscription_{dispatcher->registerOnObjectsLifeCycle([this](SEditorObject obj) { createAdaptor(obj); }, [this](SEditorObject obj) { removeAdaptor(obj); })},
	  linksLifecycle_{dispatcher->registerOnLinksLifeCycle(
		  [this](const core::LinkDescriptor& link) { createLink(link); },
		  [this](const core::LinkDescriptor& link) { removeLink(link); })},
	  linkValidityChangeSub_{dispatcher->registerOnLinkValidityChange(
		  [this](const core::LinkDescriptor& link) {
			changeLinkValidity(link, link.isValid); })},
	  dispatcher_{dispatcher},
	  errors_{errors} {
	defaultRenderGroup_->setName(defaultRenderGroupName);

	for (const SEditorObject& obj : project_->instances()) {
		createAdaptor(obj);
	}

	dispatcher_->registerBulkChangeCallback([this](const std::set<SEditorObject>& changedObjects) {
		performBulkEngineUpdate(changedObjects);
	});
	const auto& instances{project_->instances()};
	std::set<SEditorObject> initialBulkUpdate(instances.begin(), instances.end());
	performBulkEngineUpdate(initialBulkUpdate);

	for (const SLink& link : project_->links()) {
		createLink(link->descriptor());
	}

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
	auto adaptor = Factories::createAdaptor(this, obj);
	if (adaptor) {
		adaptor->tagDirty();
		adaptors_[obj] = std::move(adaptor);
	}
}

void SceneAdaptor::removeAdaptor(SEditorObject obj) {
	auto adaptorWasLogicProvider = dynamic_cast<ILogicPropertyProvider*>(lookupAdaptor(obj)) != nullptr;
	adaptors_.erase(obj);
	deleteUnusedDefaultResources();
	if (adaptorWasLogicProvider) {
		updateRuntimeErrorList();
	}
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
		return e1.node < e2.node;
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
					return e.node < s;
				});
				if (itRuntimeErrorForScript != logicEngineErrors.end() && itRuntimeErrorForScript->node == logicNode) {
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
}

void SceneAdaptor::readDataFromEngine(core::DataChangeRecorder& recorder) {
	updateRuntimeErrorList();

	for (const auto& [link, adaptor] : links_) {
		adaptor->readDataFromEngine(recorder);
	}
	for (const auto& adaptor : adaptors_) {
		if (adaptor.first->getTypeDescription().typeName == user_types::LuaScript::typeDescription.typeName) {
			static_cast<LuaScriptAdaptor*>(adaptor.second.get())->readDataFromEngine(recorder);
		}
	}
}

void SceneAdaptor::createLink(const core::LinkDescriptor& link) {
	links_[link] = std::make_unique<LinkAdaptor>(link, this);
}

void SceneAdaptor::changeLinkValidity(const core::LinkDescriptor& link, bool isValid) {
	assert(links_.count(link) > 0);
	links_[link]->editorLink().isValid = isValid;
}

void SceneAdaptor::removeLink(const core::LinkDescriptor& link) {
	auto it = links_.find(link);
	assert(it != links_.end());
	links_.erase(it);
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

ramses::RenderGroup& SceneAdaptor::defaultRenderGroup() {
	return *defaultRenderGroup_;
}

void SceneAdaptor::setCamera(ramses::Camera* camera) {
	if (camera == nullptr) {
		defaultRenderPass_.reset();
	} else {
		defaultRenderPass_ = ramsesRenderPass(scene_.get());
		defaultRenderPass_->addRenderGroup(defaultRenderGroup());
		defaultRenderPass_->setCamera(*camera);
		defaultRenderPass_->setName(defaultRenderPassName);
		// Ramses ignores the clear flags/clear color because we are rendering to the default framebuffer.
		// The default framebuffer for our scene happens to be an offscreen render buffer (this is set up in
		// RamsesPreviewWindow::commit). Ramses will give an export warning if we apply any clear flags to a
		// render pass rendering to the default framebuffer.
		// The clear color is set with RamsesRenderer::setDisplayBufferClearColor (see RamsesPreviewWindow::commit).
		defaultRenderPass_->setClearFlags(ramses::EClearFlags_None);
	}
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

ObjectAdaptor* SceneAdaptor::lookupAdaptor(const core::SEditorObject& editorObject) const {
	auto adaptorIt = adaptors_.find(editorObject);
	if (adaptorIt != adaptors_.end()) {
		return adaptorIt->second.get();
	}
	return nullptr;
}

raco::core::Project& SceneAdaptor::project() const {
	return *project_;
}

void SceneAdaptor::depthFirstSearch(data_storage::ReflectionInterface* object, DependencyNode& item, std::set<SEditorObject> const& instances, std::set<SEditorObject>& sortedObjs, std::vector<DependencyNode>& outSorted) {

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

void SceneAdaptor::depthFirstSearch(SEditorObject object, std::set<SEditorObject> const& instances, std::set<SEditorObject>& sortedObjs, std::vector<DependencyNode>& outSorted) {
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

void SceneAdaptor::rebuildSortedDependencyGraph(std::set<SEditorObject> const& objects) {
	dependencyGraph_.clear();
	dependencyGraph_.reserve(objects.size());
	std::set<SEditorObject> sortedObjs;
	for (auto obj : objects) {
		depthFirstSearch(obj, objects, sortedObjs, dependencyGraph_);
	}
}

void SceneAdaptor::buildDefaultRenderGroup() {
	defaultRenderGroup().removeAllRenderables();
	defaultRenderGroup().removeAllRenderGroups();

	std::vector<SEditorObject> topLevelNodes;
	for (auto const& child : project_->instances()) {
		if (!child->getParent() && child->getTypeDescription().typeName != raco::user_types::Prefab::typeDescription.typeName) {
			topLevelNodes.emplace_back(child);
		}
	}

	auto defaultMeshOrderIndex = 0;
	auto defaultRenderableOrderFunc = [&defaultMeshOrderIndex, this](const SEditorObject& obj) {
		if (obj->getTypeDescription().typeName == raco::user_types::MeshNode::typeDescription.typeName) {
			auto renderGroupObj = lookup<IRenderGroupObject>(obj);
			renderGroupObj->addObjectToRenderGroup(defaultRenderGroup(), defaultMeshOrderIndex++);
		}
	};

	buildRenderableOrder(defaultRenderGroup(), topLevelNodes, defaultRenderableOrderFunc);
}

void SceneAdaptor::buildRenderableOrder(ramses::RenderGroup& renderGroup, std::vector<SEditorObject>& objs, const std::function<void(const SEditorObject&)>& renderableOrderFunc) {
	for (const auto& obj : objs) {
		renderableOrderFunc(obj); 
		if (obj->children_->size() > 0) {
			auto vec = obj->children_->asVector<SEditorObject>();
			buildRenderableOrder(renderGroup, vec, renderableOrderFunc);
		}
	}
}

void SceneAdaptor::performBulkEngineUpdate(const std::set<core::SEditorObject>& changedObjects) {
	if (dependencyGraph_.empty() || !changedObjects.empty()) {
		buildDefaultRenderGroup();
		rebuildSortedDependencyGraph(std::set<SEditorObject>(project_->instances().begin(), project_->instances().end()));
	}
	std::set<LinkAdaptor*> liftedLinks;

	std::set<SEditorObject> updated;
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
				for (const auto& link : links_) {
					if (link.first.start.object() == object || link.first.end.object() == object) {
						liftedLinks.insert(link.second.get());
						link.second->lift();
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

	if (!updated.empty()) {
		deleteUnusedDefaultResources();
	}
}

}  // namespace raco::ramses_adaptor
