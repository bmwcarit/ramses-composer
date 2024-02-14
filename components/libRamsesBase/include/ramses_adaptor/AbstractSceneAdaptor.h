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

#include "components/DataChangeDispatcher.h"
#include "core/Context.h"
#include "core/Link.h"
#include "ramses_adaptor/AbstractObjectAdaptor.h"
#include "ramses_adaptor/CameraController.h"
#include "ramses_adaptor/ClearDepthObject.h"
#include "ramses_adaptor/DefaultRamsesObjects.h"
#include "ramses_adaptor/Gizmos.h"
#include "ramses_adaptor/InfiniteGrid.h"
#include "ramses_adaptor/utilities.h"
#include "ramses_base/RamsesHandles.h"
#include <map>

#include <QObject>

namespace raco::ramses_adaptor {

class SceneAdaptor;

	
using SRamsesAdaptorDispatcher = std::shared_ptr<components::DataChangeDispatcher>;

class AbstractSceneAdaptor : public QObject {
	Q_OBJECT
	using Project = core::Project;
	using SEditorObject = core::SEditorObject;
	using ValueHandle = core::ValueHandle;
	using SEditorObjectSet = core::SEditorObjectSet;

public:
	explicit AbstractSceneAdaptor(ramses::RamsesClient* client, ramses::sceneId_t id, Project* project, components::SDataChangeDispatcher dispatcher, core::MeshCache* meshCache, SceneAdaptor* previewAdaptor);

	~AbstractSceneAdaptor();

	ramses::Scene* scene();
	ramses::sceneId_t sceneId();

	void setPreviewAdaptor(SceneAdaptor* previewAdaptor);
	SceneAdaptor* previewAdaptor();

	/* START: Adaptor API */
	ramses::RamsesClient* client();
	const ramses::RamsesClient* client() const;
	const SRamsesAdaptorDispatcher dispatcher() const;
	const ramses_base::RamsesAppearance defaultAppearance(bool withMeshNormals, bool highlight);
	const ramses_base::RamsesArrayResource defaultVertices(int index);
	const ramses_base::RamsesArrayResource defaultNormals(int index);
	const ramses_base::RamsesArrayBuffer defaultTriangles(int index);
	const ramses_base::RamsesArrayResource defaultIndices(int index);
	AbstractObjectAdaptor* lookupAdaptor(const core::SEditorObject& editorObject) const;
	Project& project() const;

	template <class T>
	T* lookup(const core::SEditorObject& editorObject) const {
		return dynamic_cast<T*>(lookupAdaptor(editorObject));
	}
	/* END: Adaptor API */

	void iterateAdaptors(std::function<void(AbstractObjectAdaptor*)> func);

	void rescaleCameraToViewport(uint32_t width, uint32_t height);

	CameraController& cameraController();
	ramses_base::RamsesPerspectiveCamera camera();

	/**
	 * @brief Get world-space bounding box of all meshnodes in the scenegraph subtrees of the given objects.
	 * @return Merged bounding box.
	 */
	BoundingBox getBoundingBox(std::vector<core::SEditorObject> objects);

	float gridScale();

	void setHighlightedObjects(const std::vector<SEditorObject>& objects);
	void attachGizmo(const std::vector<SEditorObject>& objects);

	enum class GizmoMode {
		None,
		Locator,
		Translate,
		Rotate,
		Scale
	};

	void setGizmoMode(GizmoMode mode);
	GizmoMode gizmoMode();


	void setHighlightUsingTransparency(bool useTransparency);

	ramses::pickableObjectId_t getPickId(SEditorObject object);
	ramses::pickableObjectId_t getPickId();
	SEditorObject getPickedObject(ramses::pickableObjectId_t pickId);
	std::pair<int, GizmoTriad::PickElement> getPickedGizmoElement(const std::vector<ramses::pickableObjectId_t>& pickIds);


	/**
	 * @brief Return model matrix for the given object. The model matrix is obtained from Ramses.
	 * @param object Object to query the model matrix for. This may be a nullptr.
	 * @return Return model matrix if object is a Node or child type of Node and unit matrix otherwise.
	*/
	glm::mat4 modelMatrix(SEditorObject object);

	bool hasModelMatrix(SEditorObject object);

	void enableGuides(glm::vec3 origin, glm::vec3 u, glm::vec3 v, int idx_u, int idx_v, bool full_grid, glm::vec2 enable);
	void disableGuides();

Q_SIGNALS:
	/**
	 * @brief Signal emitted when the grid scale changes
	 * @param newScale is the new scale of the grid.
	 */
	void scaleChanged(float newScale);

private:
	bool needAdaptor(SEditorObject object);
	void createAdaptor(SEditorObject obj);
	void removeAdaptor(SEditorObject obj);

	void performBulkEngineUpdate(const core::SEditorObjectSet& changedObjects);
	void rebuildSortedDependencyGraph(SEditorObjectSet const& objects);
	void deleteUnusedDefaultResources();

	void updateGridScale(float cameraDistance);
	void updateGizmo();

	ramses::RamsesClient* client_;
	Project* project_;
	ramses_base::RamsesScene scene_{};
	core::MeshCache* meshCache_;
	SceneAdaptor* previewAdaptor_;

	struct Flags {
		bool normals;
		bool highlight;
		bool transparent;

		bool operator<(const Flags& other) const;
	};
	std::map<Flags, ramses_base::RamsesAppearance> defaultAppearances_;

	std::array<ramses_base::RamsesArrayResource, 2> defaultIndices_;
	std::array<ramses_base::RamsesArrayResource, 2> defaultVertices_;
	std::array<ramses_base::RamsesArrayResource, 2> defaultNormals_;
	std::array<ramses_base::RamsesArrayBuffer, 2> defaultTriangles_;
	
	RamsesGizmoMeshBuffers gizmoArrowBuffers_;
	RamsesGizmoMeshBuffers gizmoScaleBuffers_;
	RamsesGizmoMeshBuffers gizmoSphereBuffers_;
	RamsesGizmoMeshBuffers gizmoTorusBuffers_;

	std::map<SEditorObject, std::unique_ptr<AbstractObjectAdaptor>> adaptors_{};

	components::Subscription subscription_;
	components::Subscription childrenSubscription_;
	SRamsesAdaptorDispatcher dispatcher_;

	bool adaptorStatusDirty_ = false;

	std::vector<DependencyNode> dependencyGraph_;

	ramses_base::RamsesPerspectiveCamera camera_;
	ramses_base::RamsesRenderGroup renderGroup_;
	ramses_base::RamsesRenderPass renderPass_;
	ramses_base::RamsesRenderGroup gizmoRenderGroup_;
	ramses_base::RamsesRenderGroup gizmoRenderGroupTransparent_;
	
	SEditorObject gizmoObject_ = nullptr;
	std::unique_ptr<GizmoTriad> gizmo_;
	GizmoMode gizmoMode_ = GizmoMode::Locator;

	CameraController cameraController_;

	InfiniteGrid grid_;
	float gridScale_;

	ClearDepthObject clearDepth_;

	InfiniteGrid guides_;

	bool highlightUsingTransparency_ = false;

	std::map<SEditorObject, ramses::pickableObjectId_t> objectPickIds_;
	uint32_t nextFreePickId_{0};
};

}  // namespace raco::ramses_adaptor
