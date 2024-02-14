/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_adaptor/AbstractSceneAdaptor.h"

#include "components/DataChangeDispatcher.h"
#include "components/EditorObjectFormatter.h"
#include "core/Iterators.h"
#include "core/PrefabOperations.h"
#include "core/Project.h"
#include "core/ProjectSettings.h"
#include "core/Queries.h"
#include "ramses_adaptor/AbstractMeshNodeAdaptor.h"
#include "ramses_adaptor/Factories.h"
#include "ramses_base/RamsesHandles.h"
#include "user_types/MeshNode.h"
#include "user_types/RenderPass.h"

#include <algorithm>
#include <map>
#include <memory>
#include <unordered_set>

namespace raco::ramses_adaptor {

using namespace raco::ramses_base;

bool AbstractSceneAdaptor::Flags::operator<(const Flags& other) const {
	unsigned flags = (normals ? 1 : 0) | (highlight ? 2 : 0) | (transparent ? 4 : 0);
	unsigned otherFlags = (other.normals ? 1 : 0) | (other.highlight ? 2 : 0) | (other.transparent ? 4 : 0);
	return flags < otherFlags;
}

AbstractSceneAdaptor::AbstractSceneAdaptor(ramses::RamsesClient* client, ramses::sceneId_t id, Project* project, components::SDataChangeDispatcher dispatcher, core::MeshCache* meshCache, SceneAdaptor* previewAdaptor)
	: client_{client},
	  project_(project),
	  scene_{ramsesScene(id, client_)},
	  meshCache_(meshCache),
	  previewAdaptor_(previewAdaptor),
	  subscription_{dispatcher->registerOnObjectsLifeCycle([this](SEditorObject obj) { createAdaptor(obj); }, [this](SEditorObject obj) { removeAdaptor(obj); })},
	  childrenSubscription_(dispatcher->registerOnPropertyChange("children", [this](core::ValueHandle handle) {
		  adaptorStatusDirty_ = true;
	  })),
	  dispatcher_{dispatcher},
	  camera_{ramsesPerspectiveCamera(scene(), {0, 0})},
	  cameraController_(camera_),
	  grid_(scene_.get()),
	  clearDepth_(scene_.get()),
	  guides_(scene_.get()) {
	for (const SEditorObject& obj : project_->instances()) {
		createAdaptor(obj);
	}

	dispatcher_->addBulkChangeCallback(id.getValue(), [this](const core::SEditorObjectSet& changedObjects) {
		performBulkEngineUpdate(changedObjects);
	});

	rescaleCameraToViewport(1440, 720);

	renderGroup_ = ramsesRenderGroup(scene(), {0, 0});

	renderPass_ = ramses_base::ramsesRenderPass(scene(), camera_, {}, "abstractRenderPass", {0, 0});
	renderPass_->addRenderGroup(renderGroup_);

	gizmoRenderGroup_ = ramsesRenderGroup(scene(), {0, 0});
	renderPass_->addRenderGroup(gizmoRenderGroup_, 3);

	gizmoRenderGroupTransparent_ = ramsesRenderGroup(scene(), {0, 0});
	renderPass_->addRenderGroup(gizmoRenderGroupTransparent_, 1);

	grid_.setup(renderPass_, true, 0.75, 1);
	updateGridScale(cameraController_.cameraDistance());
	grid_.enable(glm::vec3(0, 0, 0), glm::vec3(1, 0, 0), glm::vec3(0, 0, 1), 0, 2, true, glm::vec2(1, 1));

	clearDepth_.setup(renderPass_, 2);

	guides_.setup(renderPass_, false, 1.0, 4);


	const auto& instances{project_->instances()};
	core::SEditorObjectSet initialBulkUpdate(instances.begin(), instances.end());
	performBulkEngineUpdate(initialBulkUpdate);

	scene_->flush();
	scene_->publish(ramses::EScenePublicationMode::LocalAndRemote);

	QObject::connect(&cameraController_, &CameraController::distanceChanged, [this](float cameraDistance) {
		updateGridScale(cameraDistance);
	});

	gizmoArrowBuffers_ = loadGizmoMesh(scene_.get(), meshCache_, "meshes/gizmo-arrow.glb");
	gizmoScaleBuffers_ = loadGizmoMesh(scene_.get(), meshCache_, "meshes/gizmo-scale.glb");
	gizmoSphereBuffers_ = loadGizmoMesh(scene_.get(), meshCache_, "meshes/sphere-ico.glb");
	gizmoTorusBuffers_ = loadGizmoMesh(scene_.get(), meshCache_, "meshes/gizmo-torus.glb");
}

AbstractSceneAdaptor::~AbstractSceneAdaptor() {
	dispatcher_->removeBulkChangeCallback(sceneId().getValue());
}

void AbstractSceneAdaptor::updateGridScale(float cameraDistance) {
	gridScale_ = pow(10.0, floor(log10(cameraDistance / 2.0)));
	grid_.setScale(gridScale_);
	Q_EMIT scaleChanged(gridScale_);
}

void AbstractSceneAdaptor::rescaleCameraToViewport(uint32_t width, uint32_t height) {
	cameraController_.rescaleCameraToViewport(width, height);
}

CameraController& AbstractSceneAdaptor::cameraController() {
	return cameraController_;
}

ramses_base::RamsesPerspectiveCamera AbstractSceneAdaptor::camera() {
	return camera_;
}

BoundingBox AbstractSceneAdaptor::getBoundingBox(std::vector<core::SEditorObject> selection) {
	BoundingBox bbox;
	for (auto obj : core::Queries::collectAllChildren(selection)) {
		if (auto adaptor = lookup<AbstractMeshNodeAdaptor>(obj)) {
			bbox.merge(adaptor->getBoundingBox(true));
		}
	}
	return bbox;
}

float AbstractSceneAdaptor::gridScale() {
	return gridScale_;
}

ramses::Scene* AbstractSceneAdaptor::scene() {
	return scene_.get();
}

ramses::sceneId_t AbstractSceneAdaptor::sceneId() {
	return scene_->getSceneId();
}

void AbstractSceneAdaptor::setPreviewAdaptor(SceneAdaptor* previewAdaptor) {
	previewAdaptor_ = previewAdaptor;
}

SceneAdaptor* AbstractSceneAdaptor::previewAdaptor() {
	return previewAdaptor_;
}

bool AbstractSceneAdaptor::needAdaptor(SEditorObject object) {
	return !core::PrefabOperations::findContainingPrefab(object) && !object->isType<core::ProjectSettings>();
}

void AbstractSceneAdaptor::createAdaptor(SEditorObject obj) {
	if (needAdaptor(obj)) {
		auto adaptor = Factories::createAbstractAdaptor(this, obj);
		if (adaptor) {
			adaptor->tagDirty();
			adaptors_[obj] = std::move(adaptor);
		}
	}
}

void AbstractSceneAdaptor::removeAdaptor(SEditorObject obj) {
	adaptors_.erase(obj);
	deleteUnusedDefaultResources();
	dependencyGraph_.clear();
	if (gizmo_ && gizmo_->object() == obj) {
		gizmo_.reset();
	}
}

void AbstractSceneAdaptor::iterateAdaptors(std::function<void(AbstractObjectAdaptor*)> func) {
	for (const auto& [obj, adaptor] : adaptors_) {
		func(adaptor.get());
	}
}

void AbstractSceneAdaptor::deleteUnusedDefaultResources() {
	// Resource use count is 1 => it is stored in the scene but not used by any MeshNodeAdaptor
	// - delete the resource as to not get unnecessarily exported.
	auto it = defaultAppearances_.begin();
	while (it != defaultAppearances_.end()) {
		if (it->second.use_count() == 1) {
			it = defaultAppearances_.erase(it);
		} else {
			++it;
		}
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
	if (defaultTriangles_[0].use_count() == 1) {
		defaultTriangles_[0].reset();
	}
	if (defaultTriangles_[1].use_count() == 1) {
		defaultTriangles_[1].reset();
	}
}

ramses::RamsesClient* AbstractSceneAdaptor::client() {
	return client_;
}

const ramses::RamsesClient* AbstractSceneAdaptor::client() const {
	return client_;
}

const SRamsesAdaptorDispatcher AbstractSceneAdaptor::dispatcher() const {
	return dispatcher_;
}

const RamsesAppearance AbstractSceneAdaptor::defaultAppearance(bool withMeshNormals, bool highlight) {
	bool transparent = !highlight && highlightUsingTransparency_;
	auto flags = Flags{withMeshNormals, highlight, transparent};

	if (defaultAppearances_.find(flags) == defaultAppearances_.end()) {
		defaultAppearances_[flags] = createDefaultAppearance(scene_.get(), withMeshNormals, highlight, transparent);
	}
	return defaultAppearances_[flags];
}


const RamsesArrayResource AbstractSceneAdaptor::defaultVertices(int index) {
	if (!defaultVertices_[index]) {
		defaultVertices_[index] = index == 1 ? createCatVertexDataBuffer(scene_.get()) : createCubeVertexDataBuffer(scene_.get());
	}
	return defaultVertices_[index];
}

const RamsesArrayResource AbstractSceneAdaptor::defaultNormals(int index) {
	if (!defaultNormals_[index]) {
		defaultNormals_[index] = index == 1 ? createCatNormalDataBuffer(scene_.get()) : createCubeNormalDataBuffer(scene_.get());
	}
	return defaultNormals_[index];
}

const RamsesArrayBuffer AbstractSceneAdaptor::defaultTriangles(int index) {
	if (!defaultTriangles_[index]) {
		const std::vector<glm::vec3>& vertices = index == 1 ? cat_vertex_data : cubeVerticesData;
		const std::vector<uint32_t>& indices = index == 1 ? cat_indices_data : cubeIndicesData;
		auto triangleData = core::MeshData::buildTriangleBuffer(vertices.data(), indices);
		defaultTriangles_[index] = ramsesArrayBuffer(scene(), ramses::EDataType::Vector3F, triangleData.size(), triangleData.data());
	}
	return defaultTriangles_[index];
}

const RamsesArrayResource AbstractSceneAdaptor::defaultIndices(int index) {
	if (!defaultIndices_[index]) {
		defaultIndices_[index] = index == 1 ? createCatIndexDataBuffer(scene_.get()) : createCubeIndexDataBuffer(scene_.get());
	}
	return defaultIndices_[index];
}

AbstractObjectAdaptor* AbstractSceneAdaptor::lookupAdaptor(const core::SEditorObject& editorObject) const {
	if (!editorObject) {
		return nullptr;
	}
	auto adaptorIt = adaptors_.find(editorObject);
	if (adaptorIt != adaptors_.end()) {
		return adaptorIt->second.get();
	}
	return nullptr;
}

core::Project& AbstractSceneAdaptor::project() const {
	return *project_;
}

void AbstractSceneAdaptor::rebuildSortedDependencyGraph(SEditorObjectSet const& objects) {
	dependencyGraph_ = buildSortedDependencyGraph(objects);
}

void AbstractSceneAdaptor::performBulkEngineUpdate(const core::SEditorObjectSet& changedObjects) {
	if (adaptorStatusDirty_) {
		for (const auto& item : dependencyGraph_) {
			auto object = item.object;
			auto adaptor = lookupAdaptor(object);

			bool haveAdaptor = adaptor != nullptr;
			if (haveAdaptor != needAdaptor(object)) {
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
	}

	renderGroup_->removeAllRenderables();

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
				auto hasChanged = adaptor->sync();
				if (hasChanged) {
					updated.insert(object);
				}
			}
		}
	}

	for (const auto& item : dependencyGraph_) {
		auto object = item.object;
		if (auto adaptor = lookup<ramses_adaptor::AbstractMeshNodeAdaptor>(object)) {
			bool transparent = highlightUsingTransparency_ && !adaptor->highlighted();
			renderGroup_->addMeshNode(adaptor->getRamsesObjectPointer(), transparent ? 1 : 0);
		}
	}

	if (gizmo_) {
		gizmo_->update();
	}

	if (!updated.empty()) {
		deleteUnusedDefaultResources();
	}
}

void AbstractSceneAdaptor::setHighlightedObjects(const std::vector<SEditorObject>& objects) {
	auto highlightedObjects = core::Queries::collectAllChildren(objects);
	for (const auto& [obj, adaptor] : adaptors_) {
		if (auto meshNodeAdaptor = dynamic_cast<AbstractMeshNodeAdaptor*>(adaptor.get())) {
			meshNodeAdaptor->setHighlighted(highlightedObjects.find(obj) != highlightedObjects.end());
		}
	}
}

void AbstractSceneAdaptor::updateGizmo() {
	gizmo_.reset();
	if (gizmoObject_) {
		switch (gizmoMode_) {
			case GizmoMode::Locator:
				gizmo_ = std::make_unique<GizmoTriad>(this, scene_.get(), gizmoRenderGroup_, gizmoRenderGroupTransparent_, gizmoArrowBuffers_, gizmoObject_);
				break;
			case GizmoMode::Translate:
				gizmo_ = std::make_unique<GizmoTransformation>(this, scene_.get(), gizmoRenderGroup_, gizmoRenderGroupTransparent_, gizmoArrowBuffers_, gizmoSphereBuffers_, gizmoObject_, true, true, false);
				break;
			case GizmoMode::Rotate:
				gizmo_ = std::make_unique<GizmoTransformation>(this, scene_.get(), gizmoRenderGroup_, gizmoRenderGroupTransparent_, gizmoTorusBuffers_, gizmoSphereBuffers_, gizmoObject_, false, false, true);
				break;
			case GizmoMode::Scale:
				gizmo_ = std::make_unique<GizmoTransformation>(this, scene_.get(), gizmoRenderGroup_, gizmoRenderGroupTransparent_, gizmoScaleBuffers_, gizmoSphereBuffers_, gizmoObject_, false, true, false);
				break;
		}
	}
}

void AbstractSceneAdaptor::attachGizmo(const std::vector<SEditorObject>& objects) {
	if (objects.size() == 1 && hasModelMatrix(objects.front())) {
		if (objects.front() != gizmoObject_) {
			gizmoObject_ = objects.front();
			updateGizmo();
		}
	} else if (gizmoObject_) {
		gizmoObject_.reset();
		updateGizmo();
	}
}

void AbstractSceneAdaptor::setGizmoMode(GizmoMode mode) {
	if (mode != gizmoMode_) {
		gizmoMode_ = mode;
		updateGizmo();
	}
}

AbstractSceneAdaptor::GizmoMode AbstractSceneAdaptor::gizmoMode() {
	return gizmoMode_;
}

void AbstractSceneAdaptor::setHighlightUsingTransparency(bool useTransparency) {
	if (highlightUsingTransparency_ != useTransparency) {
		highlightUsingTransparency_ = useTransparency;
		for (const auto& [obj, adaptor] : adaptors_) {
			if (auto meshNodeAdaptor = dynamic_cast<AbstractMeshNodeAdaptor*>(adaptor.get())) {
				meshNodeAdaptor->tagDirty();
			}
		}
	}
}

ramses::pickableObjectId_t AbstractSceneAdaptor::getPickId(SEditorObject object) {
	auto it = objectPickIds_.find(object);
	if (it != objectPickIds_.end()) {
		return it->second;
	}
	return objectPickIds_[object] = ramses::pickableObjectId_t(nextFreePickId_++);
}

ramses::pickableObjectId_t AbstractSceneAdaptor::getPickId() {
	return ramses::pickableObjectId_t(nextFreePickId_++);
}

core::SEditorObject AbstractSceneAdaptor::getPickedObject(ramses::pickableObjectId_t pickId) {
	auto it = std::find_if(objectPickIds_.begin(), objectPickIds_.end(), [pickId](auto item) {
		return item.second == pickId;
	});
	if (it != objectPickIds_.end()) {
		return it->first;
	}
	return {};
}

std::pair<int, GizmoTriad::PickElement> AbstractSceneAdaptor::getPickedGizmoElement(const std::vector<ramses::pickableObjectId_t>& pickIds) {
	if (gizmo_) {
		for (auto id : pickIds) {
			auto element = gizmo_->pickElement(id);
			if (element.first != -1) {
				return element;
			}
		}
	}
	return {-1, GizmoTriad::PickElement::None};
}

bool AbstractSceneAdaptor::hasModelMatrix(SEditorObject object) {
	return lookup<AbstractMeshNodeAdaptor>(object) || lookup<AbstractNodeAdaptor>(object);
}

glm::mat4 AbstractSceneAdaptor::modelMatrix(SEditorObject node) {
	glm::mat4 matrix = glm::identity<glm::mat4>();
	if (auto adaptor = lookup<AbstractMeshNodeAdaptor>(node)) {
		(*adaptor->ramsesObject()).getModelMatrix(matrix);
	} else if (auto adaptor = lookup<AbstractNodeAdaptor>(node)) {
		(*adaptor->ramsesObject()).getModelMatrix(matrix);
	}
	return matrix;
}

void AbstractSceneAdaptor::enableGuides(glm::vec3 origin, glm::vec3 u, glm::vec3 v, int idx_u, int idx_v, bool full_grid, glm::vec2 enable) {
	guides_.enable(origin, u, v, idx_u, idx_v, full_grid, enable);
}

void AbstractSceneAdaptor::disableGuides() {
	guides_.disable();
}

}  // namespace raco::ramses_adaptor